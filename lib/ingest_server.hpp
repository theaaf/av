#pragma once

#include <future>
#include <utility>

#include <asio.hpp>

#include "logger.hpp"

/**
 * IngestServer is the high level wrapper for the ingest server's functionality.
 */
class IngestServer {
public:
    struct Config {
        Logger* logger = nullptr;
    };

    explicit IngestServer(Config config) : _config{std::move(config)} {}

    void start();
    void stop();

private:
    const Config _config;

    asio::io_service _service;
    asio::ip::tcp::acceptor _acceptor{_service};

    void _run();
    std::future<void> _result;
};
