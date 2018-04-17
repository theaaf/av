#pragma once

#include <chrono>
#include <memory>
#include <mutex>

extern "C" {
    #include <libavformat/avformat.h>
}

#include <h264/seq_parameter_set.hpp>

#include "encoded_av_receiver.hpp"
#include "logger.hpp"
#include "mpeg4.hpp"
#include "segment_storage.hpp"

// Packager takes incoming audio and video, and synchronously muxes them into MPEG-TS segments.
class Packager : public EncodedAVReceiver {
public:
    Packager(Logger logger, SegmentStorage* storage);
    virtual ~Packager();

    virtual void receiveEncodedAudioConfig(const void* data, size_t len) override;
    virtual void receiveEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override;
    virtual void receiveEncodedVideoConfig(const void* data, size_t len) override;
    virtual void receiveEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override;

private:
    const Logger _logger;
    SegmentStorage* const _storage;

    std::mutex _mutex;

    std::vector<uint8_t> _inputBuffer;

    std::unique_ptr<MPEG4AudioSpecificConfig> _audioConfig;
    std::unique_ptr<AVCDecoderConfigurationRecord> _videoConfig;
    std::unique_ptr<h264::seq_parameter_set_rbsp> _videoConfigSPS;

    AVFormatContext* _outputContext = nullptr;
    void* _ioContextBuffer = nullptr;
    AVIOContext* _ioContext = nullptr;
    AVStream* _audioStream = nullptr;
    AVStream* _videoStream = nullptr;
    std::shared_ptr<SegmentStorage::Segment> _segment;

    std::chrono::microseconds _segmentPTS;
    std::chrono::microseconds _maxVideoPTS = std::chrono::microseconds::min();

    void _beginSegment(std::chrono::microseconds pts);
    void _endSegment(bool writeTrailer = true);

    static int _writePacket(void* opaque, uint8_t* buf, int len);
};
