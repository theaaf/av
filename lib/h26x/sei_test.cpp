#include <gtest/gtest.h>

#include "sei.hpp"

TEST(sei_rbsp, decode) {
    const unsigned char data[] = {
        0x04, 0x47, 0xb5, 0x00, 0x31, 0x47, 0x41, 0x39, 0x34, 0x03, 0xd4, 0xff, 0xfc, 0x80, 0x80,
        0xfd, 0x80, 0x80, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00,
        0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00,
        0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00,
        0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xfa, 0x00, 0x00, 0xff, 0x80
	};
    h264::bitstream bs(data, sizeof(data));

    h264::sei_rbsp sei;
    ASSERT_FALSE(sei.decode(&bs));

    EXPECT_EQ(1, sei.sei_message.size());
    EXPECT_EQ(4, sei.sei_message[0].payloadType);
}
