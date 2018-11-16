#include "rtmp_connection.hpp"

#include <unistd.h>

#include "librtmp/log.h"
#include "mpeg4.hpp"
#include "utility.hpp"

#define STR2AVAL(av, str) av.av_val = const_cast<char*>(str); av.av_len = strlen(av.av_val) // NOLINT(cppcoreguidelines-pro-type-const-cast)
#define SAVCX(name, x) static const AVal name = {const_cast<char*>(x), sizeof(x)-1} // NOLINT(cppcoreguidelines-pro-type-const-cast)
#define SAVC(x) SAVCX(av_##x, #x)

SAVC(connect);
SAVC(objectEncoding);
SAVC(_result);
SAVC(createStream);
SAVC(releaseStream);
SAVC(fmsVer);
SAVC(capabilities);
SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
SAVC(publish);
SAVC(onStatus);
SAVC(FCPublish);
SAVC(onFCPublish);
SAVC(status);

SAVCX(av_NetStream_Publish_Start, "NetStream.Publish.Start");
SAVCX(av_started_publishing, "started publishing");

void format_arg(fmt::BasicFormatter<char> &f, const char*& format, const AVal& v) {
    if (v.av_len > 0) {
        f.writer().write("{:{}}", v.av_val, v.av_len);
    } else {
        f.writer().write("");
    }
}

struct RTMPLogger {
    RTMPLogger() {
        RTMP_LogSetCallback(&Callback);
    }

    static void Callback(int level, const char* format, va_list args) {
        char buf[500];

        va_list tmp;
        va_copy(tmp, args);
        auto n = vsnprintf(buf, sizeof(buf), format, tmp);
        va_end(tmp);

        if (n < 0) {
            Logger{}.error("rtmp log error: {}", format);
        } else if (static_cast<size_t>(n) < sizeof(buf)) {
            Log(level, buf);
        } else {
            auto buf = std::vector<char>(n+1);
            vsnprintf(&buf[0], buf.size(), format, args);
            Log(level, buf.data());
        }
    }

    static void Log(int level, const char* str) {
        switch (level) {
        case RTMP_LOGCRIT:
        case RTMP_LOGERROR:
            Logger{}.error("[rtmp] {}", str);
            break;
        case RTMP_LOGWARNING:
            Logger{}.warn("[rtmp] {}", str);
            break;
        case RTMP_LOGINFO:
            Logger{}.info("[rtmp] {}", str);
            break;
        }
    }
} _rtmpLogger;

void RTMPConnection::run(int fd, asio::ip::tcp::endpoint remote) {
    _connectionId = GenerateUUID();

    if (!_waitUntilReadable(fd)) {
        // probably just a health check
        close(fd);
        return;
    }

    _logger = _logger.with("connection_id", _connectionId);
    _logger.with("remote", remote).info("handled rtmp connection");

    RTMP* rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    rtmp->m_sb.sb_socket = fd;
    std::function<int()> interruptCallback = [this, fd]() {
        return _waitUntilReadable(fd) ? 0 : -1;
    };
    rtmp->m_sb.sb_interrupt_callback = [](void* f){return (*reinterpret_cast<std::function<int()>*>(f))(); };
    rtmp->m_sb.sb_context = &interruptCallback;

    _serveRTMP(rtmp);

    _logger.info("closing connection");

    RTMP_Close(rtmp);
    RTMP_Free(rtmp);

    _logger.info("connection closed");
}

void RTMPConnection::cancel() {
    _logger.info("canceling rtmp connection");
    _shouldReturn = true;
}

void RTMPConnection::_serveRTMP(RTMP* rtmp) {
    if (!RTMP_Serve(rtmp)) {
        _logger.error("rtmp handshake failed");
        return;
    }

    RTMPPacket packet{0};

    while (!_shouldReturn && RTMP_IsConnected(rtmp) && RTMP_ReadPacket(rtmp, &packet)) {
        if (!RTMPPacket_IsReady(&packet)) {
            continue;
        }
        if (!_servePacket(rtmp, &packet)) {
            break;
        }
        RTMPPacket_Free(&packet);
    }

    RTMPPacket_Free(&packet);
}

