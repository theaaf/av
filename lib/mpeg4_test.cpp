#include <gtest/gtest.h>

#include "h264/h264.hpp"
#include "mpeg4.hpp"

TEST(MPEG4AudioSpecificConfig, decode) {
	const unsigned char data[] = {0x12, 0x10};

	MPEG4AudioSpecificConfig config;
	ASSERT_TRUE(config.decode(data, sizeof(data)));

    EXPECT_EQ(MPEG4AudioObjectType::AACLC, config.objectType);
    EXPECT_EQ(44100, config.frequency);
    EXPECT_EQ(MPEG4ChannelConfiguration::TwoChannels, config.channelConfiguration);
}

TEST(AVCDecoderConfigurationRecord, decode) {
	const unsigned char data[] = {
        0x01, 0x4d, 0x40, 0x1f, 0xff, 0xe1, 0x00, 0x1c, 0x67, 0x4d, 0x40, 0x1f, 0xec, 0xa0, 0x28,
        0x02, 0xdd, 0x80, 0xb5, 0x01, 0x01, 0x01, 0x40, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x05,
        0xdc, 0x03, 0xc6, 0x0c, 0x65, 0x80, 0x01, 0x00, 0x04, 0x68, 0xef, 0xbc, 0x80,
	};

	AVCDecoderConfigurationRecord config;
	ASSERT_TRUE(config.decode(data, sizeof(data)));

    EXPECT_EQ(1, config.configurationVersion);
    EXPECT_EQ(h264::kMainProfileIDC, config.avcProfileIndication);
    EXPECT_EQ(0x40, config.profileCompatibility);
    EXPECT_EQ(31, config.avcLevelIndication);
    EXPECT_EQ(3, config.lengthSizeMinusOne);

    ASSERT_EQ(1, config.sequenceParameterSets.size());
    EXPECT_EQ(data + sizeof(data) - 7 - 28, config.sequenceParameterSets[0].first);
    EXPECT_EQ(28, config.sequenceParameterSets[0].second);

    ASSERT_EQ(1, config.pictureParameterSets.size());
    EXPECT_EQ(data + sizeof(data) - 4, config.pictureParameterSets[0].first);
    EXPECT_EQ(4, config.pictureParameterSets[0].second);
}
