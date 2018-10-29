#include "packager.hpp"

#include <cstring>

#include <h26x/nal_unit.hpp>

#include "ffmpeg.hpp"

void H264Packager::_beginSegment(std::chrono::microseconds pts) {
    _endSegment(true, pts);
    InitFFmpeg();

    if (!_audioConfig || !std::holds_alternative<UniqueAVCDecoderRecord>(_decoderRecord)) {
        return;
    }
    auto& videoConfig = std::get<UniqueAVCDecoderRecord>(_decoderRecord);

    if (videoConfig.get() == nullptr) {
        _logger.error("UniqueAVCDecoderRecord is holding a nullptr");
        _decoderRecord = nullptr;
        return;
    }

    _logger.with(
            "video_height", _videoConfigSPS->FrameCroppingRectangleHeight(),
            "video_width", _videoConfigSPS->FrameCroppingRectangleWidth()
    ).info("beginning new segment");

    if(_shouldMarkNextSegmentDiscontinuous) {
        _logger.info("segment created with discontinuity");
    }

    _segment = _storage->createSegment("ts");
    _segment->metadata.discontinuity = _shouldMarkNextSegmentDiscontinuous;
    _shouldMarkNextSegmentDiscontinuous = false;

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
    for (auto sps : videoConfig->sequenceParameterSets) {
        extraDataSize += 3 + sps.size();
    }
    for (auto pps : videoConfig->pictureParameterSets) {
        extraDataSize += 3 + pps.size();
    }
    auto ptr = reinterpret_cast<uint8_t*>(av_malloc(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
    _videoStream->codecpar->extradata = ptr;
    _videoStream->codecpar->extradata_size = extraDataSize;
    for (auto sps : videoConfig->sequenceParameterSets) {
        ptr[0] = 0;
        ptr[1] = 0;
        ptr[2] = 1;
        std::memcpy(ptr + 3, sps.data(), sps.size());
        ptr += 3 + sps.size();
    }
    for (auto pps : videoConfig->pictureParameterSets) {
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

void H264Packager::handleEncodedVideoConfig(const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    auto config = std::make_unique<AVCDecoderConfigurationRecord>();
    if (!config->decode(data, len)) {
        _logger.error("unable to decode video config");
        _endSegment();
        _decoderRecord = nullptr;
        _videoConfigSPS = nullptr;
        return;
    }
    if (std::holds_alternative<UniqueAVCDecoderRecord>(_decoderRecord)) {
        auto& videoConfig = std::get<UniqueAVCDecoderRecord>(_decoderRecord);
        if (*config ==  *videoConfig) {
            return;
        }
    }

    _decoderRecord = nullptr;
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

    _decoderRecord = std::move(config);
    _videoConfigSPS = std::move(configSPS);
}

void H264Packager::handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};
    if (!std::holds_alternative<UniqueAVCDecoderRecord>(_decoderRecord)) {
        return;
    }

    auto& videoConfig = std::get<UniqueAVCDecoderRecord>(_decoderRecord);

    auto isIDR = false;
    if (!h264::IterateAVCC(data, len, videoConfig->lengthSizeMinusOne + 1, [&](const void* data, size_t len) {
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
    if (!h264::AVCCToAnnexB(&_inputBuffer, data, len, videoConfig->lengthSizeMinusOne + 1)) {
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