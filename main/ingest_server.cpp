#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include <args.hxx>

#include "lib/ingest_server.hpp"

namespace {
    volatile std::sig_atomic_t gSignal = 0;
}

void signalHandler(int signal) {
    gSignal = signal;
}

int main(int argc, const char* argv[]) {
    Logger logger;

    args::ArgumentParser parser("This is the ingest server. It receives RTMP connections, archives the raw streams, and redistributes the streams to the transcoders and CDNs.");
    args::HelpFlag help(parser, "help", "display this help", {'h', "help"});
    args::ValueFlag<std::string> archiveBucket(parser, "s3 bucket name", "the s3 bucket to archive to", {"archive-bucket"});
    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        logger.error(e.what());
        std::cerr << parser;
        return 1;
    }

    std::signal(SIGINT, signalHandler);

    IngestServer s{logger, archiveBucket.Get()};

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
