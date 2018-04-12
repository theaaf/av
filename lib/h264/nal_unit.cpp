#include "nal_unit.hpp"

namespace h264 {

error nal_unit::decode(bitstream* bs, size_t NumBytesInNALunit) {
    auto err = bs->decode(
        &forbidden_zero_bit,
        &nal_ref_idc,
        &nal_unit_type
    );
    if (err) {
        return err;
    }

    if (forbidden_zero_bit != 0) {
        return {"non-zero forbidden_zero_bit"};
    }

    constexpr size_t nalUnitHeaderBytes = 1;
    size_t NumBytesInRBSP = 0;

    if (nal_unit_type == 14 || nal_unit_type == 20 || nal_unit_type == 21) {
        return {"unsupported nal_unit_type"};
    }

    rbsp_byte.clear();

    for (auto i = nalUnitHeaderBytes; i < NumBytesInNALunit; ++i) {
        if (i + 2 < NumBytesInNALunit && bs->next_bits(24) == 0x000003) {
            rbsp_byte.resize(NumBytesInRBSP + 2);
            rbsp_byte[NumBytesInRBSP++] = 0;
            rbsp_byte[NumBytesInRBSP++] = 0;
            i += 2;
            bs->advance_bits(24);
        } else {
            rbsp_byte.resize(NumBytesInRBSP + 1);
            rbsp_byte[NumBytesInRBSP++] = bs->next_bits(8);
            if (!bs->advance_bits(8)) {
                return {"not enough bytes"};
            }
        }
    }

    return {};
}

} // namespace h264
