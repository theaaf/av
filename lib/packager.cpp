#include "packager.hpp"

#include <cstring>

#include "ffmpeg.hpp"

Packager::Packager(Logger logger, SegmentStorage* storage)
    : _logger{std::move(logger)}, _storage{storage}
{
    constexpr size_t kBufferSize = 4 * 1024;
    _ioContextBuffer = av_malloc(kBufferSize);
    _ioContext = avio_alloc_context(reinterpret_cast<unsigned char*>(_ioContextBuffer), kBufferSize, 1, this, nullptr, &_writePacket, nullptr);
}

void Packager::handleEncodedVideoDiscontinuity() {
    std::lock_guard<std::mutex> l{_mutex};
    _endSegment();
    _decoderRecord = nullptr;
    _audioConfig = nullptr;
    _shouldMarkNextSegmentDiscontinuous = true;
    _shouldBeginNewSegment = true;
}

Packager::~Packager() {
    _endSegment();
    av_free(_ioContext);
    av_free(_ioContextBuffer);
}

void Packager::beginNewSegment() {
    std::lock_guard<std::mutex> l{_mutex};
    _shouldBeginNewSegment = true;
}

void Packager::_endSegment(bool writeTrailer, std::chrono::microseconds nextSegmentPTS) {
    if (_outputContext) {
        if (writeTrailer) {
            av_write_trailer(_outputContext);
        }
        avformat_free_context(_outputContext);
        _outputContext = nullptr;
        _audioStream = nullptr;
        _videoStream = nullptr;
    }

    if (_segment) {
        std::chrono::microseconds duration;

        if (nextSegmentPTS != std::chrono::microseconds::zero()) {
            auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(nextSegmentPTS - _maxVideoPTS);
            if (gap.count() > 100) {
                _logger.with("_nextSegmentPTS - _maxVideoPTS", gap.count()).warn("encoder might not be keeping up; duration may not be accurate");
            }
            duration = nextSegmentPTS - _segmentPTS;
        } else {
            duration = _maxVideoPTS - _lastMaxVideoPTS;
        }

        _logger.with("duration_ms", std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()).info("closing segment");
        if (!_segment->close(duration)) {
            _logger.error("unable to close segment");
        }
        _segment = nullptr;
    }
    _lastMaxVideoPTS = _maxVideoPTS;
}

void Packager::handleEncodedAudioConfig(const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};

    auto config = std::make_unique<MPEG4AudioSpecificConfig>();
    if (!config->decode(data, len)) {
        _logger.error("unable to decode audio config");
        _endSegment();
        return;
    }
    if (_audioConfig && *config == *_audioConfig) {
        return;
    }

    _audioConfig = std::move(config);
}

void Packager::handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) {
    std::lock_guard<std::mutex> l{_mutex};
    if (!_audioConfig || !_audioStream) {
        return;
    }

    auto adts = _audioConfig->adtsHeader(len);
    _inputBuffer.resize(adts.size() + len);
    std::memcpy(&_inputBuffer[0], adts.data(), adts.size());
    std::memcpy(&_inputBuffer[adts.size()], data, len);

    AVPacket packet{};
    av_init_packet(&packet);
    packet.data = _inputBuffer.data();
    packet.size = _inputBuffer.size();
    packet.stream_index = _audioStream->index;
    packet.pts = pts.count() / 1000000.0 / av_q2d(_audioStream->time_base);
    packet.dts = packet.pts;

    auto err = av_interleaved_write_frame(_outputContext, &packet);
    if (err != 0) {
        _logger.error("error writing audio frame: {}", FFmpegErrorString(err));
    }
}

int Packager::_writePacket(void* opaque, uint8_t* buf, int len) {
    auto self = reinterpret_cast<Packager*>(opaque);
    if (self->_segment) {
        if (!self->_segment->write(buf, len)) {
            self->_logger.error("unable to write to segment");
            return AVERROR_UNKNOWN;
        }
    }
    return len;
}
