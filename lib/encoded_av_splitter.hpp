#pragma once

#include <vector>

#include "encoded_av_receiver.hpp"

template <typename Vector>
void _avSplitterAppend(Vector& v) {}

template <typename Vector, typename Next, typename... Rem>
void _avSplitterAppend(Vector& v, Next&& next, Rem&&... rem) {
    v.emplace_back(std::forward<Next>(next));
    return _avSplitterAppend(v, std::forward<Rem>(rem)...);
}

class EncodedAudioSplitter : public EncodedAudioReceiver {
public:
    template <typename... Args>
    explicit EncodedAudioSplitter(Args&&... args) {
        _avSplitterAppend(_receivers, std::forward<Args>(args)...);
    }

    virtual ~EncodedAudioSplitter() {}

    virtual void receiveEncodedAudioConfig(const void* data, size_t len) override {
        for (auto r : _receivers) {
            r->receiveEncodedAudioConfig(data, len);
        }
    }

    virtual void receiveEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override {
        for (auto r : _receivers) {
            r->receiveEncodedAudio(pts, data, len);
        }
    }

private:
    std::vector<EncodedAudioReceiver*> _receivers;
};

class EncodedVideoSplitter : public EncodedVideoReceiver {
public:
    template <typename... Args>
    explicit EncodedVideoSplitter(Args&&... args) {
        _avSplitterAppend(_receivers, std::forward<Args>(args)...);
    }

    virtual ~EncodedVideoSplitter() {}

    virtual void receiveEncodedVideoConfig(const void* data, size_t len) override {
        for (auto r : _receivers) {
            r->receiveEncodedVideoConfig(data, len);
        }
    }

    virtual void receiveEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        for (auto r : _receivers) {
            r->receiveEncodedVideo(pts, dts, data, len);
        }
    }

private:
    std::vector<EncodedVideoReceiver*> _receivers;
};

struct EncodedAVSplitter : EncodedAudioSplitter, EncodedVideoSplitter {
    virtual ~EncodedAVSplitter() {}
};
