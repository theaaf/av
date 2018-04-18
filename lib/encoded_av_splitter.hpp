#pragma once

#include <vector>

#include "encoded_av_handler.hpp"

template <typename Vector>
void _avSplitterAppend(Vector& v) {}

template <typename Vector, typename Next, typename... Rem>
void _avSplitterAppend(Vector& v, Next&& next, Rem&&... rem) {
    v.emplace_back(std::forward<Next>(next));
    return _avSplitterAppend(v, std::forward<Rem>(rem)...);
}

class EncodedAudioSplitter : public EncodedAudioHandler {
public:
    template <typename... Args>
    explicit EncodedAudioSplitter(Args&&... args) {
        _avSplitterAppend(_handlers, std::forward<Args>(args)...);
    }

    virtual ~EncodedAudioSplitter() {}

    // addHandler adds a handler. It is not safe to call this while other threads may be invoking
    // the handle methods.
    void addHandler(EncodedAudioHandler* handler) {
        _handlers.emplace_back(handler);
    }

    virtual void handleEncodedAudioConfig(const void* data, size_t len) override {
        for (auto r : _handlers) {
            r->handleEncodedAudioConfig(data, len);
        }
    }

    virtual void handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override {
        for (auto r : _handlers) {
            r->handleEncodedAudio(pts, data, len);
        }
    }

private:
    std::vector<EncodedAudioHandler*> _handlers;
};

class EncodedVideoSplitter : public EncodedVideoHandler {
public:
    template <typename... Args>
    explicit EncodedVideoSplitter(Args&&... args) {
        _avSplitterAppend(_handlers, std::forward<Args>(args)...);
    }

    virtual ~EncodedVideoSplitter() {}

    // addHandler adds a handler. It is not safe to call this while other threads may be invoking
    // the handle methods.
    void addHandler(EncodedVideoHandler* handler) {
        _handlers.emplace_back(handler);
    }

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
        for (auto r : _handlers) {
            r->handleEncodedVideoConfig(data, len);
        }
    }

    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        for (auto r : _handlers) {
            r->handleEncodedVideo(pts, dts, data, len);
        }
    }

private:
    std::vector<EncodedVideoHandler*> _handlers;
};

struct EncodedAVSplitter : EncodedAudioSplitter, EncodedVideoSplitter, EncodedAVHandler {
    template <typename... Args>
    explicit EncodedAVSplitter(Args... args) : EncodedAudioSplitter{args...}, EncodedVideoSplitter{args...} {}
    virtual ~EncodedAVSplitter() {}

    virtual void handleEncodedAudioConfig(const void* data, size_t len) override {
        EncodedAudioSplitter::handleEncodedAudioConfig(data, len);
    }

    virtual void handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override {
        EncodedAudioSplitter::handleEncodedAudio(pts, data, len);
    }

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
        EncodedVideoSplitter::handleEncodedVideoConfig(data, len);
    }

    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        EncodedVideoSplitter::handleEncodedVideo(pts, dts, data, len);
    }

    using EncodedAudioSplitter::addHandler;
    using EncodedVideoSplitter::addHandler;

    // addHandler adds a handler. It is not safe to call this while other threads may be invoking
    // the handle methods.
    void addHandler(EncodedAVHandler* handler) {
        EncodedAudioSplitter::addHandler(handler);
        EncodedVideoSplitter::addHandler(handler);
    }
};
