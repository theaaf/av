#pragma once

#include <ostream>
#include <vector>

enum class MPEG4AudioObjectType {
    Null = 0,
    AACMain = 1,
    AACLC = 2,
    AACSSR = 3,
    AACLTP = 4,
    AACScalable = 6,
    Escape = 31,
};

std::ostream& operator<<(std::ostream& os, const MPEG4AudioObjectType& ot);

enum class MPEG4SamplingFrequency {
    F96000Hz = 0,
    F88200Hz = 1,
    F64000Hz = 2,
    F48000Hz = 3,
    F44100Hz = 4,
    F32000Hz = 5,
    F24000Hz = 6,
    F22050Hz = 7,
    F16000Hz = 8,
    F12000Hz = 9,
    F11025Hz = 10,
    F8000Hz = 11,
    F7350Hz = 12,
    Explicit = 15,
};

enum class MPEG4ChannelConfiguration {
    AOTSpecific = 0,
    OneChannel = 1,
    TwoChannels = 2,
    ThreeChannels = 3,
    FourChannels = 4,
    FiveChannels = 5,
    SixChannels = 6,
    EightChannels = 7,
};

std::ostream& operator<<(std::ostream& os, const MPEG4ChannelConfiguration& cc);

struct MPEG4AudioSpecificConfig {
    MPEG4AudioObjectType objectType;
    unsigned int frequency;
    MPEG4ChannelConfiguration channelConfiguration;

    bool decode(const void* data, size_t len);
};

struct AVCDecoderConfigurationRecord {
    unsigned int configurationVersion;
    unsigned int avcProfileIndication;
    unsigned int profileCompatibility;
    unsigned int avcLevelIndication;
    unsigned int lengthSizeMinusOne;
    std::vector<std::pair<const void*, size_t>> sequenceParameterSets;
    std::vector<std::pair<const void*, size_t>> pictureParameterSets;

    bool decode(const void* data, size_t len);
};
