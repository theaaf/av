#include <gtest/gtest.h>

#include <unordered_set>

#include "encoded_av_handler_test.hpp"
#include "encoded_av_splitter.hpp"
#include "logger_test.hpp"
#include "video_decoder.hpp"

TEST(VideoDecoder, decoding) {
    struct Handler : EncodedAVHandler, VideoHandler {
        virtual ~Handler() {}

        virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
            ++encodedFrameCount;
            timestamps.insert(pts.count());
        }

        virtual void handleVideo(std::chrono::microseconds pts, const AVFrame* frame) override {
            EXPECT_GT(timestamps.count(pts.count()), 0);
            EXPECT_GE(pts, lastPTS);
            lastPTS = pts;
            ++frameCount;
        }

        std::unordered_set<std::chrono::microseconds::rep> timestamps;
        size_t encodedFrameCount = 0;
        size_t frameCount = 0;
        std::chrono::microseconds lastPTS{-1};
    } handler;
    ExerciseEncodedAVHandler(&handler);

    {
        TestLogDestination logDestination;
        VideoDecoder decoder{&logDestination, &handler};

        EncodedAVSplitter avHandler;
        avHandler.addHandler(&decoder);
        ExerciseEncodedAVHandler(&avHandler);

        EXPECT_LT(handler.encodedFrameCount - handler.frameCount, 10);
    }

    EXPECT_EQ(handler.encodedFrameCount, handler.frameCount);
}
