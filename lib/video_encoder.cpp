#include "lib/video_encoder.hpp"

#include "ffmpeg.hpp"
#include "mpeg4.hpp"

#include <h264/h264.hpp>
#include <h264/nal_unit.hpp>
#include <h264/seq_parameter_set.hpp>

VideoEncoder::~VideoEncoder() {
    _endEncoding();
}

void VideoEncoder::handleVideo(std::chrono::microseconds pts, const AVFrame* frame) {
    if (!_context) {
        _beginEncoding(static_cast<AVPixelFormat>(frame->format));
    }

    if (!_context) {
        return;
    }

    auto clone = av_frame_clone(frame);
    clone->pts = pts.count() / 1000000.0 / av_q2d(_context->time_base);
    clone->pict_type = AV_PICTURE_TYPE_NONE;

	auto err = avcodec_send_frame(_context, clone);
    av_frame_free(&clone);
	if (err < 0) {
		_logger.error("error sending video frame to encoder: {}", FFmpegErrorString(err));
		return;
	}

    _handleEncodedPackets();
}

void VideoEncoder::_beginEncoding(AVPixelFormat pixelFormat) {
    _endEncoding();
    InitFFmpeg();

    auto codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        _logger.error("encoder not found");
        return;
    }

    _context = avcodec_alloc_context3(codec);
    if (!_context) {
        _logger.error("unable to allocate codec context");
        return;
    }

    _context->bit_rate = _configuration.bitrate;
    _context->width = _configuration.width;
    _context->height = _configuration.height;
    _context->pix_fmt = pixelFormat;
    _context->max_b_frames = 1;

    // TODO: probably something different. not specifying the actual framerate will make bitrate calculations inaccurate
    _context->time_base = AVRational{1, 120};
    _context->ticks_per_frame = 2;

    AVDictionary* options = nullptr;
    av_dict_set(&options, "preset", _configuration.h264Preset.c_str(), 0);
    av_dict_set(&options, "rc-lookahead", "5", 0);

    auto err = avcodec_open2(_context, codec, &options);
    if (err < 0) {
        _logger.error("unable to open codec: {}", FFmpegErrorString(err));
        av_free(_context);
        _context = nullptr;
        return;
    }
}

void VideoEncoder::_endEncoding() {
    if (!_context) {
        return;
    }

	auto err = avcodec_send_frame(_context, nullptr);
	if (err < 0) {
		_logger.error("error sending video flush frame to encoder: {}", FFmpegErrorString(err));
	} else {
        _handleEncodedPackets();
    }

    avcodec_close(_context);
    av_free(_context);
    _context = nullptr;
}

void VideoEncoder::_handleEncodedPackets() {
    AVPacket packet{0};
    while (true) {
        auto err = avcodec_receive_packet(_context, &packet);
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            break;
        } else if (err < 0) {
		    _logger.error("error receiving video packets from encoder: {}", FFmpegErrorString(err));
            break;
        }

        std::unique_ptr<AVCDecoderConfigurationRecord> config;

        if (!h264::IterateAnnexB(packet.data, packet.size, [&](const void* data, size_t len) {
            auto naluType = *reinterpret_cast<const uint8_t*>(data) & 0x0f;

            if (naluType != h264::NALUnitType::SequenceParameterSet && naluType != h264::NALUnitType::PictureParameterSet) {
                return;
            }

            if (!config) {
                config = std::make_unique<AVCDecoderConfigurationRecord>();
                config->lengthSizeMinusOne = 3;
            }

            h264::nal_unit nalu;
            h264::bitstream bs{data, len};
            auto err = nalu.decode(&bs, len);
            if (err) {
                _logger.error("error decoding config nalu: {}", err.message);
                return;
            }

            auto bytePtr = reinterpret_cast<const uint8_t*>(data);

            if (naluType == h264::NALUnitType::SequenceParameterSet) {
                config->avcProfileIndication = nalu.rbsp_byte[0];
                config->profileCompatibility = nalu.rbsp_byte[1];
                config->avcLevelIndication = nalu.rbsp_byte[2];
                config->sequenceParameterSets.emplace_back(bytePtr, bytePtr + len);
            } else if (naluType == h264::NALUnitType::PictureParameterSet) {
                config->pictureParameterSets.emplace_back(bytePtr, bytePtr + len);
            }
        })) {
            _logger.error("unable to iterate encoded nalus");
        } else {

            if (config) {
                if (config->sequenceParameterSets.empty() || config->pictureParameterSets.empty()) {
                    _logger.error("expected both sequence and picture parameter sets with frame");
                } else {
                    auto encodedConfig = config->encode();
                    _configuration.handler->handleEncodedVideoConfig(encodedConfig.data(), encodedConfig.size());
                }
            }

            auto pts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(packet.pts * av_q2d(_context->time_base)));
            auto dts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(packet.dts * av_q2d(_context->time_base)));

            _outputBuffer.clear();
            if (!h264::AnnexBToAVCC(&_outputBuffer, packet.data, packet.size)) {
                _logger.error("unable to convert annex-b encoder output to avcc");
            } else {
                _configuration.handler->handleEncodedVideo(pts, dts, _outputBuffer.data(), _outputBuffer.size());
            }
        }

        av_packet_unref(&packet);
    }
}
