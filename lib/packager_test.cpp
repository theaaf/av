#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "encoded_av_receiver_test.hpp"
#include "packager.hpp"
#include "utility.hpp"

struct TestSegmentStorage : SegmentStorage {
    struct Segment : SegmentStorage::Segment {
        virtual bool write(const void* data, size_t len) override {
            bytesWritten += len;
            return true;
        }

        virtual bool close() override {
            closed = true;
            return true;
        }

        bool closed = false;
        size_t bytesWritten = 0;
    };

    virtual std::shared_ptr<SegmentStorage::Segment> createSegment() override {
        auto segment = std::make_shared<Segment>();
        segments.emplace_back(segment);
        return segment;
    }

    std::vector<std::shared_ptr<Segment>> segments;
};

TEST(Packager, packaging) {
    TestSegmentStorage storage;

    {
        Packager packager{Logger::Void, &storage};
        ExerciseEncodedAVReceiver(&packager);
    }

    EXPECT_GT(storage.segments.size(), 2);

    for (size_t i = 0; i < storage.segments.size(); ++i) {
        EXPECT_TRUE(storage.segments[i]->closed) << "segment " << i << " wasn't closed (" << storage.segments.size() << " segments were opened)";
    }

    for (size_t i = 0; i < storage.segments.size() - 1; ++i) {
        EXPECT_GT(storage.segments[i]->bytesWritten, 100 * 1024) << "segment " << i << " should probably be larger (" << storage.segments.size() << " segments were opened)";
    }
}
