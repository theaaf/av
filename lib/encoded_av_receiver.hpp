#pragma once

#include <chrono>
#include <cstddef>

// Simultaneous calls to EncodedAudioReceiver's methods are not allowed. Receivers are expected to
// return quickly from calls, performing expensive tasks asynchronously if needed.
struct EncodedAudioReceiver {
    virtual ~EncodedAudioReceiver() {}

    // receiveEncodedAudioConfig receives MPEG4 configuration data (see MPEG4AudioSpecificConfig).
    virtual void receiveEncodedAudioConfig(const void* data, size_t len) {}

    // receiveEncodedAudio receives raw audio packets.
    virtual void receiveEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) {}
};

// Simultaneous calls to EncodedVideoReceiver's methods are not allowed. Receivers are expected to
// return quickly from calls, performing expensive tasks asynchronously if needed.
struct EncodedVideoReceiver {
    virtual ~EncodedVideoReceiver() {}

    // receiveEncodedVideoConfig receives AVC configuration data (see AVCDecoderConfigurationRecord).
    virtual void receiveEncodedVideoConfig(const void* data, size_t len) {}

    // receiveEncodedVideo receives raw AVCC NALUs.
    virtual void receiveEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) {}
};

struct EncodedAVReceiver : EncodedAudioReceiver, EncodedVideoReceiver {
    virtual ~EncodedAVReceiver() {}
};
