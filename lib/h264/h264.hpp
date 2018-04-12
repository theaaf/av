#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "bitstream.hpp"

namespace h264 {

namespace ProfileIDC {
    constexpr unsigned int Baseline = 66;
    constexpr unsigned int Main = 77;
    constexpr unsigned int Extended = 88;
};

namespace NALUnitType {
    constexpr unsigned int IDRSlice = 5;
    constexpr unsigned int SequenceParameterSet = 7;
}

// ITU-T H.264, 04/2017, 7.2
template <size_t Bits>
struct u {
    uint64_t value = 0;

    constexpr operator uint64_t() const {
        return value;
    }

    error decode(bitstream* bs) {
        if (!bs->read_bits(&value, Bits)) {
            return {"unable to decode u<>"};
        }
        return {};
    }
};

// ITU-T H.264, 04/2017, 7.2
template <size_t Bits>
using f = u<Bits>;

// ITU-T H.264, 04/2017, 7.2 / 9.1
struct ue {
    uint64_t codeNum = 0;

    constexpr operator uint64_t() const {
        return codeNum;
    }

    error decode(bitstream* bs) {
        int leadingZeroBits = -1;
        for (int b = 0; !b; ++leadingZeroBits) {
            if (!bs->read_bits(&b, 1)) {
                return {"unable to decode ue: no non-zero bits"};
            }
        }
        if (!bs->read_bits(&codeNum, leadingZeroBits)) {
            return {"unable to decode ue: not enough bits"};
        }
        codeNum += (1 << leadingZeroBits) - 1;
        return {};
    }
};

// ITU-T H.264, 04/2017, 7.2 / 9.1 / 9.1.1
struct se {
    int64_t value = 0;

    constexpr operator int64_t() const {
        return value;
    }

    error decode(bitstream* bs) {
        ue n;
        auto err = n.decode(bs);
        if (err) {
            return err;
        }
        value = (n.codeNum + 1) >> 1;
        if (!(n.codeNum & 1)) {
            value = -value;
        }
        return {};
    }
};

bool IterateAVCC(const void* data, size_t len, size_t naluSizeLength, const std::function<void(const void* data, size_t len)>& f);

bool AVCCToAnnexB(std::vector<uint8_t>* dest, const void* data, size_t len, size_t naluSizeLength);

} // namespace h264
