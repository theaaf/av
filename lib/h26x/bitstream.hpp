#pragma once

#include <cstdint>
#include <utility>

#include "error.hpp"

namespace h264 {

class bitstream;

template <typename T>
error decode(bitstream* bs, T dest) {
    return dest->decode(bs);
}

template <typename Next, typename... Rem>
error decode(bitstream* bs, Next&& next, Rem&&... rem) {
    auto err = decode(bs, next);
    if (err) {
        return err;
    }
    return decode(bs, std::forward<Rem>(rem)...);
}

// bitstream represents an h.264 bitstream as defined by ITU-T H.264. It mimics the naming and
// conventions in the spec, which don't necessarily match the rest of our codebase.
class bitstream {
public:
    bitstream(const void* data, size_t len) : _data{reinterpret_cast<const uint8_t*>(data)}, _bit_count{len * 8} {}

    template <typename... Args>
    error decode(Args&&... args) {
        return ::h264::decode(this, std::forward<Args>(args)...);
    }

    bool advance_bits(size_t n) {
        if (bits_remaining() < n) {
            return false;
        }
        _bit_offset += n;
        return true;
    }

    uint64_t next_bits(size_t n) const {
        if (bits_remaining() < n) {
            return 0;
        }
        uint64_t ret = 0;
        for (size_t i = 0; i < n; ++i) {
            ret = (ret << 1) | ((_data[(_bit_offset + i) / 8] >> (8 - (_bit_offset + i) % 8 - 1)) & 1);
        }
        return ret;
    }

    template <typename T>
    bool read_bits(T* dest, size_t n) {
        if (bits_remaining() < n) {
            return false;
        }
        *dest = static_cast<T>(next_bits(n));
        _bit_offset += n;
        return true;
    }

    bool byte_aligned() const {
        return !(_bit_offset % 8);
    }

    bool read_byte_aligned_bytes(const void** dest, size_t n) {
        if (!byte_aligned() || bits_remaining() < n * 8) {
            return false;
        }
        *dest = _data + (_bit_offset / 8);
        _bit_offset += n * 8;
        return true;
    }

    size_t bits_remaining() const {
        return _bit_count - _bit_offset;
    }

private:
    const uint8_t* _data;
    size_t _bit_count;
    size_t _bit_offset = 0;
};

} // namespace h264
