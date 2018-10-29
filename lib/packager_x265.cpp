#include "packager.hpp"
#include "ffmpeg.hpp"
#include <h26x/h265.hpp>

void H265Packager::_beginSegment(std::chrono::microseconds pts) {
    _endSegment(true, pts);
    InitFFmpeg();

    if (!_audioConfig || !std::holds_alternative<UniqueHEVCDecoderRecord>(_decoderRecord)) {
        return;
    }
    auto& videoConfig = std::get<UniqueHEVCDecoderRecord>(_decoderRecord);

    if (videoConfig.get() == nullptr) {
        _logger.error("UniqueHEVCDecoderRecord is holding a nullptr");
        _decoderRecord = nullptr;
        return;
    }

    _logger.with(
            "video_height", videoConfig->width,
            "video_width", videoConfig->height
    ).info("beginning new segment");

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
    }

    _segmentPTS = pts;

    _audioStream = avformat_new_stream(_outputContext, nullptr);
    _audioStream->codecpar->codec_id = AV_CODEC_ID_AAC;
    _audioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    _audioStream->codecpar->sample_rate = _audioConfig->frequency;
    _audioStream->codecpar->channels = _audioConfig->channelCount();

    _videoStream = avformat_new_stream(_outputContext, nullptr);
    _videoStream->codecpar->codec_id = AV_CODEC_ID_HEVC;
    _videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    _videoStream->codecpar->width = videoConfig->width;
    _videoStream->codecpar->height = videoConfig->height;

    size_t extraDataSize = 0;
    auto vps_nals = videoConfig->fetchNalus(h265::NALUnitType::VideoParameterSet);
    auto sps_nals = videoConfig->fetchNalus(h265::NALUnitType::VideoParameterSet);
    auto pps_nals = videoConfig->fetchNalus(h265::NALUnitType::VideoParameterSet);
    for (auto& vps : *vps_nals) {
        extraDataSize += 3 + vps.size();
    }
    for (auto& sps : *sps_nals) {
        extraDataSize += 3 + sps.size();
    }
    for (auto& pps : *pps_nals) {
        extraDataSize += 3 + pps.size();
    }
    auto ptr = reinterpret_cast<uint8_t*>(av_malloc(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
    _videoStream->codecpar->extradata = ptr;
    _videoStream->codecpar->extradata_size = extraDataSize;
    for (auto& vps : *vps_nals) {
        ptr[0] = 0;
        ptr[1] = 0;
        ptr[2] = 1;
        std::memcpy(ptr + 3, vps.data(), vps.size());
        ptr += 3 + vps.size();
    }
    for (auto& sps : *sps_nals) {
        ptr[0] = 0;
        ptr[1] = 0;
        ptr[2] = 1;
        std::memcpy(ptr + 3, sps.data(), sps.size());
        ptr += 3 + sps.size();
    }
    for (auto& pps : *pps_nals) {
        ptr[0] = 0;
        ptr[1] = 0;
        ptr[2] = 0;
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

void H265Packager::handleEncodedVideoConfig(const void *data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    auto config = std::make_unique<HEVCDecoderConfigurationRecord>();
    if (!config->decode(data, len)) {
        _logger.error("unable to decode hevc video config");
        _endSegment();
        _decoderRecord = nullptr;
        return;
    }
    _decoderRecord = nullptr;

    /// do some sanity checking? the decoder record should tell us everything we need to know

    auto vps_nals = config->fetchNalus(h265::NALUnitType::VideoParameterSet);
    auto sps_nals = config->fetchNalus(h265::NALUnitType::SequenceParameterSet);
    auto pps_nals = config->fetchNalus(h265::NALUnitType::PictureParameterSet);
    if (!vps_nals || !sps_nals || !pps_nals) {
        _logger.warn("HEVCDecoderConfigurationRecord missing: VPS({}) SPS({}) PPS({})", !vps_nals ? "yes":"no", !sps_nals ? "yes":"no", !pps_nals ? "yes":"no");
        _endSegment();
        return;
    }
    _decoderRecord = std::move(config);
}

void H265Packager::handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void *data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};
    if (!std::holds_alternative<UniqueHEVCDecoderRecord>(_decoderRecord)) {
        return;
    }

    auto isRandomAccess = false;
    if (!h264::IterateAnnexB(data, len, [&](const void* data, size_t len) {
        if (len < 1) {
            return;
        }
        auto naluType = (*reinterpret_cast<const uint8_t*>(data) & 0x7F) >> 1;

        isRandomAccess |=
                naluType == h265::NALUnitType::IDR_W_RADL ||
                naluType == h265::NALUnitType::IDR_N_LP ||
                naluType == h265::NALUnitType::CRA_NUT;
    })) {
        _logger.error("unable to iterate annexB");
        return;
    }

    if( isRandomAccess && (!_videoStream || _shouldBeginNewSegment)) {
        _shouldBeginNewSegment = false;
        _beginSegment(pts);
    }

    if (!_videoStream) {
        return;
    }

    AVPacket packet{0};
    av_init_packet(&packet);
    _inputBuffer.clear();
    _inputBuffer.insert(_inputBuffer.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + len);
    packet.data = _inputBuffer.data();
    packet.size = _inputBuffer.size();
    packet.stream_index = _videoStream->index;

    packet.pts = pts.count() / 1000000.0 / av_q2d(_videoStream->time_base);
    packet.dts = dts.count() / 1000000.0 / av_q2d(_videoStream->time_base);
    if (isRandomAccess) {
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

