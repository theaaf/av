#pragma once

#include <string>

namespace h264 {

struct error {
    std::string message;

    explicit operator bool() const {
        return !message.empty();
    }
};

} // namespace h264
