#include <chrono>
#include <csignal>
#include <thread>

#include "lib/ingest_server.hpp"

namespace {
    volatile std::sig_atomic_t gSignal = 0;
}

void signalHandler(int signal) {
    gSignal = signal;
}

int main(int argc, const char* argv[]) {
    std::signal(SIGINT, signalHandler);

    IngestServer s{Logger{}};

    if (!s.start(asio::ip::address_v4::any(), 1935)) {
        Logger{}.error("unable to start ingest server");
        return 1;
    }

    while (gSignal != SIGINT) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger{}.info("signal received");
    s.stop();

    return 0;
}
