#include "seq_parameter_set.hpp"

namespace h264 {

error vui_parameters::decode(bitstream* bs) {
    auto err = bs->decode(&aspect_ratio_info_present_flag);
    if (err) {
        return err;
    }

    if (aspect_ratio_info_present_flag) {
        err = bs->decode(&aspect_ratio_idc);
        if (err) {
            return err;
        }

        if (aspect_ratio_idc == AspectRatioIDC::Extended_SAR) {
            err = bs->decode(
                &sar_width,
                &sar_height
            );
            if (err) {
                return err;
            }
        }
    }

    err = bs->decode(&overscan_info_present_flag);
    if (err) {
        return err;
    }

    if (overscan_info_present_flag) {
        err = bs->decode(&overscan_appropriate_flag);
        if (err) {
            return err;
        }
    }

    err = bs->decode(&video_signal_type_present_flag);
    if (err) {
        return err;
    }

    if (video_signal_type_present_flag) {
        err = bs->decode(
            &video_format,
            &video_full_range_flag,
            &colour_description_present_flag
        );
        if (err) {
            return err;
        }

        if (colour_description_present_flag) {
            err = bs->decode(
                &colour_primaries,
                &transfer_characteristics,
                &matrix_coefficients
            );
            if (err) {
                return err;
            }
        }
    }

    err = bs->decode(&chroma_loc_info_present_flag);
    if (err) {
        return err;
    }

    if (chroma_loc_info_present_flag) {
        err = bs->decode(
            &chroma_sample_loc_type_top_field,
            &chroma_sample_loc_type_bottom_field
        );
        if (err) {
            return err;
        }
    }

    err = bs->decode(&timing_info_present_flag);
    if (err) {
        return err;
    }

    if (timing_info_present_flag) {
        err = bs->decode(
            &num_units_in_tick,
            &time_scale,
            &fixed_frame_rate_flag
        );
        if (err) {
            return err;
        }
    }

    return {};
}

error seq_parameter_set_data::decode(bitstream* bs) {
    auto err = bs->decode(
        &profile_idc,
        &constraint_set0_flag,
        &constraint_set1_flag,
        &constraint_set2_flag,
        &constraint_set3_flag,
        &constraint_set4_flag,
        &constraint_set5_flag,
        &reserved_zero_2bits,
        &level_idc,
        &seq_parameter_set_id
    );
    if (err) {
        return err;
    }

    if (
        profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
        profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
        profile_idc == 128 || profile_idc == 138 || profile_idc == 139 ||
        profile_idc == 134 || profile_idc == 135
    ) {
        err = bs->decode(&chroma_format_idc);
        if (err) {
            return err;
        }

        if (chroma_format_idc == 3) {
            err = bs->decode(&separate_colour_plane_flag);
            if (err) {
                return err;
            }
        }

        err = bs->decode(
            &bit_depth_luma_minus8,
            &bit_depth_chroma_minus8,
            &qpprime_y_zero_transform_bypass_flag,
            &seq_scaling_matrix_present_flag
        );
        if (err) {
            return err;
        }

        if (seq_scaling_matrix_present_flag) {
            return {"scaling matrices are not supported"};
        }
    }

    err = bs->decode(
        &log2_max_frame_num_minus4,
        &pic_order_cnt_type
    );
    if (err) {
        return err;
    }

    if (pic_order_cnt_type == 0) {
        err = bs->decode(&log2_max_pic_order_cnt_lsb_minus4);
        if (err) {
            return err;
        }
    } else if (pic_order_cnt_type == 1) {
        err = bs->decode(
            &delta_pic_order_always_zero_flag,
            &offset_for_non_ref_pic,
            &offset_for_top_to_bottom_field,
            &num_ref_frames_in_pic_order_cnt_cycle
        );
        if (err) {
            return err;
        }

        for (size_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) {
            se offset;
            err = bs->decode(&offset);
            if (err) {
                return err;
            }
            offset_for_ref_frame.emplace_back(offset);
        }
    }

    err = bs->decode(
        &max_num_ref_frames,
        &gaps_in_frame_num_value_allowed_flag,
        &pic_width_in_mbs_minus1,
        &pic_height_in_map_units_minus1,
        &frame_mbs_only_flag
    );
    if (err) {
        return err;
    }

    if (!frame_mbs_only_flag) {
        err = bs->decode(&mb_adaptive_frame_field_flag);
        if (err) {
            return err;
        }
    }

    err = bs->decode(
        &direct_8x8_inference_flag,
        &frame_cropping_flag
    );
    if (err) {
        return err;
    }

    if (frame_cropping_flag) {
        err = bs->decode(
            &frame_crop_left_offset,
            &frame_crop_right_offset,
            &frame_crop_top_offset,
            &frame_crop_bottom_offset
        );
        if (err) {
            return err;
        }
    }

    err = bs->decode(&vui_parameters_present_flag);
    if (err) {
        return err;
    }

    if (vui_parameters_present_flag) {
        err = bs->decode(&vui_parameters);
        if (err) {
            return err;
        }
    }

    return {};
}

error seq_parameter_set_rbsp::decode(bitstream* bs) {
    return seq_parameter_set_data::decode(bs);
}

} // namespace h264
