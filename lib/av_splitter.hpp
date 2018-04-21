#pragma once

#include <vector>

#include "av_handler.hpp"

template <typename Vector>
void _avSplitterAppend(Vector& v) {}

template <typename Vector, typename Next, typename... Rem>
void _avSplitterAppend(Vector& v, Next&& next, Rem&&... rem) {
    v.emplace_back(std::forward<Next>(next));
    return _avSplitterAppend(v, std::forward<Rem>(rem)...);
}

class VideoSplitter : public VideoHandler {
public:
    template <typename... Args>
    explicit VideoSplitter(Args&&... args) {
        _avSplitterAppend(_handlers, std::forward<Args>(args)...);
    }

    virtual ~VideoSplitter() {}

    // addHandler adds a handler. It is not safe to call this while other threads may be invoking
    // the handle methods.
    void addHandler(VideoHandler* handler) {
        _handlers.emplace_back(handler);
    }

    virtual void handleVideo(std::chrono::microseconds pts, const AVFrame* frame) override {
        for (auto r : _handlers) {
            r->handleVideo(pts, frame);
        }
    }

private:
    std::vector<VideoHandler*> _handlers;
};
