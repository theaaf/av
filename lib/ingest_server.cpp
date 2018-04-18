#include "ingest_server.hpp"

std::shared_ptr<EncodedAVHandler> IngestServer::authenticate(const std::string& connectionId) {
    auto logger = _logger.with("connection_id", connectionId);

    if (!_configuration.platformAPI) {
        return std::make_shared<Stream>(logger, _configuration, connectionId);
    }

    // TODO: actual authentication

    std::string streamId;

    {
        // TODO: these hard-coded things will be parameterized as transcoding configuration
        PlatformAPI::AVStream stream;
        stream.bitrate = 4000000;
        stream.codecs = {"mp4a.40.2", "avc1.64001f"};
        stream.maximumSegmentDuration = std::chrono::seconds(30);
        stream.videoHeight = 1120;
        stream.videoWidth = 700;

        auto result = _configuration.platformAPI->createAVStream(stream);
        if (!result.requestError.empty()) {
            logger.error("createAVStream request error: {}", result.requestError);
            return nullptr;
        } else if (!result.errors.empty()) {
            for (auto& err : result.errors) {
                logger.error("createAVStream error: {}", err.message);
            }
            return nullptr;
        }
        streamId = result.data.id;
        logger.with("av_stream_id", streamId).info("created platform AVStream");
    }

    return std::make_shared<Stream>(logger, _configuration, connectionId, streamId);
}

IngestServer::Stream::Stream(Logger logger, const Configuration& configuration, std::string connectionId, std::string streamId) {
    if (configuration.archiveFileStorage) {
        _archiver = std::make_unique<Archiver>(
            logger,
            configuration.archiveFileStorage,
            connectionId + "/{:010}"
        );
        addHandler(_archiver.get());
    }

    if (!configuration.segmentFileStorage.empty()) {
        SegmentManager::Configuration smConfig;
        for (auto fs : configuration.segmentFileStorage) {
            smConfig.storage.emplace_back(fs);
        }
        smConfig.platformAPI = configuration.platformAPI;
        smConfig.streamId = streamId;
        _segmentManager = std::make_unique<SegmentManager>(logger, smConfig);
        _packager = std::make_unique<Packager>(logger, _segmentManager.get());
        addHandler(_packager.get());
    }
}
