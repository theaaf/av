#include "mpeg4.hpp"

#include <h264/bitstream.hpp>

std::ostream& operator<<(std::ostream& os, const MPEG4AudioObjectType& ot) {
    return os << static_cast<std::underlying_type_t<MPEG4AudioObjectType>>(ot);
}

std::ostream& operator<<(std::ostream& os, const MPEG4ChannelConfiguration& cc) {
    return os << static_cast<std::underlying_type_t<MPEG4ChannelConfiguration>>(cc);
}

std::vector<uint8_t> MPEG4AudioSpecificConfig::adtsHeader(size_t aacLength) const {
    const auto frameLength = 7 + aacLength;
    std::vector<uint8_t> ret(7);
    ret[0] = 0xff /* syncword */;
    ret[1] = 0xf0 /* syncword */ | 1 /* protection absent */;
    ret[2] = (static_cast<unsigned int>(objectType) - 1) << 6;
    ret[2] |= static_cast<unsigned int>(frequencyIndex()) << 2;
    ret[2] |= static_cast<unsigned int>(channelConfiguration) >> 2;
    ret[3] = static_cast<unsigned int>(channelConfiguration) << 6;
    ret[3] |= frameLength >> 11;
    ret[4] = (frameLength >> 3);
    ret[5] = (frameLength << 5) | 0x1f /* buffer fullness */;
    ret[6] = 0xfc /* buffer fullness */ | 0 /* raw data blocks */;
    return ret;
}

MPEG4SamplingFrequency MPEG4AudioSpecificConfig::frequencyIndex() const {
    switch (frequency) {
        case 96000: return MPEG4SamplingFrequency::F96000Hz;
        case 88200: return MPEG4SamplingFrequency::F88200Hz;
        case 64000: return MPEG4SamplingFrequency::F64000Hz;
        case 48000: return MPEG4SamplingFrequency::F48000Hz;
        case 44100: return MPEG4SamplingFrequency::F44100Hz;
        case 32000: return MPEG4SamplingFrequency::F32000Hz;
        case 24000: return MPEG4SamplingFrequency::F24000Hz;
        case 22050: return MPEG4SamplingFrequency::F22050Hz;
        case 16000: return MPEG4SamplingFrequency::F16000Hz;
        case 12000: return MPEG4SamplingFrequency::F12000Hz;
        case 11025: return MPEG4SamplingFrequency::F11025Hz;
        case 8000: return MPEG4SamplingFrequency::F8000Hz;
        case 7350: return MPEG4SamplingFrequency::F7350Hz;
    }
    return MPEG4SamplingFrequency::Explicit;
}

bool MPEG4AudioSpecificConfig::decode(const void* data, size_t len) {
    h264::bitstream b{data, len};
    if (!b.read_bits(&objectType, 5)) {
        return false;
    }

    if (objectType == MPEG4AudioObjectType::Escape) {
        if (!b.read_bits(&objectType, 6)) {
            return false;
        }
        objectType = static_cast<MPEG4AudioObjectType>(static_cast<int>(objectType) + 32);
    }

    MPEG4SamplingFrequency frequencyIndex;
    if (!b.read_bits(&frequencyIndex, 4)) {
        return false;
    }

    switch (frequencyIndex) {
    case MPEG4SamplingFrequency::F96000Hz:
        frequency = 96000;
        break;
    case MPEG4SamplingFrequency::F88200Hz:
        frequency = 88200;
        break;
    case MPEG4SamplingFrequency::F64000Hz:
        frequency = 64000;
        break;
    case MPEG4SamplingFrequency::F48000Hz:
        frequency = 48000;
        break;
    case MPEG4SamplingFrequency::F44100Hz:
        frequency = 44100;
        break;
    case MPEG4SamplingFrequency::F32000Hz:
        frequency = 32000;
        break;
    case MPEG4SamplingFrequency::F24000Hz:
        frequency = 24000;
        break;
    case MPEG4SamplingFrequency::F22050Hz:
        frequency = 22050;
        break;
    case MPEG4SamplingFrequency::F16000Hz:
        frequency = 16000;
        break;
    case MPEG4SamplingFrequency::F12000Hz:
        frequency = 12000;
        break;
    case MPEG4SamplingFrequency::F11025Hz:
        frequency = 11025;
        break;
    case MPEG4SamplingFrequency::F8000Hz:
        frequency = 8000;
        break;
    case MPEG4SamplingFrequency::F7350Hz:
        frequency = 7350;
        break;
    case MPEG4SamplingFrequency::Explicit:
        if (!b.read_bits(&frequency, 24)) {
            return false;
        }
        break;
    default:
        return false;
    }

    return b.read_bits(&channelConfiguration, 4);
}

