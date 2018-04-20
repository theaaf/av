#pragma once

#include <memory>

extern "C" {
    #include <libavcodec/avcodec.h>
}

#include "av_handler.hpp"
#include "encoded_av_handler.hpp"
#include "logger.hpp"
#include "mpeg4.hpp"

// VideoDecoder takes incoming video and synchronously decodes it.
class VideoDecoder : public EncodedVideoHandler {
public:
    VideoDecoder(Logger logger, VideoHandler* handler) : _logger{std::move(logger)}, _handler{handler} {}
    virtual ~VideoDecoder();

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override;
    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

private:
    const Logger _logger;
    VideoHandler* const _handler;

    std::vector<uint8_t> _inputBuffer;

    std::unique_ptr<AVCDecoderConfigurationRecord> _videoConfig;

    AVCodecContext* _context = nullptr;

    void _beginDecoding();
    void _endDecoding();
    void _handleDecodedFrames();
};
