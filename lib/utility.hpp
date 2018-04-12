#pragma once

#include <cstdint>
#include <vector>

template <typename T = uint8_t>
constexpr T HexToInt(char hex) {
    if (hex >= 'a' && hex <= 'f') {
        return 10 + (hex - 'a');
    } else if (hex >= 'A' && hex <= 'F') {
        return 10 + (hex - 'A');
    }
    return hex - '0';
}

template <size_t N>
std::vector<uint8_t> HexToVector(const char(&hex)[N]) {
    std::vector<uint8_t> ret;
    ret.resize((N - 1) / 2);
    for (size_t i = 0; i < ret.size(); ++i) {
        ret[i] = (HexToInt<uint8_t>(hex[i * 2]) << 4) | HexToInt<uint8_t>(hex[i * 2 + 1]);
    }
    return ret;
}
