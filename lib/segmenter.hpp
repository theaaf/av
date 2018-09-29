#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "encoded_av_handler.hpp"
#include "logger.hpp"
#include "mpeg4.hpp"

// Segmenter is responsible for determining segment boundaries.
class Segmenter : public EncodedAVHandler {
public:
    // Audio and video is synchronously forwarded to the given handler, and boundaryCallback is
    // invoked whenever a segment boundary should be made. boundaryCallback may want to, for
    // example, flush coders and invoke a Packager instance's beginNewSegment method.
    Segmenter(Logger logger, EncodedAVHandler* handler, std::function<void()> boundaryCallback = {})
        : _logger{std::move(logger)}, _handler{handler}, _boundaryCallback{std::move(boundaryCallback)} {}
    virtual ~Segmenter() {}

    virtual void handleEncodedAudioConfig(const void* data, size_t len) override;
    virtual void handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override;
    virtual void handleEncodedVideoConfig(const void* data, size_t len) override;
    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

    virtual void handleEncodedVideoDiscontinuity() override {
        _handler->handleEncodedVideoDiscontinuity();
    }

private:
    const Logger _logger;
    EncodedAVHandler* const _handler;
    std::function<void()> _boundaryCallback;
    std::mutex _mutex;

    bool _didStartFirstSegment = false;
    std::chrono::microseconds _currentSegmentPTS{std::chrono::microseconds::min()};

    std::vector<uint8_t> _audioConfig;
    std::vector<uint8_t> _videoConfig;
    std::unique_ptr<AVCDecoderConfigurationRecord> _videoConfigRecord;
};
