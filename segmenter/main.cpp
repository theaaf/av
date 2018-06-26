#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include <args.hxx>
#include <nlohmann/json.hpp>

#include "lib/av_splitter.hpp"
#include "lib/demuxer.hpp"
#include "lib/encoded_av_splitter.hpp"
#include "lib/file_storage.hpp"
#include "lib/logger.hpp"
#include "lib/packager.hpp"
#include "lib/segmenter.hpp"
#include "lib/segment_manager.hpp"
#include "lib/video_decoder.hpp"
#include "lib/video_encoder.hpp"

using json = nlohmann::json;

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

struct EncodingConfiguration {
    VideoEncoder::Configuration video;
};

struct EncodingParser {
    void operator()(const std::string& name, const std::string& value, EncodingConfiguration& destination) {
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

struct EncodingResource {
    EncodingResource(Logger logger, SegmentManager::Configuration smConfiguration, VideoEncoder::Configuration encoderConfiguration)
        : segmentManager{logger, std::move(smConfiguration)}
        , packager{logger, &segmentManager}
        , videoEncoder{logger, &packager, std::move(encoderConfiguration)}
    {}

    SegmentManager segmentManager;
    Packager packager;
    VideoEncoder videoEncoder;
};

int main(int argc, const char* argv[]) {
    args::ArgumentParser parser(
        "This is the segmenter. It can be used to manually segment video in faster-than-realtime for testing purposes.",
        "Storage URIs can be of the form \"file:my-directory\" or \"s3:my-bucket\". "
    );
    parser.helpParams.width = 120;
    args::HelpFlag help(parser, "help", "display this help", {'h', "help"});
    args::ValueFlag<std::string> input(parser, "input", "input path", {'i', "input"});
    args::ValueFlag<std::shared_ptr<FileStorage>, FileStorageParser> segmentStorage(parser, "uri", "uri to write segments to", {"segment-storage"});
    args::ValueFlagList<EncodingConfiguration, std::vector, EncodingParser> encodings(parser, "encoding", "a/v encoding configuration as json (see ingest-server)", {"encoding"});
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

    std::vector<std::unique_ptr<EncodingResource>> encodingResources;

    VideoSplitter decodedSegmentSplitter;
    VideoDecoder videoDecoder{gLogger, &decodedSegmentSplitter};
    EncodedAVSplitter segmentSplitter;
    segmentSplitter.addHandler(&videoDecoder);

    Segmenter segmenter(gLogger, &segmentSplitter, [&]{
        videoDecoder.flush();
            for (auto& encoding : encodingResources) {
                encoding->videoEncoder.flush();
                encoding->packager.beginNewSegment();
            }
    });

    SegmentManager::Configuration segmentConfig;
    segmentConfig.storage.emplace_back(segmentStorage.Get().get());

    for (auto& encoding : encodings) {
        auto resource = std::make_unique<EncodingResource>(gLogger, segmentConfig, encoding.video);
        segmentSplitter.addHandler(dynamic_cast<EncodedAudioHandler*>(&resource->packager));
        decodedSegmentSplitter.addHandler(&resource->videoEncoder);
        encodingResources.emplace_back(std::move(resource));
    }

    Demuxer demuxer{gLogger, input.Get(), &segmenter};

    auto lastReportTime = std::chrono::steady_clock::now();

    while (!demuxer.isDone()) {
        if (gSignal == SIGINT) {
            Logger{}.info("signal received");
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
