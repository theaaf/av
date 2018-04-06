#pragma once

#include <asio.hpp>

#include "librtmp/rtmp.h"

#include "encoded_av_receiver.hpp"
#include "logger.hpp"

struct RTMPConnectionDelegate {
    virtual ~RTMPConnectionDelegate() {}

    virtual std::shared_ptr<EncodedAVReceiver> authenticate(const std::string& connectionId) = 0;
};

class RTMPConnection {
public:
    RTMPConnection(Logger logger, RTMPConnectionDelegate* delegate) : _logger{logger}, _delegate{delegate} {}

    void run(int fd, asio::ip::tcp::endpoint remote);
    void cancel();

private:
    Logger _logger;
    RTMPConnectionDelegate* const _delegate;
    std::string _connectionId;

    std::atomic<bool> _shouldReturn{false};

    uint32_t _nextStreamId{1};
    std::shared_ptr<EncodedAVReceiver> _avReceiver;

    void _serveRTMP(RTMP* rtmp);
    bool _serveInvoke(RTMP* rtmp, RTMPPacket* packet, const char* body, size_t len);
    bool _servePacket(RTMP* rtmp, RTMPPacket* packet);

    bool _sendConnectResult(RTMP* rtmp, double txn);
    bool _sendResultNumber(RTMP* rtmp, double txn, double n);
    bool _sendOnFCPublish(RTMP* rtmp, uint32_t streamId, const AVal* code, const AVal* description);
    bool _sendOnStatus(RTMP* rtmp, uint32_t streamId, const AVal* code, const AVal* description);

    bool _waitUntilReadable(int fd);
};
