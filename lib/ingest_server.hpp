#pragma once

#include <memory>

#include "archiver.hpp"
#include "av_splitter.hpp"
#include "encoded_av_splitter.hpp"
#include "file_storage.hpp"
#include "packager.hpp"
#include "platform_api.hpp"
#include "rtmp_connection.hpp"
#include "segmenter.hpp"
#include "segment_manager.hpp"
#include "tcp_server.hpp"
#include "video_decoder.hpp"
#include "video_encoder.hpp"

class IngestServer : public TCPServer<RTMPConnection, RTMPConnectionDelegate*>, public RTMPConnectionDelegate {
public:
    struct Configuration {
        FileStorage* archiveFileStorage = nullptr;
        std::vector<FileStorage*> segmentFileStorage;
        PlatformAPI* platformAPI = nullptr;
        std::string gameId;

        struct Encoding {
            VideoEncoderConfiguration video;
        };

        std::vector<Encoding> encodings;
    };

    IngestServer(Logger logger, Configuration configuration)
        : TCPServer(logger, this), _logger{std::move(logger)}, _configuration{std::move(configuration)}
    {
        if (_configuration.archiveFileStorage == nullptr && _configuration.segmentFileStorage.empty()) {
            _logger.info("no archive or segment storage configured. streams will be discarded");
        }
        if (!_configuration.segmentFileStorage.empty() && _configuration.encodings.empty()) {
            _logger.warn("segment storage was provided without encodings. segments will not be created");
        }
    }

    virtual ~IngestServer() {}

    virtual std::shared_ptr<EncodedAVHandler> authenticate(const std::string& connectionId) override;

private:
    const Logger _logger;
    const Configuration _configuration;

    class Stream : public EncodedAVSplitter {
    public:
        Stream(Logger logger, const Configuration& configuration, const std::string& connectionId);
        virtual ~Stream() {}

        void addEncoding(size_t index, Configuration::Encoding configuration, std::string streamId = "");

    private:
        Logger _logger;
        Configuration _configuration;

        std::unique_ptr<Archiver> _archiver;

        struct Encoding {
            Encoding(Logger logger, SegmentManager::Configuration smConfiguration, VideoEncoderConfiguration encoderConfiguration)
                : segmentManager{logger, std::move(smConfiguration)}
            {
                switch(encoderConfiguration.codec) {
                    case VideoCodec::x264:
                        packager = std::make_unique<H264Packager>(logger, &segmentManager);
                        videoEncoder = std::make_shared<H264VideoEncoder>(logger, packager.get(), std::move(encoderConfiguration));
                        break;
                    case VideoCodec::x265:
                        packager = std::make_unique<H265Packager>(logger, &segmentManager);
                        videoEncoder = std::make_shared<H265VideoEncoder>(logger, packager.get(), std::move(encoderConfiguration));
                        break;
                    default:
                        logger.error("encoderConfiguration.codec is null. expect a segfault soon");
                        break;
                }
            }

            SegmentManager segmentManager;
            std::shared_ptr<Packager> packager;
            std::shared_ptr<VideoEncoder> videoEncoder;
        };

        std::vector<std::unique_ptr<Encoding>> _encodings;
        VideoSplitter _decodedSegmentSplitter;
        std::unique_ptr<VideoDecoder> _videoDecoder;
        EncodedAVSplitter _segmentSplitter;
        std::unique_ptr<Segmenter> _segmenter;
    };
};
