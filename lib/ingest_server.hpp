#pragma once

#include <time.h>

#include <fmt/format.h>

#include "archiver.hpp"
#include "rtmp_connection.hpp"
#include "tcp_server.hpp"

class IngestServer : public TCPServer<RTMPConnection, RTMPConnectionDelegate*>, public RTMPConnectionDelegate {
public:
    explicit IngestServer(Logger logger, std::string archiveBucket = "")
        : TCPServer(logger, this), _logger{std::move(logger)}, _archiveBucket{std::move(archiveBucket)}
    {
        if (_archiveBucket.empty()) {
            _logger.warn("no archive bucket configured. streams will be discarded");
        }
    }

    virtual ~IngestServer() {}

    virtual std::shared_ptr<EncodedAVReceiver> authenticate(const std::string& connectionId) override {
        if (_archiveBucket.empty()) {
            return std::make_shared<EncodedAVReceiver>();
        }

        auto now = std::chrono::system_clock::now();
        auto nowTime = std::chrono::system_clock::to_time_t(now);
        struct tm gmTime;
        gmtime_r(&nowTime, &gmTime);
        return std::make_shared<Archiver>(_logger.with("connection_id", connectionId), _archiveBucket, fmt::format("{}/{:02}/{:02}/{:010}", gmTime.tm_year + 1900, gmTime.tm_mon + 1, gmTime.tm_mday, connectionId)+"/{}");
    }

private:
    const Logger _logger;
    const std::string _archiveBucket;
};
