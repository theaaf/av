#include "ffmpeg.hpp"

#include <mutex>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/error.h>
}

namespace {
    std::once_flag gInitFFmpegOnce;
}

void InitFFmpeg() {
    std::call_once(gInitFFmpegOnce, [] {
        av_register_all();
    });
}

std::string FFmpegErrorString(int errnum) {
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(buf, sizeof(buf), errnum);
    return buf;
}
