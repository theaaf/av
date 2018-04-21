#include <gtest/gtest.h>

#include <string>

#include "encoded_av_handler_test.hpp"
#include "encoded_av_splitter.hpp"
#include "logger_test.hpp"
#include "video_decoder.hpp"
#include "video_encoder.hpp"

#include <h264/h264.hpp>

TEST(VideoEncoder, encoding) {
    struct Handler : EncodedAVHandler {
        virtual ~Handler() {}

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

    TestLogDestination logDestination;

    Handler before, after;
    ExerciseEncodedAVHandler(&before);

    {
        VideoEncoder::Configuration configuration;
        configuration.width = 600;
        configuration.height = 480;
        VideoEncoder encoder{&logDestination, &after, configuration};

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
