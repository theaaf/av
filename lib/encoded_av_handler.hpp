#pragma once

#include <chrono>
#include <cstddef>

// Simultaneous calls to EncodedAudioHandler's methods are not allowed. Handlers are expected to
// return quickly from calls, performing expensive tasks asynchronously if needed.
struct EncodedAudioHandler {
    virtual ~EncodedAudioHandler() {}

    // handleEncodedAudioConfig handles MPEG4 configuration data (see MPEG4AudioSpecificConfig).
    virtual void handleEncodedAudioConfig(const void* data, size_t len) {}

    // handleEncodedAudio handles raw audio packets.
    virtual void handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) {}
};

// Simultaneous calls to EncodedVideoHandler's methods are not allowed. Handlers are expected to
// return quickly from calls, performing expensive tasks asynchronously if needed.
struct EncodedVideoHandler {
    virtual ~EncodedVideoHandler() {}

    // handleEncodedVideoConfig handles AVC configuration data (see AVCDecoderConfigurationRecord).
    virtual void handleEncodedVideoConfig(const void* data, size_t len) {}

    // handleEncodedVideo handles raw AVCC NALUs.
    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) {}
};

struct EncodedAVHandler : EncodedAudioHandler, EncodedVideoHandler {
    virtual ~EncodedAVHandler() {}
};