bool RTMPConnection::_serveInvoke(RTMP* rtmp, RTMPPacket* packet, const char* body, size_t len) {
    if (len < 1 || body[0] != 2) {
        _logger.error("expected 0x02 lead byte in invoke packet");
        return false;
    }

    AMFObject obj;
    if (AMF_Decode(&obj, body, len, false) < 0) {
        _logger.error("unable to decode invoke packet");
        return false;
    }

    AVal method;
    AMFProp_GetString(AMF_GetProp(&obj, nullptr, 0), &method);
    auto txn = AMFProp_GetNumber(AMF_GetProp(&obj, nullptr, 1));

    if (AVMATCH(&method, &av_connect)) {
        if (_avHandler) {
            _logger.error("second connect handled");
            return false;
        }

        AMFObject cobj;
        AMFProp_GetObject(AMF_GetProp(&obj, nullptr, 2), &cobj);
        for (int i = 0; i < cobj.o_num; ++i) {
            auto pname = cobj.o_props[i].p_name;
            double number = 0.0;
            AVal string{0};

            if (cobj.o_props[i].p_type == AMF_STRING) {
                string = cobj.o_props[i].p_vu.p_aval;
                _logger.with("key", pname, "value", string).info("handled connect property");
            } else if (cobj.o_props[i].p_type == AMF_NUMBER) {
                number = cobj.o_props[i].p_vu.p_number;
                _logger.with("key", pname, "value", number).info("handled connect property");
            } else {
                _logger.with("key", pname).info("handled connect object property");
            }

            if (AVMATCH(&pname, &av_objectEncoding)) {
                rtmp->m_fEncoding = number;
            }
        }

        _avHandler = _delegate->authenticate(_connectionId);
        if (!_avHandler) {
            _logger.info("authentication failed");
            return false;
        }
        return _sendConnectResult(rtmp, txn);
    } else if (AVMATCH(&method, &av_createStream)) {
        return _sendResultNumber(rtmp, txn, _nextStreamId++);
    } else if (AVMATCH(&method, &av_releaseStream)) {
        return true;
    } else if (AVMATCH(&method, &av_FCPublish)) {
        AVal name{0};
        AMFProp_GetString(AMF_GetProp(&obj, nullptr, 3), &name);
        return _sendOnFCPublish(rtmp, packet->m_nInfoField2, &av_NetStream_Publish_Start, &name);
    } else if (AVMATCH(&method, &av_publish)) {
        AVal name, type;
        AMFProp_GetString(AMF_GetProp(&obj, nullptr, 3), &name);
        AMFProp_GetString(AMF_GetProp(&obj, nullptr, 4), &type);
        _logger.with("name", name, "type", type).info("handled publish");
        return _sendOnStatus(rtmp, packet->m_nInfoField2, &av_NetStream_Publish_Start, &av_started_publishing);
    } else {
        _logger.warn("unknown invoke method: {}", method);
    }

    return true;
}

bool RTMPConnection::_servePacket(RTMP* rtmp, RTMPPacket* packet) {
    switch (packet->m_packetType) {
    case RTMP_PACKET_TYPE_CHUNK_SIZE:
        if (packet->m_nBodySize >= 4) {
            rtmp->m_inChunkSize = AMF_DecodeInt32(packet->m_body);
        }
        return true;
    case RTMP_PACKET_TYPE_INVOKE:
        return _serveInvoke(rtmp, packet, packet->m_body, packet->m_nBodySize);
    case RTMP_PACKET_TYPE_AUDIO:
        if (!_avHandler || packet->m_nBodySize < 2 || static_cast<uint8_t>(packet->m_body[0]) != 0xaf) {
            _logger.error("invalid audio packet");
            return false;
        } else if (packet->m_body[1] == 0) {
            MPEG4AudioSpecificConfig config;
            if (!config.decode(packet->m_body + 2, packet->m_nBodySize - 2)) {
                _logger.error("unable to decode audio config");
                return false;
            }
            _logger.with(
                "object_type", config.objectType,
                "frequency", config.frequency,
                "channel_configuration", config.channelConfiguration
            ).info("handled audio config");
            _avHandler->handleEncodedAudioConfig(packet->m_body + 2, packet->m_nBodySize - 2);
        } else if (packet->m_body[1] == 1) {
            auto pts = std::chrono::milliseconds{packet->m_nTimeStamp};
            _avHandler->handleEncodedAudio(pts, packet->m_body + 2, packet->m_nBodySize - 2);
        }
        return true;
    case RTMP_PACKET_TYPE_VIDEO: {
        if (!_avHandler || packet->m_nBodySize < 5 || (packet->m_body[0] & 0x0f) != 0x07) {
            _logger.error("invalid video packet");
            return false;
        }
        auto ctPtr = reinterpret_cast<const uint8_t*>(packet->m_body + 2);
        auto compositionTimeOffset = std::chrono::milliseconds{(static_cast<unsigned int>(ctPtr[0]) << 16) | (static_cast<unsigned int>(ctPtr[1]) << 8) | ctPtr[2]};
        if (packet->m_body[1] == 0) {
            AVCDecoderConfigurationRecord config;
            if (!config.decode(packet->m_body + 5, packet->m_nBodySize - 5)) {
                _logger.error("unable to decode video config");
                return false;
            }
            _logger.with(
                "profile", config.avcProfileIndication,
                "level", config.avcLevelIndication,
                "length_size", config.lengthSizeMinusOne + 1
            ).info("handled video config");
            _avHandler->handleEncodedVideoConfig(packet->m_body + 5, packet->m_nBodySize - 5);
        } else if (packet->m_body[1] == 1) {
            auto dts = std::chrono::milliseconds{packet->m_nTimeStamp};
            auto pts = dts + compositionTimeOffset;
            _avHandler->handleEncodedVideo(pts, dts, packet->m_body + 5, packet->m_nBodySize - 5);
        }
        return true;
    }
    default:
        _logger.warn("handled unknown rtmp packet type {}", packet->m_packetType);
    }
    return true;
}

