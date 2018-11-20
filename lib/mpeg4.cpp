#include "mpeg4.hpp"

#include <h26x/bitstream.hpp>
#include <algorithm>
#include <bitset>

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
    return configurationVersion == other.configurationVersion
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

bool HEVCDecoderConfigurationRecord::operator==(const HEVCDecoderConfigurationRecord &other) const {
    return configurationVersion == other.configurationVersion
        && general_profile_space == other.general_profile_space
        && general_tier_flag == other.general_tier_flag
        && general_profile_idc == other.general_profile_idc
        && general_profile_compatibility_flags == other.general_profile_compatibility_flags
        && general_constraint_indicator_flags == other.general_constraint_indicator_flags
        && general_level_idc == other.general_level_idc
        && min_spatial_segmentation_dc == other.min_spatial_segmentation_dc
        && parallelismType == other.parallelismType
        && chromaFormat == other.chromaFormat
        && bitDepthLumaMinus8== other.bitDepthLumaMinus8
        && bitDepthChromaMinus8 == other.bitDepthChromaMinus8
        && avgFrameRate == other.avgFrameRate
        && constantFrameRate == other.constantFrameRate
        && numTemporalLayers == other.numTemporalLayers
        && temporalIdNested == other.temporalIdNested
        && lengthSizeMinusOne == other.lengthSizeMinusOne
        && numOfArrays == other.numOfArrays
        && width == other.width
        && height == other.height
        && naluArrays == other.naluArrays;
}

std::vector<uint8_t> HEVCDecoderConfigurationRecord::encode() const {
    std::vector<uint8_t> ret;

    ret.emplace_back(configurationVersion);
    ret.emplace_back(general_profile_space);
    ret.emplace_back(general_tier_flag);
    ret.emplace_back(general_profile_idc);
    for (int i = 1; i <= 4; i++) {
        ret.emplace_back(general_profile_compatibility_flags >> (32 - i * 8) & 0xff);
    }
    for (int i = 1; i <= 8; i++) {
        ret.emplace_back(general_constraint_indicator_flags >> (64 - i * 8) & 0xff);
    }
    ret.emplace_back(general_level_idc);
    ret.emplace_back(min_spatial_segmentation_dc >> 8);
    ret.emplace_back(min_spatial_segmentation_dc & 0xff);
    ret.emplace_back(parallelismType);
    ret.emplace_back(chromaFormat);
    ret.emplace_back(bitDepthLumaMinus8);
    ret.emplace_back(bitDepthChromaMinus8);
    ret.emplace_back(avgFrameRate >> 8);
    ret.emplace_back(avgFrameRate & 0xff);
    ret.emplace_back(numTemporalLayers);
    ret.emplace_back(temporalIdNested);
    ret.emplace_back(lengthSizeMinusOne);
    ret.emplace_back(width >> 8);
    ret.emplace_back(width & 0xff);
    ret.emplace_back(height >> 8);
    ret.emplace_back(height & 0xff);
    ret.emplace_back(numOfArrays);

    for (size_t j = 0; j < numOfArrays; j++) {
        auto array = naluArrays[j];

        ret.emplace_back(array.array_completeness << 7 | array.NAL_unit_type);    // 2nd MSB is reserved 0
        ret.emplace_back(array.numNalus >> 8);
        ret.emplace_back(array.numNalus & 0xff);

        for (size_t i = 0; i < array.numNalus; i++) {
            ret.emplace_back(array.nalUnitLength[i] >> 8);
            ret.emplace_back(array.nalUnitLength[i] & 0xff);
            ret.insert(ret.end(), array.nalUnit[i].begin(), array.nalUnit[i].end());
        }
    }

    return ret;
}

