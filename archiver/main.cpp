#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include <args.hxx>

#include "lib/archiver.hpp"
#include "lib/aws.hpp"
#include "lib/demuxer.hpp"

namespace {
    volatile std::sig_atomic_t gSignal = 0;
}

void signalHandler(int signal) {
    gSignal = signal;
}

Logger gLogger;

int main(int argc, const char* argv[]) {
    args::ArgumentParser parser(
        "This is the archiver. It takes a video and copies it without modification to an S3 bucket."
    );
    parser.helpParams.width = 120;
    args::HelpFlag help(parser, "help", "display this help", {'h', "help"});
    args::ValueFlag<std::string> input(parser, "input", "input path", {'i', "input"});
    args::ValueFlag<std::string> s3Bucket(parser, "s3-bucket", "s3 bucket to archive to", {"s3-bucket"});
    args::ValueFlag<std::string> s3Prefix(parser, "s3-prefix", "s3 prefix to archive to", {"s3-prefix"});
    args::ValueFlag<std::string> s3Endpoint(parser, "s3-endpoint", "s3 endpoint to archive to", {"s3-endpoint"});
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

    InitAWS();

    Aws::Client::ClientConfiguration clientConfiguration;

    auto endpoint = args::get(s3Endpoint);
    auto colon = endpoint.find(":");
    if (colon == std::string::npos) {
        clientConfiguration.scheme = Aws::Http::Scheme::HTTP;
        clientConfiguration.endpointOverride = endpoint;
    } else {
        auto scheme = endpoint.substr(0, colon);
        auto authorityPart = colon + 1;
        if (authorityPart + 1 < endpoint.size() && endpoint[authorityPart] == '/' && endpoint[authorityPart+1] == '/') {
            authorityPart += 2;
        }
        if (scheme == "http") {
            clientConfiguration.scheme = Aws::Http::Scheme::HTTP;
        } else if (scheme == "https") {
            clientConfiguration.scheme = Aws::Http::Scheme::HTTPS;
        }
        clientConfiguration.endpointOverride = endpoint.substr(authorityPart);
    }

    auto client = std::make_shared<Aws::S3::S3Client>(clientConfiguration, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);

    S3FileStorage storage(gLogger, args::get(s3Bucket), args::get(s3Prefix), client);

    Archiver archiver{gLogger, &storage, "{:010}"};
    Demuxer demuxer{gLogger, input.Get(), &archiver};

    auto lastReportTime = std::chrono::steady_clock::now();

    while (!demuxer.isDone()) {
        if (gSignal == SIGINT) {
            gLogger.info("signal received");
            break;
        }
        auto now = std::chrono::steady_clock::now();
        if (now - lastReportTime > std::chrono::seconds(5)) {
            gLogger.info("frames processed: {} / {} ({:.2f}%)", demuxer.framesDemuxed(), demuxer.totalFrameCount(), 100.0 * demuxer.framesDemuxed() / demuxer.totalFrameCount());
            lastReportTime = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
