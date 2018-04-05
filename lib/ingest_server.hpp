#pragma once

#include "rtmp_connection.hpp"
#include "tcp_server.hpp"

using IngestServer = TCPServer<RTMPConnection>;
