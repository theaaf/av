#include "segment_manager.hpp"

#include "utility.hpp"

std::shared_ptr<SegmentStorage::Segment> SegmentManager::createSegment(const std::string& extension) {
    auto segmentId = GenerateUUID();
    auto path = segmentId + "." + extension;
    _logger.with("path", path).info("creating segment");

    std::vector<std::shared_ptr<AsyncFile>> files;
    for (auto fs : _storage) {
        files.emplace_back(std::make_shared<AsyncFile>(fs, path));
    }
    return std::make_shared<Segment>(files);
}

bool SegmentManager::Segment::write(const void* data, size_t len) {
    auto begin = reinterpret_cast<const uint8_t*>(data);
    auto sharedData = std::make_shared<std::vector<uint8_t>>(begin, begin + len);
    for (auto& f : _files) {
        f->write(sharedData);
    }
    return true;
}

bool SegmentManager::Segment::close() {
    for (auto& f : _files) {
        f->close();
    }
    return true;
}