bool HEVCDecoderConfigurationRecord::decode(const void *data, size_t len) {
    h264::bitstream b{data, len};

    int reserved;

    if (
        !b.read_bits(&configurationVersion, 8) || configurationVersion != 1 ||
            !b.read_bits(&general_profile_space, 8) ||
            !b.read_bits(&general_tier_flag, 8) ||
            !b.read_bits(&general_profile_idc, 8) ||
            !b.read_bits(&general_profile_compatibility_flags, 32) ||
            !b.read_bits(&general_constraint_indicator_flags, 64) ||
            !b.read_bits(&general_level_idc, 8) ||
            !b.read_bits(&min_spatial_segmentation_dc, 16) ||
            !b.read_bits(&parallelismType, 8) ||
            !b.read_bits(&chromaFormat, 8) ||
            !b.read_bits(&bitDepthLumaMinus8, 8) ||
            !b.read_bits(&bitDepthChromaMinus8, 8) ||
            !b.read_bits(&avgFrameRate, 16) ||
            !b.read_bits(&numTemporalLayers, 8) ||
            !b.read_bits(&temporalIdNested, 8) ||
            !b.read_bits(&lengthSizeMinusOne, 8) ||
            !b.read_bits(&width, 16) ||
            !b.read_bits(&height, 16) ||
            !b.read_bits(&numOfArrays, 8)
    ) {
        return false;
    }

    for (size_t j = 0; j < numOfArrays; j++) {
        naluArrays.emplace_back();
        HVCCArray* array = &naluArrays.back();

        if (
            !b.read_bits(&array->array_completeness, 1) ||
            !b.read_bits(&reserved, 1) || reserved != 0 ||
            !b.read_bits(&array->NAL_unit_type, 6) ||
            !b.read_bits(&array->numNalus, 16) ) {
            return false;
        }

        for (size_t i = 0; i < array->numNalus; i++) {
            uint16_t _nalUnitLength;
            const void* data;

            if (!b.read_bits(&_nalUnitLength, 16) || !b.read_byte_aligned_bytes(&data, _nalUnitLength)) {
                return false;
            }

            array->nalUnitLength.emplace_back(_nalUnitLength);
            array->nalUnit.emplace_back(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + _nalUnitLength);
        }

    }

    return true;
}

h264::error HEVCDecoderConfigurationRecord::parseVPS(const h265::nal_unit& vps){
    h264::bitstream bs{vps.rbsp_byte.data(), vps.rbsp_byte.size()};

    h264::u<3> vps_max_sub_layers_minus1;
    bs.advance_bits(12);
    if (auto err = bs.decode(&vps_max_sub_layers_minus1)) {
        return {"failed to decode vps_max_sub_layers_minus1"};
    }
    numTemporalLayers = std::max((uint64_t) numTemporalLayers,  vps_max_sub_layers_minus1 + 1);
    bs.advance_bits(17);

    if (auto err = parseProfileTierLevel(bs, vps_max_sub_layers_minus1)){
        return err;
    }

    return {};
}