bool RTMPConnection::_sendConnectResult(RTMP* rtmp, double txn) {
    RTMPPacket packet;
    char pbuf[384], *pend = pbuf+sizeof(pbuf);
    AMFObject obj;
    AMFObjectProperty p, op;
    AVal av;

    packet.m_nChannel = 0x03;
    packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "FMS/3,5,1,525");
    enc = AMF_EncodeNamedString(enc, pend, &av_fmsVer, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_capabilities, 31.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_mode, 1.0);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "status");
    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
    STR2AVAL(av, "NetConnection.Connect.Success");
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
    STR2AVAL(av, "Connection succeeded.");
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, rtmp->m_fEncoding);
    STR2AVAL(p.p_name, "version");
    STR2AVAL(p.p_vu.p_aval, "3,5,1,525");
    p.p_type = AMF_STRING;
    obj.o_num = 1;
    obj.o_props = &p;
    op.p_type = AMF_OBJECT;
    STR2AVAL(op.p_name, "data");
    op.p_vu.p_object = obj;
    enc = AMFProp_Encode(&op, enc, pend);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(rtmp, &packet, false);
}

bool RTMPConnection::_sendResultNumber(RTMP* rtmp, double txn, double n) {
    RTMPPacket packet;
    char pbuf[256], *pend = pbuf+sizeof(pbuf);

    packet.m_nChannel = 0x03;
    packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_NULL;
    enc = AMF_EncodeNumber(enc, pend, n);

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(rtmp, &packet, false);
}

bool RTMPConnection::_sendOnFCPublish(RTMP* rtmp, uint32_t streamId, const AVal* code, const AVal* description) {
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf+sizeof(pbuf);

    packet.m_nChannel = 0x03;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = streamId;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onFCPublish);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;

    *enc++ = AMF_OBJECT;
    enc = AMF_EncodeNamedString(enc, pend, &av_code, code);
    if (!enc) { return false; }
    enc = AMF_EncodeNamedString(enc, pend, &av_description, description);
    if (!enc || pend - enc < 3) { return false; }
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(rtmp, &packet, false);
}

bool RTMPConnection::_sendOnStatus(RTMP* rtmp, uint32_t streamId, const AVal* code, const AVal* description) {
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf+sizeof(pbuf);

    packet.m_nChannel = 0x03;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = streamId;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;

    *enc++ = AMF_OBJECT;
    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, code);
    if (!enc) { return false; }
    enc = AMF_EncodeNamedString(enc, pend, &av_description, description);
    if (!enc || pend - enc < 3) { return false; }
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(rtmp, &packet, false);
}

bool RTMPConnection::_waitUntilReadable(int fd) {
    while (!_shouldReturn) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        struct timeval timeout{0};
        timeout.tv_sec = 1;
        if (select(fd + 1, &rfds, nullptr, nullptr, &timeout) > 0) {
            return true;
        }
    }

    return false;
}
