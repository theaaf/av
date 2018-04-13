#include "ingest_server.hpp"

#include <fmt/format.h>

IngestServer::Stream::Stream(Logger logger, const Configuration& configuration, const std::string& connectionId)
    : _segmentManager{logger}
    , _packager{logger, &_segmentManager}
{
    if (configuration.archiveFileStorage) {
        auto now = std::chrono::system_clock::now();
        auto nowTime = std::chrono::system_clock::to_time_t(now);
        struct tm gmTime;
        gmtime_r(&nowTime, &gmTime);
        _archiver = std::make_unique<Archiver>(
            logger,
            configuration.archiveFileStorage,
            fmt::format("{}/{:02}/{:02}/{}", gmTime.tm_year + 1900, gmTime.tm_mon + 1, gmTime.tm_mday, connectionId)+"/{:010}"
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
