#pragma once

#include <vector>

#include "h264.hpp"

namespace h264 {

// ITU-T H.264, 04/2017, 7.3.2.1.1
struct seq_parameter_set_data {
    u<8> profile_idc;
    u<1> constraint_set0_flag;
    u<1> constraint_set1_flag;
    u<1> constraint_set2_flag;
    u<1> constraint_set3_flag;
    u<1> constraint_set4_flag;
    u<1> constraint_set5_flag;
    u<2> reserved_zero_2bits;
    u<8> level_idc;
    ue seq_parameter_set_id;

    /* if (
        profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
        profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
        profile_idc == 128 || profile_idc == 138 || profile_idc == 139 ||
        profile_idc == 134 || profile_idc == 135
    ) { */
        ue chroma_format_idc;
        // if (chroma_format_idc == 3)
            u<1> separate_colour_plane_flag;
        ue bit_depth_luma_minus8;
        ue bit_depth_chroma_minus8;
        u<1> qpprime_y_zero_transform_bypass_flag;
        u<1> seq_scaling_matrix_present_flag;
    /* } */

    ue log2_max_frame_num_minus4;
    ue pic_order_cnt_type;

    // if (pic_order_cnt_type == 0)
        ue log2_max_pic_order_cnt_lsb_minus4;
    // else if (pic_order_cnt_type == 1) {
        u<1> delta_pic_order_always_zero_flag;
        se offset_for_non_ref_pic;
        se offset_for_top_to_bottom_field;
        ue num_ref_frames_in_pic_order_cnt_cycle;
        std::vector<se> offset_for_ref_frame;
    // }

    ue max_num_ref_frames;
    u<1> gaps_in_frame_num_value_allowed_flag;
    ue pic_width_in_mbs_minus1;
    ue pic_height_in_map_units_minus1;
    u<1> frame_mbs_only_flag;

    // if (!frame_mbs_only_flag)
        u<1> mb_adaptive_frame_field_flag;

    u<1> direct_8x8_inference_flag;
    u<1> frame_cropping_flag;

    // if (frame_cropping_flag) {
        ue frame_crop_left_offset;
        ue frame_crop_right_offset;
        ue frame_crop_top_offset;
        ue frame_crop_bottom_offset;
    // }

    uint16_t SubWidthC() const {
        return chroma_format_idc == 3 ? 1 : 2;
    }

    uint16_t SubHeightC() const {
        return chroma_format_idc == 1 ? 2 : 1;
    }

    uint16_t MbWidthC() const {
        return 16 / SubWidthC();
    }

    uint16_t MbHeightC() const {
        return 16 / SubHeightC();
    }

    uint64_t PicWidthInMbs() const {
        return pic_width_in_mbs_minus1 + 1;
    }

    uint64_t PicWidthInSamplesL() const {
        return PicWidthInMbs() * 16;
    }

    uint64_t PicWidthInSamplesC() const {
        return PicWidthInMbs() * MbWidthC();
    }

    uint64_t PicHeightInMapUnits() const {
        return pic_height_in_map_units_minus1 + 1;
    }

    uint64_t PicSizeInMapUnits() const {
        return PicWidthInMbs() * PicHeightInMapUnits();
    }

    uint64_t FrameHeightInMbs() const {
        return (2 - frame_mbs_only_flag) * PicHeightInMapUnits();
    }

    uint64_t ChromaArrayType() const {
        return separate_colour_plane_flag ? 0 : chroma_format_idc;
    }

    uint64_t CropUnitX() const {
        return ChromaArrayType() ? SubWidthC() : 1;
    }

    uint64_t CropUnitY() const {
        return (ChromaArrayType() ? SubHeightC() : 1) * (2 - frame_mbs_only_flag);
    }

    uint64_t FrameCroppingRectangleLeft() const {
        return CropUnitX() * frame_crop_left_offset;
    }

    uint64_t FrameCroppingRectangleRight() const {
        return PicWidthInSamplesL() - (CropUnitX() * frame_crop_right_offset + 1);
    }

    uint64_t FrameCroppingRectangleTop() const {
        return CropUnitY() * frame_crop_top_offset;
    }

    uint64_t FrameCroppingRectangleBottom() const {
        return (16 * FrameHeightInMbs()) - (CropUnitY() * frame_crop_bottom_offset + 1);
    }

    uint64_t FrameCroppingRectangleWidth() const {
        return FrameCroppingRectangleRight() - FrameCroppingRectangleLeft() + 1;
    }

    uint64_t FrameCroppingRectangleHeight() const {
        return FrameCroppingRectangleBottom() - FrameCroppingRectangleTop() + 1;
    }

    error decode(bitstream* bs);
};

// ITU-T H.264, 04/2017, 7.3.2.1
struct seq_parameter_set_rbsp : seq_parameter_set_data {
    error decode(bitstream* bs);
};

} // namespace h264
