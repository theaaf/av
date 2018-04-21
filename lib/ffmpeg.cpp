#include "ffmpeg.hpp"

#include <mutex>
#include <unordered_map>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/error.h>
}

#include "logger.hpp"

namespace {
    std::once_flag gInitFFmpegOnce;
}

void ffmpegLog(void* avcl, int level, const char* str) {
    std::string tag = "";

    Logger logger;

    AVClass* avc = avcl ? *reinterpret_cast<AVClass**>(avcl) : nullptr;
    if (avc) {
        tag = std::string("[") + avc->item_name(avcl) + "]";

        void* next = avcl;
        while (next) {
            if (GetLoggerForFFmpegLogContext(next, &logger)) {
                break;
            }
            AVClass* avc = next ? *reinterpret_cast<AVClass**>(next) : nullptr;
            if (!avc->parent_log_context_offset) {
                break;
            }
            next = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(next) + avc->parent_log_context_offset);
        }
    }

    switch (level) {
    case AV_LOG_PANIC:
    case AV_LOG_FATAL:
    case AV_LOG_ERROR :
        logger.error("[ffmpeg]{} {}", tag, str);
        break;
    case AV_LOG_WARNING:
        logger.warn("[ffmpeg]{} {}", tag, str);
        break;
    case AV_LOG_INFO:
        logger.info("[ffmpeg]{} {}", tag, str);
        break;
    }
}

void ffmpegLogCallback(void* avcl, int level, const char* format, va_list args) {
    if (level > AV_LOG_INFO) {
        return;
    }

    char buf[500];

    va_list tmp;
    va_copy(tmp, args);
    auto n = vsnprintf(buf, sizeof(buf), format, tmp);
    va_end(tmp);

    if (n < 0) {
        Logger{}.error("ffmpeg log error: {}", format);
    } else if (static_cast<size_t>(n) < sizeof(buf)) {
        while (buf[n] == '\0' || buf[n] == '\n') {
            buf[n--] = '\0';
        }
        ffmpegLog(avcl, level, buf);
    } else {
        auto buf = std::vector<char>(n+1);
        auto n = vsnprintf(&buf[0], buf.size(), format, args);
        while (buf[n] == '\0' || buf[n] == '\n') {
            buf[n--] = '\0';
        }
        ffmpegLog(avcl, level, buf.data());
    }
}

void InitFFmpeg() {
    std::call_once(gInitFFmpegOnce, [] {
        av_register_all();
        av_log_set_callback(&ffmpegLogCallback);
    });
}

std::string FFmpegErrorString(int errnum) {
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(buf, sizeof(buf), errnum);
    return buf;
}

std::mutex _ffmpegLogContextMutex;
std::unordered_map<const void*, Logger> _ffmpegLogContexts;

void RegisterFFmpegLogContext(const void* context, Logger logger) {
    std::lock_guard<std::mutex> l{_ffmpegLogContextMutex};
    _ffmpegLogContexts[context] = logger;
}

void UnregisterFFmpegLogContext(const void* context) {
    std::lock_guard<std::mutex> l{_ffmpegLogContextMutex};
    _ffmpegLogContexts.erase(context);
}

bool GetLoggerForFFmpegLogContext(const void* context, Logger* out) {
    std::lock_guard<std::mutex> l{_ffmpegLogContextMutex};
    auto it = _ffmpegLogContexts.find(context);
    if (it == _ffmpegLogContexts.end()) {
        return false;
    }
    *out = it->second;
    return true;
}
