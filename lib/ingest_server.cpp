#include "ingest_server.hpp"

#include <fmt/format.h>

std::shared_ptr<EncodedAVHandler> IngestServer::authenticate(const std::string& connectionId) {
    auto logger = _logger.with("connection_id", connectionId);

    // TODO: actual authentication

    auto stream = std::make_shared<Stream>(logger, _configuration, connectionId);

    for (size_t i = 0; i < _configuration.encodings.size(); ++i) {
        auto& encoding = _configuration.encodings[i];
        std::string streamId;

        if (_configuration.platformAPI) {
            PlatformAPI::AVStream stream;
            stream.bitrate = encoding.video.bitrate;
            stream.codecs = {
                "mp4a.40.2",
                fmt::format("avc1.{:02x}00{:02x}", encoding.video.profileIDC, encoding.video.levelIDC),
            };
            stream.maximumSegmentDuration = std::chrono::seconds(30);
            stream.videoHeight = encoding.video.height;
            stream.videoWidth = encoding.video.width;
            stream.gameId = _configuration.gameId;

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

        stream->addEncoding(i, encoding, streamId);
    }

    return stream;
}

IngestServer::Stream::Stream(Logger logger, const Configuration& configuration, std::string connectionId)
    : _logger{logger}, _configuration{configuration}
{
    if (configuration.archiveFileStorage) {
        _archiver = std::make_unique<Archiver>(
            logger,
            configuration.archiveFileStorage,
            connectionId + "/{:010}"
        );
        addHandler(_archiver.get());
    }

    if (!configuration.segmentFileStorage.empty()) {
        _videoDecoder = std::make_unique<VideoDecoder>(logger, &_decodedSegmentSplitter);
        _segmentSplitter.addHandler(_videoDecoder.get());
        _segmenter = std::make_unique<Segmenter>(logger, &_segmentSplitter, [this]{
            _videoDecoder->flush();
            for (auto& encoding : _encodings) {
                encoding->videoEncoder.flush();
                encoding->packager.beginNewSegment();
            }
        });
        addHandler(_segmenter.get());
    }
}

void IngestServer::Stream::addEncoding(size_t index, Configuration::Encoding configuration, std::string streamId) {
    SegmentManager::Configuration smConfig;
    for (auto fs : _configuration.segmentFileStorage) {
        smConfig.storage.emplace_back(fs);
    }
    smConfig.platformAPI = _configuration.platformAPI;
    smConfig.streamId = streamId;

    auto encoding = std::make_unique<Encoding>(_logger.with("encoding", index), smConfig, configuration.video);
    _segmentSplitter.addHandler(dynamic_cast<EncodedAudioHandler*>(&encoding->packager));
    _decodedSegmentSplitter.addHandler(&encoding->videoEncoder);
    _encodings.emplace_back(std::move(encoding));
}
