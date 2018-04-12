#pragma once

#include "h264.hpp"

#include <vector>

namespace h264 {

// ITU-T H.264, 04/2017, 7.3.1
struct nal_unit {
    f<1> forbidden_zero_bit;
    u<2> nal_ref_idc;
    u<5> nal_unit_type;
    std::vector<uint8_t> rbsp_byte;

    error decode(bitstream* bs, size_t NumBytesInNALunit);
};

} // namespace h264
