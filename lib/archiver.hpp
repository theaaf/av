#pragma once

#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "encoded_av_receiver.hpp"
#include "file_storage.hpp"
#include "logger.hpp"

enum class ArchiveDataType : uint8_t {
    AudioConfig,
    Audio,
    VideoConfig,
    Video,
};

class Archiver : public EncodedAVReceiver {
public:
    // Creates an archiver that uploads to the specified storage with the specified path format. The
    // key format will be given an integer that will increment for each file uploaded. For example,
    // "my-stream/{}" will expand to "my-stream/0" for the first file.
    Archiver(Logger logger, FileStorage* storage, std::string pathFormat);
    virtual ~Archiver();

    virtual void receiveEncodedAudioConfig(const void* data, size_t len) override;
    virtual void receiveEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override;
    virtual void receiveEncodedVideoConfig(const void* data, size_t len) override;
    virtual void receiveEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

private:
    const Logger _logger;
    FileStorage* const _storage;
    const std::string _pathFormat;

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::vector<uint8_t> _buffer;
    bool _isDestructing = false;

    void _run();
    void _write(ArchiveDataType type, std::function<void(uint8_t*)> write, size_t len);
};
