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

TEST(Packager, packaging) {
    TestSegmentStorage storage;
    TestLogDestination logDestination;

    {
        Packager packager{&logDestination, &storage};
        Segmenter segmenter{&logDestination, &packager, [&]{
            packager.beginNewSegment();
        }};
        ExerciseEncodedAVHandler(&segmenter);
    }

    EXPECT_GT(storage.segments.size(), 2);

    for (size_t i = 0; i < storage.segments.size(); ++i) {
        EXPECT_EQ("ts", storage.segments[i]->extension) << "segment " << i << " has unexpected extension";
        EXPECT_GT(storage.segments[i]->duration, std::chrono::milliseconds(500)) << "segment " << i << " has short duration";
        EXPECT_LT(storage.segments[i]->duration, std::chrono::seconds(30)) << "segment " << i << " has long duration";
        EXPECT_TRUE(storage.segments[i]->closed) << "segment " << i << " wasn't closed (" << storage.segments.size() << " segments were opened)";
    }

    for (size_t i = 0; i < storage.segments.size() - 1; ++i) {
        EXPECT_GT(storage.segments[i]->bytesWritten, 100 * 1024) << "segment " << i << " should probably be larger (" << storage.segments.size() << " segments were opened)";
    }
}
