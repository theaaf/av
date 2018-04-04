#include "lib/ingest_server.hpp"

void IngestServer::start() {
    _result = std::async(std::launch::async, [this]{
        _run();
    });
}

void IngestServer::stop() {
    _result.wait();
}

void IngestServer::_run() {
}