std::vector<uint8_t> MPEG4AudioSpecificConfig::encode() const {
    std::vector<uint8_t> ret;
    auto fi = static_cast<unsigned int>(frequencyIndex());
    auto obj = static_cast<unsigned int>(objectType);
    auto ch = static_cast<unsigned int>(channelConfiguration);
    if (objectType > MPEG4AudioObjectType::Escape && frequencyIndex() == MPEG4SamplingFrequency::Explicit) {
        // 11111OOO OOO1111F FFFFFFFF FFFFFFFF FFFFFFFC CCC00000
        ret.emplace_back(0xf8 | (obj >> 3));
        ret.emplace_back((obj << 5) | 0x1e | (frequency >> 23));
        ret.emplace_back(frequency >> 15);
        ret.emplace_back(frequency >> 7 | (ch >> 3));
        ret.emplace_back(ch << 5);
    } else if (objectType > MPEG4AudioObjectType::Escape) {
        // 11111OOO OOOFFFFC CCC00000
        ret.emplace_back(0xf8 | (obj >> 3));
        ret.emplace_back((obj << 5) | (fi << 1) | (ch >> 3));
        ret.emplace_back(ch << 5);
    } else if (frequencyIndex() == MPEG4SamplingFrequency::Explicit) {
        // OOOOO111 1FFFFFFF FFFFFFFF FFFFFFFF FCCCC000
        ret.emplace_back((obj << 3) | 7);
        ret.emplace_back(0x80 | (frequency >> 17));
        ret.emplace_back(frequency >> 9);
        ret.emplace_back(frequency >> 1);
        ret.emplace_back((frequency << 7) | (ch << 3));
    } else {
        // OOOOOFFF FCCCC000
        ret.emplace_back((obj << 3) | (fi >> 1));
        ret.emplace_back((fi << 7) | (ch << 3));
    }
    return ret;
}

bool AVCDecoderConfigurationRecord::operator==(const AVCDecoderConfigurationRecord& other) const {
    return true
        && configurationVersion == other.configurationVersion
        && avcProfileIndication == other.avcProfileIndication
        && profileCompatibility == other.profileCompatibility
        && avcLevelIndication == other.avcLevelIndication
        && lengthSizeMinusOne == other.lengthSizeMinusOne
        && sequenceParameterSets == other.sequenceParameterSets
        && pictureParameterSets == other.pictureParameterSets
    ;
}

bool AVCDecoderConfigurationRecord::decode(const void* data, size_t len) {
    h264::bitstream b{data, len};

    int reserved;
    size_t numOfSequenceParameterSets;

    if (
        !b.read_bits(&configurationVersion, 8) || configurationVersion != 1 ||
        !b.read_bits(&avcProfileIndication, 8) ||
        !b.read_bits(&profileCompatibility, 8) ||
        !b.read_bits(&avcLevelIndication, 8) ||
        !b.read_bits(&reserved, 6) || reserved != 0x3f ||
        !b.read_bits(&lengthSizeMinusOne, 2) ||
        !b.read_bits(&reserved, 3) || reserved != 0x07 ||
        !b.read_bits(&numOfSequenceParameterSets, 5)
    ) {
        return false;
    }

    sequenceParameterSets.clear();
    for (size_t i = 0; i < numOfSequenceParameterSets; ++i) {
        size_t length;
        const void* data;
        if (!b.read_bits(&length, 16) || !b.read_byte_aligned_bytes(&data, length)) {
            return false;
        }
        sequenceParameterSets.emplace_back(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + length);
    }

    size_t numOfPictureParameterSets;
    if (!b.read_bits(&numOfPictureParameterSets, 8)) {
        return false;
    }

    pictureParameterSets.clear();
    for (size_t i = 0; i < numOfPictureParameterSets; ++i) {
        size_t length;
        const void* data;
        if (!b.read_bits(&length, 16) || !b.read_byte_aligned_bytes(&data, length)) {
            return false;
        }
        pictureParameterSets.emplace_back(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + length);
    }

    return true;
}

std::vector<uint8_t> AVCDecoderConfigurationRecord::encode() const {
    std::vector<uint8_t> ret;

    ret.emplace_back(configurationVersion);
    ret.emplace_back(avcProfileIndication);
    ret.emplace_back(profileCompatibility);
    ret.emplace_back(avcLevelIndication);
    ret.emplace_back(0xfc | lengthSizeMinusOne);
    ret.emplace_back(0xe0 | sequenceParameterSets.size());

    for (auto& sps : sequenceParameterSets) {
        ret.emplace_back(sps.size() >> 8);
        ret.emplace_back(sps.size() & 0xff);
        ret.insert(ret.end(), sps.begin(), sps.end());
    }

    ret.emplace_back(pictureParameterSets.size());

    for (auto& pps : pictureParameterSets) {
        ret.emplace_back(pps.size() >> 8);
        ret.emplace_back(pps.size() & 0xff);
        ret.insert(ret.end(), pps.begin(), pps.end());
    }

    return ret;
}
