#pragma once

#include <ostream>
#include <vector>
#include <h26x/nal_unit.hpp>

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

static constexpr size_t ChannelCount(MPEG4ChannelConfiguration cc) {
    return cc == MPEG4ChannelConfiguration::EightChannels ? 8 : static_cast<size_t>(cc);
}

struct MPEG4AudioSpecificConfig {
    MPEG4AudioObjectType objectType;
    unsigned int frequency;
    MPEG4ChannelConfiguration channelConfiguration;

    constexpr size_t channelCount() const { return ChannelCount(channelConfiguration); }
    MPEG4SamplingFrequency frequencyIndex() const;

    constexpr bool operator==(const MPEG4AudioSpecificConfig& other) const {
        return objectType == other.objectType && frequency == other.frequency && channelConfiguration == other.channelConfiguration;
    }

    std::vector<uint8_t> adtsHeader(size_t aacLength) const;

    bool decode(const void* data, size_t len);

    std::vector<uint8_t> encode() const;
};

struct AVCDecoderConfigurationRecord {
    unsigned int configurationVersion = 1;
    unsigned int avcProfileIndication;
    unsigned int profileCompatibility;
    unsigned int avcLevelIndication;
    unsigned int lengthSizeMinusOne;
    std::vector<std::vector<uint8_t>> sequenceParameterSets;
    std::vector<std::vector<uint8_t>> pictureParameterSets;

    bool operator==(const AVCDecoderConfigurationRecord& other) const;
    bool operator!=(const AVCDecoderConfigurationRecord& other) const { return !(*this == other); }

    bool decode(const void* data, size_t len);

    std::vector<uint8_t> encode() const;
};

struct HEVCProfileTierLevel {
    uint8_t profile_space;
    uint8_t tier_flag;
    uint8_t profile_idc;
    uint32_t profile_compatibility_flags;
    uint64_t constraint_indicator_flags;
    uint8_t level_idc;
};

struct HVCCArray {
    uint8_t array_completeness{0};
    uint8_t NAL_unit_type;
    uint16_t numNalus{0};
    std::vector<uint16_t> nalUnitLength;
    std::vector<std::vector<uint8_t>> nalUnit;

    bool operator== (const HVCCArray& other) const {
        return true
            && array_completeness == other.array_completeness
           && NAL_unit_type == other.NAL_unit_type
           && numNalus == other.numNalus
           && nalUnitLength == other.nalUnitLength
           && nalUnit == other.nalUnit;
    }
};

struct HEVCDecoderConfigurationRecord {
    uint8_t configurationVersion = 1;
    uint8_t general_profile_space{0};
    uint8_t general_tier_flag{0};
    uint8_t general_profile_idc{0};
    uint32_t general_profile_compatibility_flags{0xff'ff'ff'ff};
    uint64_t general_constraint_indicator_flags{0xff'ff'ff'ff'ff'ff};
    uint8_t general_level_idc{0};
    uint16_t min_spatial_segmentation_dc{0};
    uint8_t parallelismType{0};
    uint8_t chromaFormat{0};
    uint8_t bitDepthLumaMinus8{0};
    uint8_t bitDepthChromaMinus8{0};
    uint16_t avgFrameRate{0};
    uint8_t constantFrameRate{0};
    uint8_t numTemporalLayers{0};
    uint8_t temporalIdNested;
    uint8_t lengthSizeMinusOne{3};
    uint8_t numOfArrays{0};
    uint16_t width{0};
    uint16_t height{0};
    std::vector<HVCCArray> naluArrays;

    bool operator==(const HEVCDecoderConfigurationRecord& other) const;
    bool operator!=(const HEVCDecoderConfigurationRecord& other) const { return !(*this == other); }

    bool decode(const void* data, size_t len);
    std::vector<uint8_t> encode() const;

    h264::error parseVPS(const h265::nal_unit& vps);
    h264::error parseSPS(const h265::nal_unit& sps);
    h264::error parsePPS(const h265::nal_unit& pps);

    void addNalu(const uint8_t* data, const size_t len);
    std::vector<std::vector<uint8_t>>* fetchNalus(uint8_t NALType);

protected:

    // 7.4.3.2.1: num_short_term_ref_pic_sets is in [0, 64].
    const static uint8_t HEVC_MAX_SHORT_TERM_REF_PIC_SETS = 64;

    bool parse_rps(h264::bitstream& bs, uint32_t rps_idx, uint32_t num_rps, uint32_t num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS]);

    h264::error parseProfileTierLevel(h264::bitstream& bs, unsigned int max_sub_layers_minus1);
    void updateProfileTierLevel(const HEVCProfileTierLevel&);
};
