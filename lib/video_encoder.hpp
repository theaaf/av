#pragma once

#include <string>
#include <utility>
#include <vector>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
}

#include "av_handler.hpp"
#include "encoded_av_handler.hpp"
#include "logger.hpp"

#include <h264/h264.hpp>

// VideoEncoder takes incoming video and synchronously encodes it.
class VideoEncoder : public VideoHandler {
public:
    struct Configuration {
        int bitrate = 4000000;
        int width = -1;
        int height = -1;
        int profileIDC = h264::ProfileIDC::High;
        int levelIDC = 31;
        std::string h264Preset = "fast";
    };

    VideoEncoder(Logger logger, EncodedVideoHandler* handler, Configuration configuration)
        : _logger{std::move(logger)}, _handler{handler}, _configuration{std::move(configuration)} {}

    virtual ~VideoEncoder();

    // flush outputs encoded video for the frames received so far.
    void flush();

    virtual void handleVideo(std::chrono::microseconds pts, const AVFrame* frame) override;

private:
    const Logger _logger;
    EncodedVideoHandler* const _handler;
    const Configuration _configuration;

    std::vector<uint8_t> _outputBuffer;

    AVCodecContext* _context = nullptr;
    SwsContext* _scalingContext = nullptr;
    AVFrame* _scaledFrame = nullptr;

    void _beginEncoding(int inputWidth, int inputHeight, AVPixelFormat inputPixelFormat);
    void _endEncoding();
    void _handleEncodedPackets();
};
