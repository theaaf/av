#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "encoded_av_handler.hpp"
#include "logger.hpp"

// Demuxer opens a file and pushes its contents to an EncodedAVHandler.
class Demuxer {
public:
    Demuxer(const Logger& logger, const std::string& in, EncodedAVHandler* handler);
    ~Demuxer();

    bool isDone() const { return _isDone; }

    size_t totalFrameCount() const { return _totalFrameCount; }
    size_t framesDemuxed() const { return _framesDemuxed; }

private:
    std::thread _thread;
    std::atomic<bool> _isDone{false};
    std::atomic<size_t> _totalFrameCount{0};
    std::atomic<size_t> _framesDemuxed{0};

    void _run(Logger logger, const std::string& in, EncodedAVHandler* handler);
};