h264::error HEVCDecoderConfigurationRecord::parseSPS(const h265::nal_unit &sps) {
    h264::bitstream bs{sps.rbsp_byte.data(), sps.rbsp_byte.size()};

    uint32_t i, j, sps_max_sub_layers_minus1, log2_max_pic_order_cnt_lsb_minus4;
    uint32_t num_short_term_ref_pic_sets, num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS];

    if (!bs.advance_bits(4) || !bs.read_bits(&sps_max_sub_layers_minus1, 3)) {
        return {"failed to read sps_max_sub_layers_minus1"};
    }

    numTemporalLayers = std::max((uint32_t)numTemporalLayers, sps_max_sub_layers_minus1 + 1);
    if(!bs.read_bits(&temporalIdNested, 1)) {
        return {"failed to read temporalIdNested"};
    }

    if (auto err = parseProfileTierLevel(bs, sps_max_sub_layers_minus1)) {
        return {err};
    }

    h264::ue _ignore, _chromaFormat;
    h264::se _ignore_se;

    if (bs.decode(&_ignore)) {
        return {"failed to decode ignore data before chromaFormat"};
    }
    if (bs.decode( &_chromaFormat)){
        return {"failed to decode chromaFormat"};
    }
    chromaFormat = _chromaFormat;
    if (chromaFormat == 3) {
        if (!bs.advance_bits(1)) {
            return {"failed to skip bit after chromaFormat == 3"};
        }
    }

    h264::ue _pic_width_in_luma_samples, _pic_height_in_luma_samples;
    h264::u<1> _conformance_window_flag;

    if (bs.decode(&_pic_width_in_luma_samples, &_pic_height_in_luma_samples, &_conformance_window_flag)) {
        return {"failed to get pic_size_in_luma_samples or conformance_window_flag"};
    }
    width = _pic_width_in_luma_samples;
    height = _pic_height_in_luma_samples;
    // @TODO: calculate stream width/height
    if (_conformance_window_flag) {
        if (bs.decode(&_ignore, &_ignore, &_ignore, &_ignore)) {
            return {"failed to decode conformance window offsets"};
        }
    }
    h264::ue _bitDepthLumaMinus8, _bitDepthChromaMinus8, _log2_max_pic_order_cnt_lsb_minus4;
    if (bs.decode(&_bitDepthLumaMinus8, &_bitDepthChromaMinus8, &_log2_max_pic_order_cnt_lsb_minus4)) {
        return {"failed to decode bitDepthLuma's or pic order cnt"};
    }
    bitDepthLumaMinus8 = _bitDepthLumaMinus8;
    bitDepthChromaMinus8 = _bitDepthChromaMinus8;
    log2_max_pic_order_cnt_lsb_minus4 = _log2_max_pic_order_cnt_lsb_minus4;
    if (!bs.read_bits(&i, 1)) {
        return {"failed to decode sps_sub_layer_ordering_info_present_flag"};
    }
    i = i ? 0 : sps_max_sub_layers_minus1;
    for (; i <= sps_max_sub_layers_minus1; i++) {
        if (bs.decode(&_ignore, &_ignore, &_ignore)) {
            return {"failed to decode sub_layer_order_info"};
        }
    }
    if (bs.decode(&_ignore, &_ignore, &_ignore, &_ignore, &_ignore, &_ignore)) {
        return {"failed to decode a block of six unneded ue values"};
    }
    if ( (!bs.read_bits(&i, 1) || !bs.read_bits(&j, 1))) {
        return {"failed to decoded conditions for if scaling list data exists"};
    }
    if (i && j) {
        /// skip_scaling_list_data()
        int temp, num_coeffs;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < ((i == 3) ? 2 : 6); j++) {
                if (!bs.read_bits(&temp, 1)) {
                    return {"failed in skip_scaling_list_data read 1"};
                }
                if (temp) {
                    if (bs.decode(&_ignore)) {
                        return {"failed in skip_scaling_list_data read 2"};
                    }
                } else {
                    num_coeffs = std::min(64, 1 << (4 + (i << 1)));
                    if (i > 1) {
                        if (bs.decode(&_ignore_se)) {
                            return {"failed in skip_scaling_list_data read 3"};
                        }
                    }
                    for (int k = 0; k < num_coeffs; k++) {
                        if (bs.decode(&_ignore_se)) {
                            return {"failed in skip_scaling_list_data read 4"};
                        }
                    }
                }
            }
        }
    }
    if (!bs.advance_bits(2) || !bs.read_bits(&i, 1)) {
        return {"failed to skip two bits and read pcm_enabled_flag"};
    }
    if (i) {
        if (!bs.advance_bits(8) || bs.decode(&_ignore, &_ignore) || !bs.advance_bits(1)) {
            return {"failed to skip pcm data chunk"};
        }
    }

    h264::ue _num_short_term_ref_pic_sets;
    if (bs.decode(&_num_short_term_ref_pic_sets)) {
        return {"failed to decode num_short_term_ref_pic_sets"};
    }
    num_short_term_ref_pic_sets = _num_short_term_ref_pic_sets;

    for (i = 0; i < num_short_term_ref_pic_sets; i++) {
        /// parse_rps(i, num_short_term_ref_pic_sets, num_delta_pocs);
        if (!parse_rps(bs, i, num_short_term_ref_pic_sets, num_delta_pocs)) {
            return {"failed to parse rps"};
        }
    }

    if (!bs.read_bits(&i, 1)) {
        return {"failed to read long_term_ref_pics_present_flag"};
    }
    h264::ue _num_long_term_ref_pics_sps;
    if (bs.decode(&_num_long_term_ref_pics_sps)) {
        return {"failed to decode num_long_term_ref_pics_sps"};
    }
    for (int i = 0; i < _num_long_term_ref_pics_sps; i++) {
        int len = std::min(log2_max_pic_order_cnt_lsb_minus4 + 4, 16u);
        if (!bs.advance_bits(len + 1)) {
            return {"failed to skip some long term pics related bits"};
        }
    }
    if (!bs.advance_bits(2) || !bs.read_bits(&i, 1)) {
        return {"failed to skip two bits and get vui_parameters_present_flag"};
    }
    if (i) {
        /// return false if not parse_vui()
        // the only thing the vui_parameters would give us is min_spatial_segmentation_idc
        // which, must indicate a level of segmentation __equal to or less than__ the lowest level
        // so, in the interest of not writing parse code just to skip over a bunch of different hdr parameter non-sense,
        // we'll keep min_spatial_segmentation_idc at the legal default value of 0
    }
    return {};
}

