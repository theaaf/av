#include "ingest_server.hpp"

IngestServer::Stream::Stream(Logger logger, const Configuration& configuration, const std::string& connectionId)
    : _segmentManager{logger}
    , _packager{logger, &_segmentManager}
{
    if (configuration.archiveFileStorage) {
        _archiver = std::make_unique<Archiver>(
            logger,
            configuration.archiveFileStorage,
            connectionId + "/{:010}"
        );
        addReceiver(_archiver.get());
    }

    if (!configuration.segmentFileStorage.empty()) {
        for (auto fs : configuration.segmentFileStorage) {
            _segmentManager.addFileStorage(fs);
        }
        addReceiver(&_packager);
    }
}
