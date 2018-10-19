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

#include <h26x/h264.hpp>
#include <h26x/h265.hpp>

enum class VideoCodec {
    null, x264, x265
};

// Unified encoder configuration; each codec uses only the fields relative to it
struct VideoEncoderConfiguration {
    VideoCodec codec = VideoCodec::null;
    int bitrate = 4000000;
    int width = -1;
    int height = -1;

    struct  {
        int profileIDC = h264::ProfileIDC::High;
        int levelIDC = 31;
        std::string h264Preset = "fast";

    } x264;

    struct {
        int profile = 1; // @TODO: https://x265.readthedocs.io/en/default/presets.html
        int level = h265::Levels::L_1080p32fps_2k20fps;
        int tier = 4;

    } x265;
};


// VideoEncoder takes incoming video and synchronously encodes it.
class VideoEncoder : public VideoHandler {
public:
    VideoEncoder(Logger logger, EncodedVideoHandler* handler, VideoEncoderConfiguration configuration)
            : _logger{std::move(logger)}, _handler{handler}, _configuration{std::move(configuration)} {}

    virtual ~VideoEncoder() {};
    virtual void handleVideo(std::chrono::microseconds pts, const AVFrame* frame);
    void flush();

protected:
    const Logger _logger;
    EncodedVideoHandler* const _handler;
    const VideoEncoderConfiguration _configuration;

    std::vector<uint8_t> _outputBuffer;

    AVCodecContext* _context = nullptr;
    SwsContext* _scalingContext = nullptr;
    AVFrame* _scaledFrame = nullptr;

    virtual void _beginEncoding(int inputWidth, int inputHeight, AVPixelFormat inputPixelFormat) = 0;
    virtual void _endEncoding();
    virtual void _handleEncodedPackets() = 0;
};

// Encoding to h.264
class H264VideoEncoder : public VideoEncoder {
public:

    H264VideoEncoder(Logger logger, EncodedVideoHandler* handler, VideoEncoderConfiguration configuration)
        : VideoEncoder(std::move(logger), handler, std::move(configuration)) {}
    virtual ~H264VideoEncoder();

private:
    virtual void _beginEncoding(int inputWidth, int inputHeight, AVPixelFormat inputPixelFormat) override;
    virtual void _handleEncodedPackets() override;
};

// Encoding to h.265
class H265VideoEncoder : public VideoEncoder {
public:

    H265VideoEncoder(Logger logger, EncodedVideoHandler* handler, VideoEncoderConfiguration configuration)
        : VideoEncoder(std::move(logger), handler, std::move(configuration)) {}
    virtual ~H265VideoEncoder();

private:
    virtual void _beginEncoding(int inputWidth, int inputHeight, AVPixelFormat inputPixelFormat) override;
    virtual void _handleEncodedPackets() override;
};