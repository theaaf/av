#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

#include <asio.hpp>

#include "logger.hpp"

// ConnectionClass should be a class with a run(int, asio::ip::tcp::endpoint) method, to be invoked
// from a new thread, and a cancel() method to cause the run() invocation to return early. The Args
// parameter represents the types of the arguments to be passed to new ConnectionClass instances.
//
// This class is based on the TCPAcceptor class in bittorrent/scraps:
// https://github.com/bittorrent/scraps
template <typename ConnectionClass, typename... Args>
class TCPServer {
public:
    // Any arguments passed to the constructor will be copied and passed to each connection class
    // constructor (including the logger).
    TCPServer(Logger logger, const Args&... args) : _logger{logger}, _args(args...), _acceptor(_service) {}
    TCPServer(const TCPServer& other) = delete;
    TCPServer& operator=(const TCPServer& other) = delete;
    ~TCPServer() { _stop(); }

    // Starts the server on the specified address and port.
    bool start(asio::ip::address address, uint16_t port) {
        std::unique_lock<std::mutex> lock(_startStopMutex);
        _stop();
        _service.reset();
        _isCancelled = false;

        try {
            _acceptor.open(address.is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4());
            _acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
            _acceptor.bind(asio::ip::tcp::endpoint(address, port));
            _acceptor.listen();
        } catch (std::exception& e) {
            _logger.error("exception opening tcp server acceptor: {}", e.what());
            return false;
        }

        _work = std::make_unique<asio::io_service::work>(_service);

        _serviceThread = std::thread([&] {
            _logger.info("starting tcp server worker thread");
            while (!_isCancelled) {
                try {
                    _service.run();
                    break;
                } catch (std::exception& e) {
                    _logger.error("exception in tcp server worker thread: {}", e.what());
                } catch (...) {
                    _logger.error("unknown exception in tcp server worker thread");
                }
            }
            _logger.info("exiting tcp server worker thread");
        });

        _connectionCleaner = std::thread([&] {
            while (!_isCancelled) {
                std::unique_lock<std::mutex> lock{_mutex};
                _cv.wait(lock, [&]{return !_completeConnections.empty() || _isCancelled;});

                for (auto connection : _completeConnections) {
                    _connections.erase(connection);
                }

                auto completeConnections = std::move(_completeConnections);
                _completeConnections.clear();
                lock.unlock();
            }
        });

        auto socket = std::make_shared<asio::ip::tcp::socket>(_service);
        _acceptor.async_accept(*socket, std::bind(ArgsHelper<sizeof...(Args)>::AcceptHandler(), this, socket, std::placeholders::_1));

        return true;
    }

    // Stops the server.
    void stop() {
        std::lock_guard<std::mutex> lock(_startStopMutex);
        _stop();
    }

    template <typename Callback>
    void iterateConnections(Callback&& callback) {
        std::vector<std::weak_ptr<ConnectionClass>> connections;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            for (auto& kv : _connections) {
                connections.emplace_back(kv.second->connectionClass);
            }
        }
        for (auto& connection : connections) {
            if (auto shared = connection.lock()) {
                callback(*shared);
            }
        }
    }

private:
    std::atomic<bool> _isCancelled{false};

    std::mutex _mutex;
    std::mutex _startStopMutex;

    Logger _logger;
    std::tuple<Args...> _args;

    asio::io_service _service;
    std::unique_ptr<asio::io_service::work> _work;
    std::thread _serviceThread;

    asio::ip::tcp::acceptor _acceptor;

    struct Connection {
        ~Connection() {
            auto connection = connectionClass.lock();
            if (connection) {
                connection->cancel();
            }

            if (connectionThread.joinable()) {
                connectionThread.join();
            }
        }
        std::weak_ptr<ConnectionClass> connectionClass;
        std::thread connectionThread;
    };
    std::unordered_map<ConnectionClass*, std::shared_ptr<Connection>> _connections;
    std::vector<ConnectionClass*> _completeConnections;
    std::condition_variable _cv;
    std::thread _connectionCleaner;

    template <int N, int... S>
    struct ArgsHelper : ArgsHelper<N - 1, N - 1, S...> {};

    template <int... S>
    struct ArgsHelper<0, S...> {
        typedef void (TCPServer::*AcceptHandlerPointer)(std::shared_ptr<asio::ip::tcp::socket>, const asio::error_code&);
        static constexpr AcceptHandlerPointer AcceptHandler() { return &TCPServer::_acceptHandler<S...>; }
    };

    void _stop() {
        _isCancelled = true;
        _cv.notify_one();

        try {
            _acceptor.close();
        } catch (...) {}

        _work.reset();

        if (_serviceThread.joinable()) {
            _serviceThread.join();
        }

        if (_connectionCleaner.joinable()) {
            _cv.notify_one();
            _connectionCleaner.join();
        }

        _connections.clear();
        _completeConnections.clear();
    }

    template <int... ArgIndices>
    void _acceptHandler(std::shared_ptr<asio::ip::tcp::socket> socket, const asio::error_code& error) {
        if (_isCancelled || error == asio::error::operation_aborted) {
            return;
        }
        auto nextSocket = std::make_shared<asio::ip::tcp::socket>(_service);
        _acceptor.async_accept(*nextSocket, std::bind(&TCPServer::_acceptHandler<ArgIndices...>, this, nextSocket, std::placeholders::_1));

        if (error) {
            _logger.error("accept error: {}", error.message().c_str());
            return;
        }

        int fd = dup(socket->native_handle());
        if (fd == -1) {
            _logger.error("error duplicating socket handle (errno = {})", errno);
            return;
        }
        auto remote = socket->remote_endpoint();

        try {
            socket->close();
        } catch (...) {}

        auto connection = std::make_shared<Connection>();
        auto connectionClass = std::make_shared<ConnectionClass>(_logger, std::get<ArgIndices>(_args)...);
        connection->connectionClass = connectionClass;
        connection->connectionThread = std::thread([this, connectionClass, fd, remote]() {
            connectionClass->run(fd, remote);
            {
                std::lock_guard<std::mutex> lock{_mutex};
                _completeConnections.emplace_back(connectionClass.get());
            }
            _cv.notify_one();
        });

        std::lock_guard<std::mutex> lock{_mutex};
        _connections.emplace(connectionClass.get(), connection);
    }
};
