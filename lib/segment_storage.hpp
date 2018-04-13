#pragma once

#include <memory>
#include <string>

struct SegmentStorage {
    // Segment implementations do not need to be thread-safe. All calls must be synchronized by the
    // caller.
    struct Segment {
        virtual bool write(const void* data, size_t len) = 0;
        virtual bool close() = 0;
    };

    // createSegment creates a segment with the given extension (such as "ts").
    virtual std::shared_ptr<Segment> createSegment(const std::string& extension) = 0;
};
