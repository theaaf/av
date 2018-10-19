#pragma once

#include "h264.hpp"

#include <vector>

namespace h264 {

    error decodeRBSP(bitstream* bs, size_t len, std::vector<uint8_t>& rbsp);

    // ITU-T H.264, 04/2017, 7.3.1
    struct nal_unit {
        f<1> forbidden_zero_bit;
        u<2> nal_ref_idc;
        u<5> nal_unit_type;
        std::vector <uint8_t> rbsp_byte;

        error decode(bitstream *bs, size_t NumBytesInNALunit);
    };
}

namespace h265 {

    using h264::f;
    using h264::u;
    using h264::bitstream;
    using h264::error;

    //  ITU-T H.265 v5 (02/2018), 7.3.1
    struct nal_unit {
        f<1> forbidden_zero_bit;
        u<6> nal_unit_type;
        u<6> nuh_layer_id;
        u<3> nuh_temporal_id_plus1;
        std::vector <uint8_t> rbsp_byte;

        error decode(bitstream *bs, size_t NumBytesInNALunit);
    };

}