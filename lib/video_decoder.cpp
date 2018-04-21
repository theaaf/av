#include "video_decoder.hpp"

#include "ffmpeg.hpp"

#include <h264/h264.hpp>

VideoDecoder::~VideoDecoder() {
    _endDecoding();
}

void VideoDecoder::flush() {
    _endDecoding();
}

void VideoDecoder::handleEncodedVideoConfig(const void* data, size_t len) {
    auto config = std::make_unique<AVCDecoderConfigurationRecord>();
    if (!config->decode(data, len)) {
        _logger.error("unable to decode video config");
        _endDecoding();
        _videoConfig = nullptr;
        return;
    } else if (_videoConfig && *config == *_videoConfig) {
        return;
    }

    _videoConfig = std::move(config);
}

void VideoDecoder::handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) {
    if (!_videoConfig) {
        return;
    }

    auto isFirstFrame = false;

    if (!_context) {
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

        if (isIDR) {
            _beginDecoding();
            isFirstFrame = true;
        }
    }

    if (!_context) {
        return;
    }

    _inputBuffer.clear();

    if (isFirstFrame) {
        for (auto& sps : _videoConfig->sequenceParameterSets) {
            _inputBuffer.emplace_back(0);
            _inputBuffer.emplace_back(0);
            _inputBuffer.emplace_back(1);
            _inputBuffer.insert(_inputBuffer.end(), sps.begin(), sps.end());
        }

        for (auto& pps : _videoConfig->pictureParameterSets) {
            _inputBuffer.emplace_back(0);
            _inputBuffer.emplace_back(0);
            _inputBuffer.emplace_back(1);
            _inputBuffer.insert(_inputBuffer.end(), pps.begin(), pps.end());
        }
    }

    if (!h264::AVCCToAnnexB(&_inputBuffer, data, len, _videoConfig->lengthSizeMinusOne + 1)) {
        _logger.error("unable to convert avcc to annex-b");
        return;
    }

    _inputBuffer.resize(_inputBuffer.size() + AV_INPUT_BUFFER_PADDING_SIZE);

    AVPacket packet{0};
    av_init_packet(&packet);
    packet.data = _inputBuffer.data();
    packet.size = _inputBuffer.size() - AV_INPUT_BUFFER_PADDING_SIZE;

    _context->reordered_opaque = pts.count();

	auto err = avcodec_send_packet(_context, &packet);
	if (err < 0) {
		_logger.error("error sending video packet to decoder: {}", FFmpegErrorString(err));
		return;
	}

    _handleDecodedFrames();
}

void VideoDecoder::_beginDecoding() {
    _endDecoding();
    InitFFmpeg();

    auto codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        _logger.error("decoder not found");
        return;
    }

    _context = avcodec_alloc_context3(codec);
    if (!_context) {
        _logger.error("unable to allocate codec context");
        return;
    }
    RegisterFFmpegLogContext(_context, _logger);

    auto err = avcodec_open2(_context, codec, nullptr);
    if (err < 0) {
        _logger.error("unable to open codec: {}", FFmpegErrorString(err));
        av_free(_context);
        UnregisterFFmpegLogContext(_context);
        _context = nullptr;
        return;
    }
}

void VideoDecoder::_endDecoding() {
    if (!_context) {
        return;
    }

	auto err = avcodec_send_packet(_context, nullptr);
	if (err < 0) {
		_logger.error("error sending video flush packet to decoder: {}", FFmpegErrorString(err));
	} else {
        _handleDecodedFrames();
    }

    avcodec_close(_context);
    av_free(_context);
    UnregisterFFmpegLogContext(_context);
    _context = nullptr;
}

void VideoDecoder::_handleDecodedFrames() {
    auto frame = av_frame_alloc();
    while (true) {
        auto err = avcodec_receive_frame(_context, frame);
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            break;
        } else if (err < 0) {
		    _logger.error("error receiving video frames from decoder: {}", FFmpegErrorString(err));
            break;
        }

        _handler->handleVideo(std::chrono::microseconds{frame->reordered_opaque}, frame);
    }
	av_frame_free(&frame);
}
