#pragma once

#include <memory>

#include "archiver.hpp"
#include "encoded_av_splitter.hpp"
#include "file_storage.hpp"
#include "packager.hpp"
#include "rtmp_connection.hpp"
#include "segment_manager.hpp"
#include "tcp_server.hpp"

class IngestServer : public TCPServer<RTMPConnection, RTMPConnectionDelegate*>, public RTMPConnectionDelegate {
public:
    struct Configuration {
        FileStorage* archiveFileStorage;
        std::vector<FileStorage*> segmentFileStorage;
    };

    explicit IngestServer(Logger logger, Configuration configuration = {})
        : TCPServer(logger, this), _logger{std::move(logger)}, _configuration{std::move(configuration)}
    {
        if (_configuration.archiveFileStorage == nullptr && _configuration.segmentFileStorage.empty()) {
            _logger.info("no archive or segment storage configured. streams will be discarded");
        }
    }

    virtual ~IngestServer() {}

    virtual std::shared_ptr<EncodedAVReceiver> authenticate(const std::string& connectionId) override {
        return std::make_shared<Stream>(_logger.with("connection_id", connectionId), _configuration, connectionId);
    }

private:
    const Logger _logger;
    const Configuration _configuration;

    class Stream : public EncodedAVSplitter {
    public:
        Stream(Logger logger, const Configuration& configuratiton, const std::string& connectionId);
        virtual ~Stream() {}

    private:
        std::unique_ptr<Archiver> _archiver;
        SegmentManager _segmentManager;
        Packager _packager;
    };
};