h264::error HEVCDecoderConfigurationRecord::parsePPS(const h265::nal_unit &pps) {
    h264::bitstream bs{pps.rbsp_byte.data(), pps.rbsp_byte.size()};
    uint8_t tiles_enabled_flag{0}, entropy_coding_sync_enabled_flag{0};
    uint64_t i{0};

    h264::ue _ignore;
    h264::se _ignore_se;
    if (bs.decode(&_ignore, &_ignore) ||
        !bs.advance_bits(7) ||
        bs.decode(&_ignore, &_ignore, &_ignore_se) ||
        !bs.advance_bits(2) ||
        !bs.read_bits(&i, 1)
    ) {
        return {"failed to decode top bits"};
    }
    if (i) {
        if (bs.decode(&_ignore)) {
            return {"failed to decode ignore"};
        }
    }

    if (bs.decode(&_ignore_se, &_ignore_se) ||
        !bs.advance_bits(4) ||
        !bs.read_bits(&tiles_enabled_flag, 1) ||
        !bs.read_bits(&entropy_coding_sync_enabled_flag, 1)
    ) {
        return {"failed to decode parallelism chunk"};
    }

    if (entropy_coding_sync_enabled_flag && tiles_enabled_flag) {
        parallelismType = 0;
    } else if (entropy_coding_sync_enabled_flag) {
        parallelismType = 3;
    } else if (tiles_enabled_flag) {
        parallelismType = 2;
    } else {
        parallelismType = 1;
    }

    return {};
}

h264::error HEVCDecoderConfigurationRecord::parseProfileTierLevel(h264::bitstream &bs, unsigned int max_sub_layers_minus1) {
    // 7.4.3.1: vps_max_layers_minus1 is in [0, 62].
    const static size_t HEVC_MAX_LAYERS = 63;
    std::bitset<HEVC_MAX_LAYERS> sub_layer_profile_present_flag{0};
    std::bitset<HEVC_MAX_LAYERS> sub_layer_level_present_flag{0};

    HEVCProfileTierLevel ptl{};

    if (!bs.read_bits(&ptl.profile_space, 2) ||
        !bs.read_bits(&ptl.tier_flag, 1) ||
        !bs.read_bits(&ptl.profile_idc, 5) ||
        !bs.read_bits(&ptl.profile_compatibility_flags, 32) ||
        !bs.read_bits(&ptl.constraint_indicator_flags, 48) ||
        !bs.read_bits(&ptl.level_idc, 8)) {
        return {"failed to read profile tier level profile space, tier_flag, compatibility indicator flags..."};
    }
    updateProfileTierLevel(ptl);

    uint8_t flag1, flag2;
    for (int i = 0; i < max_sub_layers_minus1; i++) {
        if ( !bs.read_bits(&flag1, 1) || !bs.read_bits(&flag2, 1)) {
            return {"ptl failed to read sub_layer_..._present_flags"};
        }
        sub_layer_profile_present_flag[i] = flag1;
        sub_layer_level_present_flag[i] = flag2;
    }

    if (max_sub_layers_minus1 > 0) {
        for (int i = max_sub_layers_minus1; i < 8; i++) {
            if (!bs.advance_bits(2)) {
                return {"ptl failed to read reserved zero 2 bits"};
            }
        }
    }

    for (int i = 0; i < max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            if (!bs.advance_bits(32 + 32 + 24)) {
                return {"ptl failed to skip over 32 + 32 + 24 chunk"};
            }
        }

        if (sub_layer_level_present_flag[i]) {
            if (!bs.advance_bits(8)) {
                return {"ptl failed to skip over sub_layer_present_flags"};
            }
        }
    }

    return {};
}

