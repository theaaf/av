#include <gtest/gtest.h>

#include <asio.hpp>

#include "ingest_server.hpp"
#include "logger_test.hpp"

TEST(IngestServer, tcpHealthCheck) {
    TestLogDestination logDestination;
    IngestServer server{&logDestination};
    ASSERT_TRUE(server.start(asio::ip::address_v4::loopback(), 1935));

    {
        asio::io_service service;
        asio::ip::tcp::socket s{service};
        s.connect(asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), 1935));
    }

    server.stop();
}
