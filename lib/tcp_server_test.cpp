#include <gtest/gtest.h>

#include "logger_test.hpp"
#include "tcp_server.hpp"

struct TestConnection {
    TestConnection(Logger logger) {}
    void run(int fd, asio::ip::tcp::endpoint remote) {}
    void cancel() {}
};

TEST(TCPServer, inUsePorts) {
    TestLogDestination logDestination;
    TCPServer<TestConnection> a(&logDestination), b(Logger::Void), c(Logger::Void);
    ASSERT_TRUE(a.start(asio::ip::address::from_string("127.0.0.1"), 6500));
    ASSERT_FALSE(b.start(asio::ip::address::from_string("127.0.0.1"), 6500));
    ASSERT_TRUE(b.start(asio::ip::address::from_string("127.0.0.1"), 6501));
    ASSERT_FALSE(c.start(asio::ip::address::from_string("127.0.0.1"), 6500));
    ASSERT_FALSE(c.start(asio::ip::address::from_string("127.0.0.1"), 6501));
    ASSERT_TRUE(c.start(asio::ip::address::from_string("127.0.0.1"), 6502));
}
