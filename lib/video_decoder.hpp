#pragma once

#include <memory>

extern "C" {
    #include <libavcodec/avcodec.h>
}

#include "encoded_av_handler.hpp"
#include "logger.hpp"
#include "mpeg4.hpp"

// VideoDecoder takes incoming video and synchronously decodes it.
class VideoDecoder : public EncodedVideoHandler {
public:
    explicit VideoDecoder(Logger logger) : _logger{std::move(logger)} {}
    virtual ~VideoDecoder();

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override;
    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

private:
    const Logger _logger;

    std::unique_ptr<AVCDecoderConfigurationRecord> _videoConfig;

    AVCodecContext* _context = nullptr;

    void _beginDecoding();
    void _endDecoding();
};
