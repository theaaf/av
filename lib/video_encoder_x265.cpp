#include "lib/video_encoder.hpp"

extern "C" {
    #include <libavutil/imgutils.h>
}

#include "ffmpeg.hpp"
#include "mpeg4.hpp"
#include <h26x/nal_unit.hpp>

H265VideoEncoder::~H265VideoEncoder() {
    _endEncoding();
}

void H265VideoEncoder::_beginEncoding(int inputWidth, int inputHeight, AVPixelFormat inputPixelFormat) {
    _endEncoding();
    InitFFmpeg();

    auto codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if (!codec) {
        _logger.error("encoder AV_CODEC_ID_HEVC not found");
        return;
    }
    _context = avcodec_alloc_context3(codec);
    if (!_context) {
        _logger.error("unable to allocate hevc codec context");
        return;
    }
    RegisterFFmpegLogContext(_context, _logger);

    _context->bit_rate = _configuration.bitrate;
    _context->width = _configuration.width;
    _context->height = _configuration.height;
    _context->pix_fmt = inputPixelFormat;

    // @TODO: is this a good setting for hevc?
    _context->max_b_frames = 1;
    _context->profile = _configuration.x265.profile;
    _context->level = _configuration.x265.level;

    // @TODO: verify these are good for hevc
    _context->time_base = AVRational{1, 120};
    _context->ticks_per_frame = 2;

    AVDictionary* options = nullptr;
    // @TODO: figure out options
    av_dict_set(&options, "bframes", "0", 0);
    av_dict_set(&options, "b-adapt", "0", 0);
    av_dict_set(&options, "rc-lookahead", "0", 0);
    av_dict_set(&options, "no-cenecut", "", 0);
    av_dict_set(&options, "no-cutree", "", 0);
    av_dict_set(&options, "frame-threads", "1", 0);

    auto err = avcodec_open2(_context, codec, &options);
    if (err < 0) {
        _logger.error("unable to open codec: {}", FFmpegErrorString(err));
        av_free(_context);
        UnregisterFFmpegLogContext(_context);
        _context = nullptr;
        return;
    }

    if (inputWidth != _configuration.width || inputHeight != _configuration.height) {
        _scaledFrame = av_frame_alloc();
        _scaledFrame->format = _context->pix_fmt;
        _scaledFrame->width = _context->width;
        _scaledFrame->height = _context->height;
        auto err = av_image_alloc(_scaledFrame->data, _scaledFrame->linesize, _context->width, _context->height, _context->pix_fmt, 32);
        if (err < 0) {
            _logger.error("unable to allocate image buffer");
            return;
        }
        _scalingContext = sws_getContext(
                inputWidth, inputHeight, inputPixelFormat,
                _context->width, _context->height, _context->pix_fmt,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!_scalingContext) {
            _logger.error("unable to create scaling context");
            return;
        }
    }
}

void H265VideoEncoder::_handleEncodedPackets() {
    AVPacket packet{0};
    while (true) {
        auto err = avcodec_receive_packet(_context, &packet);
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            break;
        } else if (err < 0) {
            _logger.error("error receiving video packets from encoder: {}", FFmpegErrorString(err));
            break;
        }
        std::unique_ptr<HEVCDecoderConfigurationRecord> config;

        if (!h264::IterateAnnexB(packet.data, packet.size, [&](const void* data, size_t len) {
            auto rawNalContents = reinterpret_cast<const uint8_t*>(data);
            auto naluType = (*rawNalContents & 0x7F) >> 1;

            if (naluType != h265::NALUnitType::SequenceParameterSet &&
                naluType != h265::NALUnitType::PictureParameterSet &&
                naluType != h265::NALUnitType::VideoParameterSet) {
                return;
            }
            if (!config) {
                config = std::make_unique<HEVCDecoderConfigurationRecord>();
            }
            h265::nal_unit nalu;
            h264::bitstream bs{data, len};
            if (auto err = nalu.decode(&bs, len)) {
                _logger.error("error decoding h265 config nalu: {}", err.message);
                return;
            }

            switch (naluType) {
                case h265::NALUnitType::VideoParameterSet:
                    if(auto err = config->parseVPS(nalu)){
                        _logger.error("error parsing vps: {}", err.message);
                    }
                    break;
                case h265::NALUnitType::SequenceParameterSet:
                    if (auto err = config->parseSPS(nalu)) {
                        _logger.error("error parsing sps: {}", err.message);
                    }
                    break;
                case h265::NALUnitType::PictureParameterSet:
                    if(auto err = config->parsePPS(nalu)) {
                        _logger.error("error parsing pps: {}", err.message);
                    }
                    break;
            }
            config->addNalu(rawNalContents, len);

        })) {
            _logger.error("unable to iterate encoded h265 nalus");
        } else {
            if (config) {
                if (!config->fetchNalus(h265::NALUnitType::SequenceParameterSet) || !config->fetchNalus(h265::NALUnitType::PictureParameterSet) ){
                    _logger.error("expected both SPS and PPS with frame");
                } else {
                    auto encodedConfig = config->encode();
                    _handler->handleEncodedVideoConfig(encodedConfig.data(), encodedConfig.size());
                }
            }
            auto pts = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::duration<double>(packet.pts * av_q2d(_context->time_base)));
            auto dts = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::duration<double>(packet.dts * av_q2d(_context->time_base)));

            _handler->handleEncodedVideo(pts, dts, packet.data, packet.size);
        }
        av_packet_unref(&packet);
    }
}