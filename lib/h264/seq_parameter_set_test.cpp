#include <gtest/gtest.h>

#include "seq_parameter_set.hpp"

TEST(seq_parameter_set_data, decode) {
	const unsigned char data[] = {
        0x4d, 0x40, 0x1f, 0xec, 0xa0, 0x28, 0x02, 0xdd, 0x80, 0xb5, 0x01, 0x01, 0x01, 0x40, 0x00,
        0x00, 0x00, 0x40, 0x00, 0x05, 0xdc, 0x03, 0xc6, 0x0c, 0x65, 0x80,
	};
    h264::bitstream bs(data, sizeof(data));

    h264::seq_parameter_set_rbsp sps;
    ASSERT_FALSE(sps.decode(&bs));

    EXPECT_EQ(h264::ProfileIDC::Main, sps.profile_idc);
    EXPECT_EQ(0, sps.constraint_set0_flag);
    EXPECT_EQ(1, sps.constraint_set1_flag);
    EXPECT_EQ(0, sps.constraint_set2_flag);
    EXPECT_EQ(0, sps.constraint_set3_flag);
    EXPECT_EQ(0, sps.constraint_set4_flag);
    EXPECT_EQ(0, sps.constraint_set5_flag);
    EXPECT_EQ(0, sps.reserved_zero_2bits);
    EXPECT_EQ(31, sps.level_idc);

    EXPECT_EQ(0, sps.log2_max_frame_num_minus4);
    EXPECT_EQ(0, sps.pic_order_cnt_type);
    EXPECT_EQ(2, sps.log2_max_pic_order_cnt_lsb_minus4);

    EXPECT_EQ(4, sps.max_num_ref_frames);
    EXPECT_EQ(0, sps.gaps_in_frame_num_value_allowed_flag);
    EXPECT_EQ(79, sps.pic_width_in_mbs_minus1);
    EXPECT_EQ(44, sps.pic_height_in_map_units_minus1);
    EXPECT_EQ(1, sps.frame_mbs_only_flag);

    EXPECT_EQ(1, sps.direct_8x8_inference_flag);
    EXPECT_EQ(0, sps.frame_cropping_flag);

    EXPECT_EQ(1280, sps.FrameCroppingRectangleWidth());
    EXPECT_EQ(720, sps.FrameCroppingRectangleHeight());
}
