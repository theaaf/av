#include "packager.hpp"

#include <cstring>

#include <h264/nal_unit.hpp>

#include "ffmpeg.hpp"

Packager::Packager(Logger logger, SegmentStorage* storage)
    : _logger{std::move(logger)}, _storage{storage}
{
    constexpr size_t kBufferSize = 4 * 1024;
    _ioContextBuffer = av_malloc(kBufferSize);
    _ioContext = avio_alloc_context(reinterpret_cast<unsigned char*>(_ioContextBuffer), kBufferSize, 1, this, nullptr, &_writePacket, nullptr);
}

Packager::~Packager() {
    _endSegment();

    av_free(_ioContext);
    av_free(_ioContextBuffer);
}

void Packager::beginNewSegment() {
    std::lock_guard<std::mutex> l{_mutex};
    _shouldBeginNewSegment = true;
}

void Packager::handleEncodedAudioConfig(const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    auto config = std::make_unique<MPEG4AudioSpecificConfig>();
    if (!config->decode(data, len)) {
        _logger.error("unable to decode audio config");
        _endSegment();
        return;
    } else if (_audioConfig && *config == *_audioConfig) {
        return;
    }

    _audioConfig = std::move(config);
}

void Packager::handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};
    if (!_audioConfig || !_audioStream) {
        return;
    }

    auto adts = _audioConfig->adtsHeader(len);
    _inputBuffer.resize(adts.size() + len);
    std::memcpy(&_inputBuffer[0], adts.data(), adts.size());
    std::memcpy(&_inputBuffer[adts.size()], data, len);

    AVPacket packet{0};
    av_init_packet(&packet);
    packet.data = _inputBuffer.data();
    packet.size = _inputBuffer.size();
    packet.stream_index = _audioStream->index;
    packet.pts = pts.count() / 1000000.0 / av_q2d(_audioStream->time_base);
    packet.dts = packet.pts;

    auto err = av_interleaved_write_frame(_outputContext, &packet);
    if (err != 0) {
        _logger.error("error writing audio frame: {}", FFmpegErrorString(err));
    }
}

void Packager::handleEncodedVideoConfig(const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    auto config = std::make_unique<AVCDecoderConfigurationRecord>();
    if (!config->decode(data, len)) {
        _logger.error("unable to decode video config");
        _endSegment();
        _videoConfig = nullptr;
        _videoConfigSPS = nullptr;
        return;
    } else if (_videoConfig && *config == *_videoConfig) {
        return;
    }

    _videoConfig = nullptr;
    _videoConfigSPS = nullptr;

    if (config->sequenceParameterSets.size() != 1) {
        _logger.error("expected 1 sequence parameter set (got {})", config->sequenceParameterSets.size());
        _endSegment();
        return;
    }

    h264::nal_unit nalu;
    h264::bitstream bs{config->sequenceParameterSets[0].data(), config->sequenceParameterSets[0].size()};
    auto err = nalu.decode(&bs, config->sequenceParameterSets[0].size());
    if (err) {
        _logger.error("error decoding sps nalu: {}", err.message);
        _endSegment();
        return;
    }

    bs = {nalu.rbsp_byte.data(), nalu.rbsp_byte.size()};
    auto configSPS = std::make_unique<h264::seq_parameter_set_rbsp>();
    err = configSPS->decode(&bs);
    if (err) {
        _logger.error("error decoding sps rbsp: {}", err.message);
        _endSegment();
        return;
    }

    _videoConfig = std::move(config);
    _videoConfigSPS = std::move(configSPS);
}

void Packager::handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};
    if (!_videoConfig) {
        return;
    }

    auto isIDR = false;
    if (!h264::IterateAVCC(data, len, _videoConfig->lengthSizeMinusOne + 1, [&](const void* data, size_t len) {
        if (len < 1) {
            return;
        }
        auto naluType = *reinterpret_cast<const uint8_t*>(data) & 0x1f;
        isIDR |= (naluType == h264::NALUnitType::IDRSlice);
    })) {
        _logger.error("unable to iterate avcc");
        return;
    }

    if (isIDR && (!_videoStream || _shouldBeginNewSegment)) {
        _shouldBeginNewSegment = false;
        _beginSegment(pts);
    }

    if (!_videoStream) {
        return;
    }

    _inputBuffer.clear();
    if (!h264::AVCCToAnnexB(&_inputBuffer, data, len, _videoConfig->lengthSizeMinusOne + 1)) {
        _logger.error("unable to convert avcc to annex-b");
        return;
    }

    AVPacket packet{0};
    av_init_packet(&packet);
    packet.data = _inputBuffer.data();
    packet.size = _inputBuffer.size();
    packet.stream_index = _videoStream->index;
    packet.pts = pts.count() / 1000000.0 / av_q2d(_videoStream->time_base);
    packet.dts = dts.count() / 1000000.0 / av_q2d(_videoStream->time_base);
    if (isIDR) {
        packet.flags |= AV_PKT_FLAG_KEY;
    }

    auto err = av_interleaved_write_frame(_outputContext, &packet);
    if (err != 0) {
        _logger.error("error writing video frame: {}", FFmpegErrorString(err));
    }

    if (pts > _maxVideoPTS) {
        _maxVideoPTS = pts;
    }
}

