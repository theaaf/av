#pragma once

#include <vector>

#include "h264.hpp"

namespace h264 {

// ITU-T H.264, 04/2017, 7.3.2.3.1
struct sei_message {
    unsigned int payloadType = 0;
    unsigned int payloadSize = 0;

    std::vector <uint8_t> sei_payload;

    error decode(bitstream* bs);
};

// ITU-T H.264, 04/2017, 7.3.2.3
struct sei_rbsp {
    std::vector<sei_message> sei_message;

    error decode(bitstream* bs);
};

} // namespace h264
