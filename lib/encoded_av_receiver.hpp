#pragma once

#include "mpeg4.hpp"

// Simultaneous calls to EncodedAudioReceiver's methods are not allowed.
struct EncodedAudioReceiver {
    virtual ~EncodedAudioReceiver() {}

    // receiveEncodedAudioConfig receives MPEG4 configuration data (see MPEG4AudioSpecificConfig).
    virtual void receiveEncodedAudioConfig(const void* data, size_t len) {}

    // receiveEncodedAudio receives raw audio packets.
    virtual void receiveEncodedAudio(const void* data, size_t len) {}
};

// Simultaneous calls to EncodedVideoReceiver's methods are not allowed.
struct EncodedVideoReceiver {
    virtual ~EncodedVideoReceiver() {}

    // receiveEncodedVideoConfig receives AVC configuration data (see AVCDecoderConfigurationRecord).
    virtual void receiveEncodedVideoConfig(const void* data, size_t len) {}

    // receiveEncodedVideo receives raw AVCC NALUs.
    virtual void receiveEncodedVideo(const void* data, size_t len) {}
};

struct EncodedAVReceiver : EncodedAudioReceiver, EncodedVideoReceiver {
    virtual ~EncodedAVReceiver() {}
};
