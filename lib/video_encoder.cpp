#include "lib/video_encoder.hpp"

extern "C" {
    #include <libavutil/imgutils.h>
}

#include "ffmpeg.hpp"

void VideoEncoder::flush() {
    _endEncoding();
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
    UnregisterFFmpegLogContext(_context);
    _context = nullptr;

    if (_scalingContext) {
        sws_freeContext(_scalingContext);
        _scalingContext = nullptr;
    }

    if (_scaledFrame) {
        av_freep(&_scaledFrame->data[0]);
        av_frame_free(&_scaledFrame);
        _scaledFrame = nullptr;
    }
}

void VideoEncoder::handleVideo(std::chrono::microseconds pts, const AVFrame* frame) {
    if (!_context) {
        _beginEncoding(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format));
    }

    if (!_context) {
        return;
    }

    AVFrame* frameCopy = nullptr;
    if (_scalingContext) {
        sws_scale(_scalingContext, frame->data, frame->linesize, 0, frame->height, _scaledFrame->data, _scaledFrame->linesize);
        frameCopy = _scaledFrame;
    } else {
        frameCopy = av_frame_clone(frame);
    }

    frameCopy->pts = pts.count() / 1000000.0 / av_q2d(_context->time_base);
    frameCopy->pict_type = AV_PICTURE_TYPE_NONE;

    auto err = avcodec_send_frame(_context, frameCopy);
    if (!_scalingContext) {
        av_frame_free(&frameCopy);
    }
    if (err < 0) {
        _logger.error("error sending video frame to encoder: {}", FFmpegErrorString(err));
        return;
    }

    _handleEncodedPackets();
}