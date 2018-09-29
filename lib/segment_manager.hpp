#pragma once

#include <mutex>
#include <vector>

#include "file_storage.hpp"
#include "platform_api.hpp"
#include "segment_storage.hpp"

// SegmentManager asynchronously uploads segments for a stream to multiple file storages.
//
// It can also post to the platform API as segment replicas complete.
class SegmentManager : public SegmentStorage {
public:
    struct Configuration {
        std::vector<FileStorage*> storage;
        PlatformAPI* platformAPI = nullptr;
        std::string streamId;
        std::string gameId;
    };

    explicit SegmentManager(Logger logger, Configuration configuration) : _logger{std::move(logger)}, _configuration{std::move(configuration)} {}
    virtual ~SegmentManager() {}

    virtual std::shared_ptr<SegmentStorage::Segment> createSegment(const std::string& extension) override;

private:
    const Logger _logger;
    std::mutex _mutex;
    const Configuration _configuration;
    int64_t _nextSegmentNumber = 0;

    class Segment : public SegmentStorage::Segment {
    public:
        Segment(Logger logger, const Configuration& configuration, const std::string& path, int64_t segmentNumber);
        virtual ~Segment();

        virtual bool write(const void* data, size_t len) override;
        virtual bool close(std::chrono::microseconds duration) override;

        // Returns true if all files have been fully written and closed.
        bool isComplete() const;

    private:
        struct Replica {
            std::shared_ptr<AsyncFile> file;
            std::thread thread;
        };

        std::vector<Replica> _replicas;
        std::chrono::microseconds _duration;
    };

    // Segments that haven't fully completed need to be kept alive so users aren't blocked when they
    // drop their references.
    std::vector<std::shared_ptr<Segment>> _segments;
};
