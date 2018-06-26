#include "h264.hpp"

#include <cstring>

namespace h264 {

bool IterateAnnexB(const void* data, size_t len, const std::function<void(const void* data, size_t len)>& f) {
    auto ptr = reinterpret_cast<const uint8_t*>(data);

    while (true) {
        while (true) {
            if (!len) {
                return true;
            } else if (ptr[0] != 0) {
                return false;
            } else if (len >= 3 && ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1) {
                break;
            }
            ++ptr;
            --len;
        }

        ptr += 3;
        len -= 3;

        auto nalu = ptr;

        while (true) {
            if (!len || (len >= 3 && ptr[0] == 0 && ptr[1] == 0 && ptr[2] <= 1)) {
                f(nalu, ptr - nalu);
                break;
            }
            ++ptr;
            --len;
        }
    }
}

bool IterateAVCC(const void* data, size_t len, size_t naluSizeLength, const std::function<void(const void* data, size_t len)>& f) {
    if (naluSizeLength > 8 || naluSizeLength < 1) {
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

bool FilterAVCC(std::vector<uint8_t>* dest, const void* data, size_t len, size_t naluSizeLength, std::function<bool(unsigned int)> filter) {
    if (naluSizeLength > 8 || naluSizeLength < 1) {
        return false;
    }

    auto ptr = reinterpret_cast<const uint8_t*>(data);
    while (len >= naluSizeLength) {
        size_t naluSize = 0;
        for (size_t i = 0; i < naluSizeLength; ++i) {
            naluSize = (naluSize << 8) | ptr[i];
        }

        const auto copyLen = naluSizeLength + naluSize;
        if (len < copyLen) {
            return false;
        }

        dest->resize(dest->size() + copyLen);
        auto destPtr = &(*dest)[dest->size() - copyLen];
        std::memcpy(destPtr, ptr, copyLen);

        ptr += copyLen;
        len -= copyLen;
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

bool AnnexBToAVCC(std::vector<uint8_t>* dest, const void* data, size_t len, std::function<bool(unsigned int)> filter) {
    return IterateAnnexB(data, len, [&](const void* data, size_t len) {
        if (filter && !filter(*reinterpret_cast<const uint8_t*>(data) & 0x1f)) {
            return;
        }
        dest->resize(dest->size() + 4 + len);
        auto ptr = &(*dest)[dest->size() - 4 - len];
        *(ptr++) = len >> 24;
        *(ptr++) = len >> 16;
        *(ptr++) = len >> 8;
        *(ptr++) = len;
        std::memcpy(ptr, data, len);
    });
}

} // namespace h264
