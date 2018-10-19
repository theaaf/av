#include "segmenter.hpp"

#include <h26x/h264.hpp>

void Segmenter::handleEncodedAudioConfig(const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    _audioConfig.clear();
    _audioConfig.insert(_audioConfig.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + len);

    if (_didStartFirstSegment) {
        _handler->handleEncodedAudioConfig(data, len);
    }
}

void Segmenter::handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    if (_didStartFirstSegment) {
        _handler->handleEncodedAudio(pts, data, len);
    }
}

void Segmenter::handleEncodedVideoConfig(const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    _videoConfig.clear();
    _videoConfig.insert(_videoConfig.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + len);

    auto config = std::make_unique<AVCDecoderConfigurationRecord>();
    if (!config->decode(data, len)) {
        _logger.error("unable to decode video config");
    } else {
        _videoConfigRecord = std::move(config);
    }

    if (_didStartFirstSegment) {
        _handler->handleEncodedVideoConfig(data, len);
    }
}

void Segmenter::handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    if (!_videoConfigRecord) {
        return;
    }

    auto isIDR = false;
    if (!h264::IterateAVCC(data, len, _videoConfigRecord->lengthSizeMinusOne + 1, [&](const void* data, size_t len) {
        if (len < 1) {
            return;
        }
        auto naluType = *reinterpret_cast<const uint8_t*>(data) & 0x1f;
        isIDR |= (naluType == h264::NALUnitType::IDRSlice);
    })) {
        _logger.error("unable to iterate avcc");
        return;
    }

    if (isIDR && _currentSegmentPTS + std::chrono::seconds(5) < pts) {
        _didStartFirstSegment = true;
        _currentSegmentPTS = pts;
        if (_boundaryCallback) {
            _boundaryCallback();
        }
        if (!_audioConfig.empty()) {
            _handler->handleEncodedAudioConfig(_audioConfig.data(), _audioConfig.size());
        }
        _handler->handleEncodedVideoConfig(_videoConfig.data(), _videoConfig.size());
    }

    if (_didStartFirstSegment) {
        _handler->handleEncodedVideo(pts, dts, data, len);
    }
}
