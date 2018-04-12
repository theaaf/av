#include "h264.hpp"

#include <cstring>

namespace h264 {

bool IterateAVCC(const void* data, size_t len, size_t naluSizeLength, const std::function<void(const void* data, size_t len)>& f) {
    if (naluSizeLength > 8) {
        return false;
    }

    auto ptr = reinterpret_cast<const uint8_t*>(data);
    while (len >= naluSizeLength) {
        size_t naluSize = 0;
        for (size_t i = 0; i < naluSizeLength; ++i) {
            naluSize = (naluSize << 8) | ptr[i];
        }
        ptr += naluSizeLength;
        len -= naluSizeLength;

        if (len < naluSize) {
            return false;
        }

        f(ptr, naluSize);
        ptr += naluSize;
        len -= naluSize;
    }

    return true;
}

bool AVCCToAnnexB(std::vector<uint8_t>* dest, const void* data, size_t len, size_t naluSizeLength) {
    return IterateAVCC(data, len, naluSizeLength, [&](const void* data, size_t len) {
        dest->resize(dest->size() + 3 + len);
        auto ptr = &(*dest)[dest->size() - 3 - len];
        *(ptr++) = 0;
        *(ptr++) = 0;
        *(ptr++) = 1;
        std::memcpy(ptr, data, len);
    });
}

} // namespace h264