void Packager::_beginSegment(std::chrono::microseconds pts) {
    _endSegment();
    InitFFmpeg();

    if (!_audioConfig || !_videoConfig) {
        return;
    }

    _logger.with(
        "video_height", _videoConfigSPS->FrameCroppingRectangleHeight(),
        "video_width", _videoConfigSPS->FrameCroppingRectangleWidth()
    ).info("beginning new segment");

    _segment = _storage->createSegment("ts");
    if (!_segment) {
        _logger.error("unable to create segment");
        _endSegment();
        return;
    }

    auto err = avformat_alloc_output_context2(&_outputContext, nullptr, "mpegts", nullptr);
    if (err < 0) {
        _logger.error("unable to allocate output context: {}", FFmpegErrorString(err));
        _endSegment();
        return;
    }

    _segmentPTS = pts;

    _audioStream = avformat_new_stream(_outputContext, nullptr);
    _audioStream->codecpar->codec_id = AV_CODEC_ID_AAC;
    _audioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    _audioStream->codecpar->sample_rate = _audioConfig->frequency;
    _audioStream->codecpar->channels = _audioConfig->channelCount();

    _videoStream = avformat_new_stream(_outputContext, nullptr);
    _videoStream->codecpar->codec_id = AV_CODEC_ID_H264;
    _videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    _videoStream->codecpar->width = _videoConfigSPS->FrameCroppingRectangleWidth();
    _videoStream->codecpar->height = _videoConfigSPS->FrameCroppingRectangleHeight();

    size_t extraDataSize = 0;
    for (auto sps : _videoConfig->sequenceParameterSets) {
        extraDataSize += 3 + sps.size();
    }
    for (auto pps : _videoConfig->pictureParameterSets) {
        extraDataSize += 3 + pps.size();
    }
    auto ptr = reinterpret_cast<uint8_t*>(av_malloc(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
    _videoStream->codecpar->extradata = ptr;
    _videoStream->codecpar->extradata_size = extraDataSize;
    for (auto sps : _videoConfig->sequenceParameterSets) {
        ptr[0] = 0;
        ptr[1] = 0;
        ptr[2] = 1;
        std::memcpy(ptr + 3, sps.data(), sps.size());
        ptr += 3 + sps.size();
    }
    for (auto pps : _videoConfig->pictureParameterSets) {
        ptr[0] = 0;
        ptr[1] = 0;
        ptr[2] = 1;
        std::memcpy(ptr + 3, pps.data(), pps.size());
        ptr += 3 + pps.size();
    }

    _outputContext->pb = _ioContext;

    err = avformat_write_header(_outputContext, nullptr);
    if (err < 0) {
        _logger.error("unable to write header: {}", FFmpegErrorString(err));
        _endSegment(false);
        return;
    }
}

void Packager::_endSegment(bool writeTrailer) {
    if (_outputContext) {
        if (writeTrailer) {
            av_write_trailer(_outputContext);
        }
        avformat_free_context(_outputContext);
        _outputContext = nullptr;
        _audioStream = nullptr;
        _videoStream = nullptr;
    }

    if (_segment) {
        auto duration = _maxVideoPTS - _segmentPTS;
        _logger.with("duration_ms", std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()).info("closing segment");
        if (!_segment->close(duration)) {
            _logger.error("unable to close segment");
        }
        _segment = nullptr;
    }
}

int Packager::_writePacket(void* opaque, uint8_t* buf, int len) {
    auto self = reinterpret_cast<Packager*>(opaque);
    if (self->_segment) {
        if (!self->_segment->write(buf, len)) {
            self->_logger.error("unable to write to segment");
            return AVERROR_UNKNOWN;
        }
    }
    return len;
}
