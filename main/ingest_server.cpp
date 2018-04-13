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

Logger gLogger;

struct FileStorageParser {
    void operator()(const std::string& name, const std::string& value, std::shared_ptr<FileStorage>& destination) {
        destination = FileStorageForURI(gLogger, value);
        if (!destination) {
            throw args::ParseError("invalid storage uri");
        }
    }
};

int main(int argc, const char* argv[]) {
    args::ArgumentParser parser(
        "This is the ingest server. It receives RTMP connections, archives the raw streams, and redistributes the streams to the transcoders and CDNs.",
        "Storage URIs can be of the form \"file:my-directory\" or \"s3:my-bucket\"."
    );
    args::HelpFlag help(parser, "help", "display this help", {'h', "help"});
    args::ValueFlag<std::shared_ptr<FileStorage>, FileStorageParser> archiveStorage(parser, "uri", "uri to archive to", {"archive-storage"});
    args::ValueFlagList<std::shared_ptr<FileStorage>, std::vector, FileStorageParser> segmentStorage(parser, "uri", "uris to write segments to", {"segment-storage"});
    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        gLogger.error(e.what());
        std::cerr << parser;
        return 1;
    }

    IngestServer::Configuration configuration;
    if (archiveStorage) {
        configuration.archiveFileStorage = args::get(archiveStorage).get();
    }
    for (auto storage : segmentStorage) {
        configuration.segmentFileStorage.emplace_back(storage.get());
    }

    IngestServer s{gLogger, configuration};

    std::signal(SIGINT, signalHandler);

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
