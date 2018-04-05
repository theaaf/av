#include "mpeg4.hpp"

#include "h264/bitstream.hpp"

std::ostream& operator<<(std::ostream& os, const MPEG4AudioObjectType& ot) {
    return os << static_cast<std::underlying_type_t<MPEG4AudioObjectType>>(ot);
}

std::ostream& operator<<(std::ostream& os, const MPEG4ChannelConfiguration& cc) {
    return os << static_cast<std::underlying_type_t<MPEG4ChannelConfiguration>>(cc);
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

    MPEG4SamplingFrequency frequencyEnum;
    if (!b.read_bits(&frequencyEnum, 4)) {
        return false;
    }

    switch (frequencyEnum) {
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
        sequenceParameterSets.emplace_back(data, length);
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
        pictureParameterSets.emplace_back(data, length);
    }

    return true;
}
