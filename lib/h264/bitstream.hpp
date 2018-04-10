#pragma once

#include <type_traits>

namespace h264 {

// bitstream represents an h.264 bitstream as defined by ITU-T H.264. It mimics the naming and
// conventions in the spec, which don't necessarily match the rest of our codebase.
class bitstream {
public:
    bitstream(const void* data, size_t len) : _data{reinterpret_cast<const uint8_t*>(data)}, _bit_count{len * 8} {}

    template <typename T>
    auto next_bits(T* dest, size_t n) -> std::enable_if_t<!std::is_enum<T>::value, bool> const {
        *dest = 0;
        if (bits_remaining() < n) {
            return false;
        }
        for (size_t i = 0; i < n; ++i) {
            *dest = (*dest << 1) | ((_data[(_bit_offset + i) / 8] >> (8 - (_bit_offset + i) % 8 - 1)) & 1);
        }
        return true;
    }

    template <typename T>
    auto next_bits(T* dest, size_t n) -> std::enable_if_t<std::is_enum<T>::value, bool> const {
        return next_bits(reinterpret_cast<std::underlying_type_t<T>*>(dest), n);
    }

    template <typename T>
    bool read_bits(T* dest, size_t n) {
        if (next_bits(dest, n)) {
            _bit_offset += n;
            return true;
        }
        return false;
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
    const size_t _bit_count;
    size_t _bit_offset = 0;
};

} // namespace h264
