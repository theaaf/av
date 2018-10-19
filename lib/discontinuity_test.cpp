#include <gtest/gtest.h>

#include "segment_manager.hpp"
#include "encoded_av_splitter.hpp"
#include "encoded_av_handler_test.hpp"
#include "logger_test.hpp"
#include "packager.hpp"

struct DiscontinuitySegmentStorage : public SegmentStorage {

    std::vector<bool> segmentDiscontinuities;
    std::mutex _mtx{};

    DiscontinuitySegmentStorage() {}
    virtual ~DiscontinuitySegmentStorage() {}

    virtual std::shared_ptr<SegmentStorage::Segment> createSegment(const std::string& ext) override {
        return std::make_shared<TestSegment>(this);
    }

    struct TestSegment : public SegmentStorage::Segment {

        TestSegment(DiscontinuitySegmentStorage* parent) {
            this->parent = parent;
        }

        virtual ~TestSegment() {
        }

        virtual bool write(const void* data, size_t len) override {
            return true;
        }

        virtual bool close(std::chrono::microseconds duration) override {
            std::lock_guard<std::mutex> l{parent->_mtx};
            parent->segmentDiscontinuities.push_back(static_cast<bool>(metadata.discontinuity));
            return true;
        }
        DiscontinuitySegmentStorage* parent;
    };

};

template <typename H26xPackager>
void testHandleDiscontinuities() {
    const std::vector<std::vector<std::vector<bool>>> runs = {
            {       // first segment is always discontinuous
                    {0,0,0,0,0},
                    {1,0,0,0,0}
            },
            {
                    {0,0,0,1,0},
                    {1,0,0,1,0}
            },
            {
                    {0,0,0,1,1,0},
                    {1,0,0,1,1,0}
            },
            {
                    {1,0,1,0,1,0},
                    {1,0,1,0,1,0}
            }
    };


    for (size_t i = 0; i < runs.size(); i++) {
        auto run = runs[i];

        TestLogDestination logDestination;
        DiscontinuitySegmentStorage segmentStorage;
        EncodedAVSplitter splitter;

        H26xPackager packager{&logDestination, &segmentStorage};
        splitter.addHandler(&packager);

        for (size_t j = 0; j < run[0].size(); j++) {
            bool injectDiscontinuity = run[0][j];

            packager.beginNewSegment();
            if (injectDiscontinuity) {
                splitter.handleEncodedVideoDiscontinuity();
            }
            ExerciseEncodedAVHandler(&splitter);
        }
        packager.beginNewSegment();
        ExerciseEncodedAVHandler(&splitter);

        std::lock_guard<std::mutex> l{segmentStorage._mtx};
        for (size_t j = 0; j < run[0].size(); j++) {
            EXPECT_EQ(run[1][j], segmentStorage.segmentDiscontinuities[j]) << "discontinuity not handled at run " << i << " index " << j;
        }
    }
}

TEST(Discontinuity, handlingDiscontinuities_H264Packager) {
    testHandleDiscontinuities<H264Packager>();
}

TEST(Discontinuity, handlingDiscontinuities_H265Packager) {
    // @TODO: make ExcerciseEncodedAVHandler pump HEVC as well
    /// This will be added in the near future
    // testHandleDiscontinuities<H265Packager>();
}