#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include <args.hxx>
#include <nlohmann/json.hpp>

#include "lib/ingest_server.hpp"

using json = nlohmann::json;

namespace {
    volatile std::sig_atomic_t gSignal = 0;
}

void signalHandler(int signal) {
    gSignal = signal;
}

Logger gLogger;

struct EncodingParser {
    void operator()(const std::string& name, const std::string& value, IngestServer::Configuration::Encoding& destination) {
        try {
            auto encoding = json::parse(value);
            destination.video.bitrate = encoding["video"]["bitrate"].get<int>();
            destination.video.width = encoding["video"]["width"].get<int>();
            destination.video.height = encoding["video"]["height"].get<int>();
            if (encoding["video"]["h264_preset"].is_string()) {
                destination.video.h264Preset = encoding["video"]["h264_preset"].get<std::string>();
            }
            if (encoding["video"]["profile"].is_number()) {
                destination.video.profileIDC = encoding["video"]["profile"].get<int>();
            }
            if (encoding["video"]["level"].is_number()) {
                destination.video.levelIDC = encoding["video"]["level"].get<int>();
            }
        } catch (...) {
            throw args::ParseError("invalid encoding");
        }
    }
};

struct FileStorageParser {
    void operator()(const std::string& name, const std::string& value, std::shared_ptr<FileStorage>& destination) {
        destination = FileStorageForURI(gLogger, value);
        if (!destination) {
            throw args::ParseError("invalid storage uri");
        }
    }
};

int main(int argc, const char* argv[]) {
    json exampleEncoding = {
        {"video", {
            {"bitrate", 4000000},
            {"width", 1280},
            {"height", 720},
            {"h264_preset", "medium"},
            {"profile_idc", 100},
            {"level_idc", 31},
        }},
    };
    args::ArgumentParser parser(
        "This is the ingest server. It receives RTMP connections, archives the raw streams, and redistributes the streams to the transcoders and CDNs.", (
        "Storage URIs can be of the form \"file:my-directory\" or \"s3:my-bucket\". "
        "Encodings are JSON strings of the form " + exampleEncoding.dump() + "."
    ).c_str());
    parser.helpParams.width = 120;
    args::HelpFlag help(parser, "help", "display this help", {'h', "help"});
    args::ValueFlag<std::string> platformURL(parser, "url", "platform url", {"platform-url"});
    args::ValueFlag<std::string> platformAccessToken(parser, "token", "platform access token", {"platform-access-token"});
    args::ValueFlag<std::shared_ptr<FileStorage>, FileStorageParser> archiveStorage(parser, "uri", "uri to archive to", {"archive-storage"});
    args::ValueFlagList<std::shared_ptr<FileStorage>, std::vector, FileStorageParser> segmentStorage(parser, "uri", "uris to write segments to", {"segment-storage"});
    args::ValueFlagList<IngestServer::Configuration::Encoding, std::vector, EncodingParser> encodings(parser, "encoding", "a/v encoding configuration as json (see below)", {"encoding"});
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

    for (auto& encoding : encodings) {
        configuration.encodings.emplace_back(encoding);
    }

    std::unique_ptr<PlatformAPI> platformAPI;
    if (platformURL) {
        platformAPI = std::make_unique<PlatformAPI>(args::get(platformURL)+"/api", args::get(platformAccessToken));
        configuration.platformAPI = platformAPI.get();
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
