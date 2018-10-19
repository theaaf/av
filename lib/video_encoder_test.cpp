#include <gtest/gtest.h>

#include <string>

#include "encoded_av_handler_test.hpp"
#include "encoded_av_splitter.hpp"
#include "logger_test.hpp"
#include "video_decoder.hpp"
#include "video_encoder.hpp"

#include <h26x/h264.hpp>

struct H264Handler : EncodedAVHandler {
    virtual ~H264Handler() {}

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
        auto config = std::make_unique<AVCDecoderConfigurationRecord>();
        ASSERT_TRUE(config->decode(data, len));
        this->config = std::move(config);
    }

    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        ASSERT_TRUE(config);
        ++frameCount;
        EXPECT_TRUE(h264::IterateAVCC(data, len, config->lengthSizeMinusOne + 1, [&](const void* data, size_t len) {
            EXPECT_GT(len, 0);
        }));
    }

    std::unique_ptr<AVCDecoderConfigurationRecord> config;
    size_t frameCount = 0;
};

struct H265Handler : EncodedAVHandler {
    virtual ~H265Handler() {}

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
        auto config = std::make_unique<HEVCDecoderConfigurationRecord>();
        ASSERT_TRUE(config->decode(data, len));
        this->config = std::move(config);
    }

    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        ASSERT_TRUE(config);
        ++frameCount;
        EXPECT_TRUE(h264::IterateAnnexB(data, len, [&](const void* data, size_t len) {
            EXPECT_GT(len, 0);
        }));
    }

    std::unique_ptr<HEVCDecoderConfigurationRecord> config;
    size_t frameCount = 0;
};

TEST(VideoEncoder, encoding_h264) {
    TestLogDestination logDestination;

    H264Handler before, after;
    ExerciseEncodedAVHandler(&before);
    {
        VideoEncoderConfiguration configuration;
        configuration.width = 600;
        configuration.height = 480;
        configuration.x264.h264Preset = "veryfast";
        H264VideoEncoder encoder{&logDestination, &after, configuration};
        {
            VideoDecoder decoder{&logDestination, &encoder};

            EncodedAVSplitter avHandler;
            avHandler.addHandler(&decoder);
            ExerciseEncodedAVHandler(&avHandler);
        }
        EXPECT_LT(before.frameCount - after.frameCount, 30);
    }
    EXPECT_EQ(before.frameCount, after.frameCount);
}

TEST(VideoEncoder, encoding_h265) {
        TestLogDestination logDestination;

        H264Handler before;
        H265Handler after;
        ExerciseEncodedAVHandler(&before);
        {
            VideoEncoderConfiguration configuration;
            configuration.width = 1280;
            configuration.height = 720;
            configuration.codec = VideoCodec::x265;
            H265VideoEncoder encoder{&logDestination, &after, configuration};
            {
                VideoDecoder decoder{&logDestination, &encoder};

                EncodedAVSplitter avHandler;
                avHandler.addHandler(&decoder);
                ExerciseEncodedAVHandler(&avHandler);
            }
            EXPECT_LT(before.frameCount - after.frameCount, 30);
        }
        EXPECT_EQ(before.frameCount, after.frameCount);
}
