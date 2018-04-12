#pragma once

#include <string>

// InitFFmpeg initializes global state used by the FFmpeg SDK and its dependencies. If you use the
// FFmpeg directly, call this function first.
void InitFFmpeg();

std::string FFmpegErrorString(int errnum);
