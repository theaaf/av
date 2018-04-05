#pragma once

#include "archiver.hpp"
#include "rtmp_connection.hpp"
#include "tcp_server.hpp"

class IngestServer : public TCPServer<RTMPConnection, RTMPConnectionDelegate*>, public RTMPConnectionDelegate {
public:
    explicit IngestServer(Logger logger) : TCPServer(logger, this), _logger{logger} {}
    virtual ~IngestServer() {}

    virtual std::shared_ptr<EncodedAVReceiver> allocateAVReceiver() override {
        return std::make_shared<Archiver>(_logger);
    }

private:
    const Logger _logger;
};
