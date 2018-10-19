#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <variant>

extern "C" {
    #include <libavformat/avformat.h>
}

#include <h26x/seq_parameter_set.hpp>

#include "encoded_av_handler.hpp"
#include "logger.hpp"
#include "mpeg4.hpp"
#include "segment_storage.hpp"

// Packager takes incoming audio and video, and synchronously muxes them into MPEG-TS segments.
class Packager : public EncodedAVHandler {
public:
    Packager(Logger logger, SegmentStorage* storage);
    virtual ~Packager();

    // beginNewSegment instructs the packager to begin a new segment at the next IDR.
    void beginNewSegment();

    virtual void handleEncodedAudioConfig(const void* data, size_t len) override;
    virtual void handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override;

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override = 0;
    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override = 0;
    virtual void handleEncodedVideoDiscontinuity() override;

protected:
    const Logger _logger;
    SegmentStorage* const _storage;

    std::mutex _mutex;

    bool _shouldBeginNewSegment = false;
    bool _shouldMarkNextSegmentDiscontinuous = true;

    std::vector<uint8_t> _inputBuffer;

    std::unique_ptr<MPEG4AudioSpecificConfig> _audioConfig;

    using UniqueAVCDecoderRecord = std::unique_ptr<AVCDecoderConfigurationRecord>;
    using UniqueHEVCDecoderRecord = std::unique_ptr<HEVCDecoderConfigurationRecord>;
    std::variant<std::nullptr_t, UniqueAVCDecoderRecord, UniqueHEVCDecoderRecord> _decoderRecord = nullptr;

    AVFormatContext* _outputContext = nullptr;
    void* _ioContextBuffer = nullptr;
    AVIOContext* _ioContext = nullptr;
    AVStream* _audioStream = nullptr;
    AVStream* _videoStream = nullptr;
    std::shared_ptr<SegmentStorage::Segment> _segment;

    std::chrono::microseconds _segmentPTS;
    std::chrono::microseconds _maxVideoPTS = std::chrono::microseconds::min();

    virtual void _beginSegment(std::chrono::microseconds pts) = 0;
    void _endSegment(bool writeTrailer = true);

    static int _writePacket(void* opaque, uint8_t* buf, int len);
};


class H264Packager : public Packager {
public:
    H264Packager(Logger logger, SegmentStorage* storage) : Packager(std::move(logger), storage) {}
    virtual ~H264Packager() {}

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override;
    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

protected:
    std::unique_ptr<h264::seq_parameter_set_rbsp> _videoConfigSPS;
    virtual void _beginSegment(std::chrono::microseconds pts) override;
};

class H265Packager : public Packager {
public:
    H265Packager(Logger logger, SegmentStorage* storage) : Packager(std::move(logger), storage) {}
    virtual ~H265Packager() {}

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override;
    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

protected:
    virtual void _beginSegment(std::chrono::microseconds pts) override;
};