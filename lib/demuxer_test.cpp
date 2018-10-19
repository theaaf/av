#include <gtest/gtest.h>

#include <cstdlib>

#include "demuxer.hpp"
#include "file_storage.hpp"
#include "logger_test.hpp"
#include "mpeg4.hpp"
#include "mpegts_test.hpp"

#include <h26x/h264.hpp>

TEST(Demuxer, demuxing) {
    struct Handler : EncodedAVHandler {
        virtual ~Handler() {}

        virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
            AVCDecoderConfigurationRecord config;
            EXPECT_TRUE(config.decode(data, len));
            ++encodedVideoConfigCount;
        }

        virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
            EXPECT_TRUE(h264::IterateAVCC(data, len, 4, [](const void* data, size_t len) {}));
            ++encodedVideoCount;
        }

        virtual void handleEncodedAudioConfig(const void* data, size_t len) override {
            MPEG4AudioSpecificConfig config;
            EXPECT_TRUE(config.decode(data, len));
            ++encodedAudioConfigCount;
        }

        virtual void handleEncodedAudio(std::chrono::microseconds pts, const void* data, size_t len) override {
            ++encodedAudioCount;
        }

        size_t encodedVideoConfigCount = 0;
        size_t encodedVideoCount = 0;
        size_t encodedAudioConfigCount = 0;
        size_t encodedAudioCount = 0;
    } handler;

    std::string directory = ".Demuxer-demuxing-test";
    system(("rm -rf " + directory).c_str());

    {
        TestLogDestination logDestination;

        LocalFileStorage storage(&logDestination, directory);
        auto file = storage.createFile("segment.ts");
        file->write(segment_ts, sizeof(segment_ts));
        file->close();

        Demuxer demuxer{&logDestination, directory + "/segment.ts", &handler};
    }

    system(("rm -rf " + directory).c_str());

    EXPECT_GT(handler.encodedVideoConfigCount, 0);
    EXPECT_GT(handler.encodedVideoCount, 10);
    EXPECT_GT(handler.encodedAudioConfigCount, 0);
    EXPECT_GT(handler.encodedAudioCount, 10);
}
