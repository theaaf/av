#pragma once

#include <mutex>
#include <vector>

#include "file_storage.hpp"
#include "segment_storage.hpp"

// SegmentManager asynchronously uploads segments to multiple file storages.
class SegmentManager : public SegmentStorage {
public:
    explicit SegmentManager(Logger logger) : _logger{std::move(logger)} {}
    virtual ~SegmentManager() {}

    void addFileStorage(FileStorage* storage) {
        std::lock_guard<std::mutex> l{_mutex};
        _storage.emplace_back(storage);
    }

    virtual std::shared_ptr<SegmentStorage::Segment> createSegment(const std::string& extension) override;

private:
    const Logger _logger;
    std::mutex _mutex;
    std::vector<FileStorage*> _storage;

    class Segment : public SegmentStorage::Segment {
    public:
        Segment(std::vector<std::shared_ptr<AsyncFile>> files) : _files{files} {}
        virtual ~Segment() {}

        virtual bool write(const void* data, size_t len) override;
        virtual bool close() override;

    private:
        std::vector<std::shared_ptr<AsyncFile>> _files;
    };
};
