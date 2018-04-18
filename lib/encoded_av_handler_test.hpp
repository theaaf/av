#pragma once

#include <tuple>
#include <vector>

#include "encoded_av_handler.hpp"

struct TestEncodedAVHandler : EncodedAVHandler {
    virtual ~TestEncodedAVHandler() {}

    virtual void handleEncodedAudioConfig(const void* data, size_t len) override {
        handledAudioConfig.emplace_back(data, len);
    }

    virtual void handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override {
        handledAudio.emplace_back(pts, data, len);
    }

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
        handledVideoConfig.emplace_back(data, len);
    }

    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        handledVideo.emplace_back(pts, dts, data, len);
    }

    std::vector<std::tuple<const void*, size_t>> handledAudioConfig;
    std::vector<std::tuple<std::chrono::microseconds, const void*, size_t>> handledAudio;
    std::vector<std::tuple<const void*, size_t>> handledVideoConfig;
    std::vector<std::tuple<std::chrono::microseconds, std::chrono::microseconds, const void*, size_t>> handledVideo;
};

// ExerciseEncodedAVHandler shotguns about 30s of actual video to the handler.
void ExerciseEncodedAVHandler(EncodedAVHandler* handler);
