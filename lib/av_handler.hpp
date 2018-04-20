#pragma once

#include <chrono>

extern "C" {
    #include <libavutil/avutil.h>
}

struct VideoHandler {
    virtual ~VideoHandler() {}

    virtual void handleVideo(std::chrono::microseconds pts, const AVFrame* frame) {}
};
