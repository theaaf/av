#pragma once

namespace h265 {

    // According to https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding_tiers_and_levels
    namespace Levels {
        constexpr unsigned int L_1080p32fps_2k20fps = 4;
    }

    // ITU-T H.265 v5 (02/2018) page 68
    // RASL Random Access Skipped Leading
    // RADL Random Access Decodable Leading
    namespace NALUnitType {

        // Coded slice segment of an IDR picture | slice_segment_layer_rbsp()
        constexpr uint8_t IDR_W_RADL = 19; // no associated RASL images, but may have associated RADL images
        constexpr uint8_t IDR_N_LP = 20; // no associated leading pictures present in bitstream

        // Coded slice segment of a Clean Random Access picture | slice_segment_layer_rbsp()
        constexpr uint8_t CRA_NUT = 21;

        constexpr uint8_t VideoParameterSet = 32;
        constexpr uint8_t SequenceParameterSet = 33;
        constexpr uint8_t PictureParameterSet = 34;
        constexpr uint8_t AccessUnitDelimiter = 35;
        constexpr uint8_t EndOfSequence = 36;
        constexpr uint8_t EndOfBitstream = 37;
        constexpr uint8_t FillerData = 38;
        constexpr uint8_t SupplementalEnhancementInformationPrefix = 39;
        constexpr uint8_t SupplementalEnhancementInformationSuffix = 40;
    }


}