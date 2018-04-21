#pragma once

#include <string>

#include "logger.hpp"

// InitFFmpeg initializes global state used by the FFmpeg SDK and its dependencies. If you use the
// FFmpeg directly, call this function first.
void InitFFmpeg();

std::string FFmpegErrorString(int errnum);

// FFmpeg log messages are accompanied by pointers to related context objects such as
// AVCodecContexts. To send FFmpeg log messages to the appropriate loggers, you can register your
// contexts with these functions.
void RegisterFFmpegLogContext(const void* context, Logger logger);
void UnregisterFFmpegLogContext(const void* context);
bool GetLoggerForFFmpegLogContext(const void* context, Logger* out);
