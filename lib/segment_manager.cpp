#include "segment_manager.hpp"

#include "utility.hpp"

std::shared_ptr<SegmentStorage::Segment> SegmentManager::createSegment(const std::string& extension) {
    auto segmentId = GenerateUUID();
    auto path = segmentId + "." + extension;

    std::lock_guard<std::mutex> l{_mutex};

    _logger.with("path", path, "segment_number", _nextSegmentNumber).info("creating segment");
    auto segment = std::make_shared<Segment>(_logger, _configuration, path, _nextSegmentNumber++);

    for (size_t i = 0; i < _segments.size();) {
        if (_segments[i]->isComplete()) {
            _segments[i] = _segments.back();
            _segments.resize(_segments.size() - 1);
        } else {
            ++i;
        }
    }
    _segments.emplace_back(segment);

    return segment;
}

SegmentManager::Segment::Segment(Logger logger, const SegmentManager::Configuration& configuration, const std::string& path, int64_t segmentNumber) {
    for (auto fs : configuration.storage) {
        auto uri = fs->uri(path);
        Replica replica;
        replica.file = std::make_shared<AsyncFile>(fs, path);
        replica.thread = std::thread([
                this,
                file = replica.file,
                uri,
                segmentNumber,
                logger = logger.with("uri", uri),
                configuration
        ]{
            file->wait();
            if (!file->isHealthy()) {
                logger.error("error writing segment replica");
                return;
            }

            if (configuration.platformAPI) {
                PlatformAPI::AVStreamSegmentReplica replica;
                replica.streamId = configuration.streamId;
                replica.segmentNumber = segmentNumber;
                replica.location = uri;
                replica.duration = std::chrono::duration_cast<decltype(replica.duration)>(_duration);

                auto result = configuration.platformAPI->createAVStreamSegmentReplica(replica);
                if (!result.requestError.empty()) {
                    logger.error("createAVStreamSegmentReplica request error: {}", result.requestError);
                } else if (!result.errors.empty()) {
                    for (auto& err : result.errors) {
                        logger.error("createAVStreamSegmentReplica error: {}", err.message);
                    }
                } else {
                    auto replicaId = result.data.id;
                    logger.with("av_stream_segment_replica_id", replicaId).info("created platform AVStreamSegmentReplica");
                }
            }
        });
        _replicas.emplace_back(std::move(replica));
    }
}

SegmentManager::Segment::~Segment() {
    for (auto& r : _replicas) {
        r.thread.join();
    }
}

bool SegmentManager::Segment::write(const void* data, size_t len) {
    auto begin = reinterpret_cast<const uint8_t*>(data);
    auto sharedData = std::make_shared<std::vector<uint8_t>>(begin, begin + len);
    for (auto& r : _replicas) {
        r.file->write(sharedData);
    }
    return true;
}

bool SegmentManager::Segment::close(std::chrono::microseconds duration) {
    _duration = duration;
    for (auto& r : _replicas) {
        r.file->close();
    }
    return true;
}

bool SegmentManager::Segment::isComplete() const {
    for (auto& r : _replicas) {
        if (!r.file->isComplete()) {
            return false;
        }
    }
    return true;
}
