#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "encoded_av_handler_test.hpp"
#include "logger_test.hpp"
#include "packager.hpp"
#include "segmenter.hpp"
#include "utility.hpp"

struct TestSegmentStorage : SegmentStorage {
    struct Segment : SegmentStorage::Segment {
        virtual bool write(const void* data, size_t len) override {
            bytesWritten += len;
            return true;
        }

        virtual ~Segment(){}

        virtual bool close(std::chrono::microseconds duration) override {
            closed = true;
            this->duration = duration;
            return true;
        }

        std::string extension;
        bool closed = false;
        size_t bytesWritten = 0;
        std::chrono::microseconds duration;
    };

    virtual std::shared_ptr<SegmentStorage::Segment> createSegment(const std::string& extension) override {
        auto segment = std::make_shared<Segment>();
        segment->extension = extension;
        segments.emplace_back(segment);
        return segment;
    }

    std::vector<std::shared_ptr<Segment>> segments;
};

struct TestH264Packager : H264Packager {

    TestH264Packager(Logger logger, SegmentStorage* storage) : H264Packager(std::move(logger), storage) {}
    virtual ~TestH264Packager() {}

    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        H264Packager::handleEncodedVideo(pts, dts, data, len);
        if (startTime == std::chrono::microseconds::min()) {
            startTime = pts;
        }
        if (pts > maxTime) {
            maxTime = pts;
        }
    }

    std::chrono::microseconds startTime = std::chrono::microseconds::min();
    std::chrono::microseconds maxTime = startTime;

};

TEST(Packager, packaging) {
    TestSegmentStorage storage;
    TestLogDestination logDestination;
    std::chrono::microseconds streamTime;
    std::chrono::microseconds sumDurations{0};

    {
        TestH264Packager packager{&logDestination, &storage};
        Segmenter segmenter{&logDestination, &packager, [&]{
            packager.beginNewSegment();
        }};
        ExerciseEncodedAVHandler(&segmenter);
        streamTime = packager.maxTime - packager.startTime;
    }

    EXPECT_GT(storage.segments.size(), 2);

    for (size_t i = 0; i < storage.segments.size(); ++i) {
        EXPECT_EQ("ts", storage.segments[i]->extension) << "segment " << i << " has unexpected extension";
        EXPECT_GT(storage.segments[i]->duration, std::chrono::milliseconds(500)) << "segment " << i << " has short duration";
        EXPECT_LT(storage.segments[i]->duration, std::chrono::seconds(30)) << "segment " << i << " has long duration";
        EXPECT_TRUE(storage.segments[i]->closed) << "segment " << i << " wasn't closed (" << storage.segments.size() << " segments were opened)";
        sumDurations += storage.segments[i]->duration;
    }

    // we know that maxTime will always be one frame too short since (33.3ms @ 30 FPS)
    auto target = std::chrono::duration_cast<std::chrono::milliseconds>(streamTime) + std::chrono::milliseconds(33);
    EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(sumDurations).count(), target.count()) << "segment durations do not add up to stream duration";

    for (size_t i = 0; i < storage.segments.size() - 1; ++i) {
        EXPECT_GT(storage.segments[i]->bytesWritten, 100 * 1024) << "segment " << i << " should probably be larger (" << storage.segments.size() << " segments were opened)";
    }
}
