#pragma once

#include <memory>

struct SegmentStorage {
    struct Segment {
        virtual bool write(const void* data, size_t len) = 0;
        virtual bool close() = 0;
    };

    virtual std::shared_ptr<Segment> createSegment() = 0;
};
