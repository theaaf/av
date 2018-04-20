#pragma once

#include <string>
#include <utility>
#include <vector>

extern "C" {
    #include <libavcodec/avcodec.h>
}

#include "av_handler.hpp"
#include "encoded_av_handler.hpp"
#include "logger.hpp"

// VideoEncoder takes incoming video and synchronously encodes it.
class VideoEncoder : public VideoHandler {
public:
    struct Configuration {
        int bitrate = 4000000;
        int width = -1;
        int height = -1;
        std::string h264Preset = "medium";
        EncodedVideoHandler* handler = nullptr;
    };

    VideoEncoder(Logger logger, Configuration configuration)
        : _logger{std::move(logger)}, _configuration{std::move(configuration)} {}

    virtual ~VideoEncoder();

    virtual void handleVideo(std::chrono::microseconds pts, const AVFrame* frame) override;

private:
    const Logger _logger;
    const Configuration _configuration;

    std::vector<uint8_t> _outputBuffer;

    AVCodecContext* _context = nullptr;

    void _beginEncoding(AVPixelFormat pixelFormat);
    void _endEncoding();
    void _handleEncodedPackets();
};
