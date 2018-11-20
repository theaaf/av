#include "archiver.hpp"

Archiver::Archiver(Logger logger, FileStorage* storage, std::string pathFormat)
    : _logger{std::move(logger)}, _storage{storage}, _pathFormat{std::move(pathFormat)}
{
    _thread = std::thread([this] {
        _run();
    });
}

Archiver::~Archiver() {
    {
        std::lock_guard<std::mutex> l{_mutex};
        _isDestructing = true;
    }
    _cv.notify_one();
    _thread.join();
}

void Archiver::handleEncodedAudioConfig(const void* data, size_t len) {
    _write(ArchiveDataType::AudioConfig, [&](uint8_t* dest) {
        std::memcpy(dest, data, len);
    }, len);
}

void Archiver::handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) {
    _write(ArchiveDataType::Audio, [&](uint8_t* dest) {
        for (int i = 0; i < 8; ++i) {
            *(dest++) = (pts.count() >> ((7 - i) * 8)) & 0xff;
        }
        std::memcpy(dest, data, len);
    }, len + 8);
}

void Archiver::handleEncodedVideoConfig(const void* data, size_t len) {
    _write(ArchiveDataType::VideoConfig, [&](uint8_t* dest) {
        std::memcpy(dest, data, len);
    }, len);
}

void Archiver::handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) {
    _write(ArchiveDataType::Video, [&](uint8_t* dest) {
        for (int i = 0; i < 8; ++i) {
            *(dest++) = (pts.count() >> ((7 - i) * 8)) & 0xff;
        }
        for (int i = 0; i < 8; ++i) {
            *(dest++) = (dts.count() >> ((7 - i) * 8)) & 0xff;
        }
        std::memcpy(dest, data, len);
    }, len + 16);
}

void Archiver::_run() {
    _logger.info("archiver thread running");

    std::vector<uint8_t> uploadBuffer;

    std::shared_ptr<FileStorage::File> file;
    std::chrono::steady_clock::time_point fileTime;
    size_t uploadCount = 0;

    std::unique_lock<std::mutex> l{_mutex};
    while (true) {
        while (!_isDestructing && _buffer.empty()) {
            _cv.wait(l);
        }

        uploadBuffer.clear();
        uploadBuffer.swap(_buffer);
        l.unlock();

        auto now = std::chrono::steady_clock::now();

        if (file && (uploadBuffer.empty() || now - fileTime > std::chrono::minutes(5))) {
            _logger.info("closing archive file");
            file->close();
            file = nullptr;
        }

        if (uploadBuffer.empty()) {
            break;
        }

        if (!file) {
            auto path = fmt::format(_pathFormat, uploadCount++);
            _logger.with("path", path).info("creating new archive file");
            file = _storage->createFile(path);
            fileTime = now;
        }

        if (file) {
            if (!file->write(uploadBuffer.data(), uploadBuffer.size())) {
                file->close();
                file = nullptr;
            }
        }

        l.lock();
    }

    _logger.info("archiver thread exiting");
}

void Archiver::_write(ArchiveDataType type, const std::function<void(uint8_t*)>& write, size_t len) {
    auto steadyTime = std::chrono::steady_clock::now();
    auto systemTime = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> l{_mutex};
        auto offset = _buffer.size();
        auto totalLen = 1 + 16 + len;
        _buffer.resize(offset + 4 + totalLen);

        for (int i = 0; i < 4; ++i) {
            _buffer[offset++] = (totalLen >> ((3 - i) * 8)) & 0xff;
        }

        _buffer[offset++] = static_cast<uint8_t>(type);

        auto steadyNano = std::chrono::duration_cast<std::chrono::nanoseconds>(steadyTime.time_since_epoch()).count();
        for (int i = 0; i < 8; ++i) {
            _buffer[offset++] = (steadyNano >> ((7 - i) * 8)) & 0xff;
        }

        auto systemNano = std::chrono::duration_cast<std::chrono::nanoseconds>(systemTime.time_since_epoch()).count();
        for (int i = 0; i < 8; ++i) {
            _buffer[offset++] = (systemNano >> ((7 - i) * 8)) & 0xff;
        }

        write(&_buffer[offset]);
    }

    _cv.notify_one();
}
