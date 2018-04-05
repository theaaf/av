#pragma once

#include <asio.hpp>

#include "librtmp/rtmp.h"

#include "logger.hpp"

class RTMPConnection {
public:
    RTMPConnection(Logger logger) : _logger{logger} {}

    void run(int fd, asio::ip::tcp::endpoint remote);
    void cancel();

private:
    Logger _logger;
    std::atomic<bool> _shouldReturn{false};

    uint32_t _nextStreamId{1};

    void _serveRTMP(RTMP* rtmp);
    bool _serveInvoke(RTMP* rtmp, RTMPPacket* packet, const char* body, size_t len);
    bool _servePacket(RTMP* rtmp, RTMPPacket* packet);

    bool _sendConnectResult(RTMP* rtmp, double txn);
    bool _sendResultNumber(RTMP* rtmp, double txn, double n);
    bool _sendOnFCPublish(RTMP* rtmp, uint32_t streamId, const AVal* code, const AVal* description);
    bool _sendOnStatus(RTMP* rtmp, uint32_t streamId, const AVal* code, const AVal* description);

    bool _waitUntilReadable(int fd);
};
