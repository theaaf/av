#pragma once

#include <tuple>
#include <vector>

#include "encoded_av_receiver.hpp"

struct TestEncodedAVReceiver : EncodedAVReceiver {
    virtual ~TestEncodedAVReceiver() {}

    virtual void receiveEncodedAudioConfig(const void* data, size_t len) override {
        receivedAudioConfig.emplace_back(data, len);
    }

    virtual void receiveEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override {
        receivedAudio.emplace_back(pts, data, len);
    }

    virtual void receiveEncodedVideoConfig(const void* data, size_t len) override {
        receivedVideoConfig.emplace_back(data, len);
    }

    virtual void receiveEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        receivedVideo.emplace_back(pts, dts, data, len);
    }

    std::vector<std::tuple<const void*, size_t>> receivedAudioConfig;
    std::vector<std::tuple<std::chrono::microseconds, const void*, size_t>> receivedAudio;
    std::vector<std::tuple<const void*, size_t>> receivedVideoConfig;
    std::vector<std::tuple<std::chrono::microseconds, std::chrono::microseconds, const void*, size_t>> receivedVideo;
};

// ExerciseEncodedAVReceiver shotguns about 30s of actual video to the receiver.
void ExerciseEncodedAVReceiver(EncodedAVReceiver* receiver);
