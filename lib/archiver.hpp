#pragma once

#include <functional>
#include <string>
#include <thread>
#include <vector>

#include <aws/s3/S3Client.h>

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
    // Creates an archiver that uploads to the specified bucket with the specified key format. The
    // key format will be given an integer that will increment for each file uploaded. For example,
    // "my-stream/{}" will expand to "my-stream/0" for the first file.
    Archiver(Logger logger, std::string bucket, std::string keyFormat);
    virtual ~Archiver();

    // Overrides the default S3 client (e.g. for testing). This must be called before any receiver
    // methods are invoked. Make sure you call InitAWS before creating the client.
    void overrideS3Client(std::shared_ptr<Aws::S3::S3Client> client) {
        _s3Client = client;
    }

    virtual void receiveEncodedAudioConfig(const void* data, size_t len) override;
    virtual void receiveEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override;
    virtual void receiveEncodedVideoConfig(const void* data, size_t len) override;
    virtual void receiveEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

private:
    const Logger _logger;
    const std::string _bucket;
    const std::string _keyFormat;

    std::shared_ptr<Aws::S3::S3Client> _s3Client;

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::vector<uint8_t> _buffer;
    bool _isDestructing = false;

    void _run();
    void _write(ArchiveDataType type, std::function<void(uint8_t*)> write, size_t len);
};
