#pragma once

#include <thread>
#include <vector>

#include "encoded_av_receiver.hpp"
#include "logger.hpp"

enum class ArchiveDataType : uint8_t {
    AudioConfig,
    Audio,
    VideoConfig,
    Video,
};

class Archiver : public EncodedAVReceiver {
public:
    explicit Archiver(Logger logger);
    virtual ~Archiver();

    virtual void receiveEncodedAudioConfig(const void* data, size_t len) override;
    virtual void receiveEncodedAudio(const void* data, size_t len) override;
    virtual void receiveEncodedVideoConfig(const void* data, size_t len) override;
    virtual void receiveEncodedVideo(const void* data, size_t len) override;

private:
    const Logger _logger;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::vector<uint8_t> _buffer;
    bool _isDestructing = false;

    void _run();
    void _write(ArchiveDataType type, const void* data, size_t len);
};