void HEVCDecoderConfigurationRecord::updateProfileTierLevel(const HEVCProfileTierLevel& ptl) {
    general_profile_space = ptl.profile_space;
    if (general_tier_flag < ptl.tier_flag) {
        general_level_idc = ptl.level_idc;
    } else {
        general_level_idc = std::max(general_level_idc, ptl.level_idc);
    }
    general_tier_flag = std::max(general_tier_flag, ptl.tier_flag);
    general_profile_idc = std::max(general_profile_idc, ptl.profile_idc);
    general_profile_compatibility_flags &= ptl.profile_compatibility_flags;
    general_constraint_indicator_flags &= ptl.constraint_indicator_flags;
}

bool HEVCDecoderConfigurationRecord::parse_rps(h264::bitstream &bs, uint32_t rps_idx, uint32_t num_rps, uint32_t *num_delta_pocs) {
    uint32_t i;
    h264::ue _ignore;
    h264::u<1> _1bit;

    if (!bs.read_bits(&i, 1)) {
        return false;
    }
    if (rps_idx && i) {
        if (rps_idx >= num_rps) {
            return false;
        }
        if (!bs.advance_bits(1) || bs.decode(&_ignore)) {
            return false;
        }

        num_delta_pocs[rps_idx] = 0;
        for (i = 0; i <= num_delta_pocs[rps_idx -1]; i++) {
            uint8_t use_delta_flag = 0;
            uint8_t used_by_curr_pic_flag;

            if (!bs.read_bits(&used_by_curr_pic_flag, 1)) {
                return false;
            }
            if (!used_by_curr_pic_flag) {
                if (!bs.read_bits(&use_delta_flag, 1)) {
                    return false;
                }
            }
            if (used_by_curr_pic_flag || use_delta_flag) {
                num_delta_pocs[rps_idx]++;
            }
        }
    } else {
        h264::ue num_negative_pics, num_positive_pics;
        if (bs.decode(&num_negative_pics, &num_positive_pics)) {
            return false;
        }
        if ((num_positive_pics + (uint64_t) num_negative_pics) * 2 > bs.bits_remaining()) {
            return false;
        }
        num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

        for (i = 0; i < num_negative_pics; i++) {
            bs.decode(&_ignore, &_1bit);
        }
        for (i = 0; i < num_positive_pics; i++) {
            bs.decode(&_ignore, &_1bit);
        }
    }
    return true;
}

void HEVCDecoderConfigurationRecord::addNalu(const uint8_t *data, const size_t len) {
    auto rawNalContents = reinterpret_cast<const uint8_t*>(data);
    auto naluType = (*rawNalContents & 0x7F) >> 1;

    HVCCArray* array = nullptr;
    for (auto& a: naluArrays) {
        if (a.NAL_unit_type == naluType) {
            array = &a;
            break;
        }
    }
    if (array == nullptr) {
        naluArrays.emplace_back();
        array = &naluArrays.back();
        array->array_completeness = 0;
        array->NAL_unit_type = naluType;
        numOfArrays++;
    }
    array->nalUnit.emplace_back(rawNalContents, rawNalContents + len);
    array->nalUnitLength.emplace_back(len);
    array->numNalus++;
}

std::vector<std::vector<uint8_t>>* HEVCDecoderConfigurationRecord::fetchNalus(uint8_t NALType) {
    for (auto& array : naluArrays) {
        if (array.NAL_unit_type == NALType) {
            return &array.nalUnit;
        }
    }
    return nullptr;
}