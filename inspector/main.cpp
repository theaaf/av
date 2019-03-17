#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include <args.hxx>

#include "lib/demuxer.hpp"
#include "lib/mpeg4.hpp"
#include "lib/h26x/h264.hpp"
#include "lib/h26x/sei.hpp"

namespace {
    volatile std::sig_atomic_t gSignal = 0;
}

void signalHandler(int signal) {
    gSignal = signal;
}

class Inspector : public EncodedAVHandler {
public:
    explicit Inspector(Logger logger) : _logger{std::move(logger)} {}
    virtual ~Inspector() {}

    virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
        auto config = std::make_unique<AVCDecoderConfigurationRecord>();
        if (!config->decode(data, len)) {
            _logger.error("unable to decode video config");
            return;
        }
        _logger.with(
            "profile", config->avcProfileIndication,
            "profile_compatibility", config->profileCompatibility,
            "level", config->avcLevelIndication
        ).info("video config");
        _videoConfig = std::move(config);
    }

    virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
        _logger.with(
            "pts", pts.count(),
            "dts", dts.count()
        ).info("video access unit");
        if (!_videoConfig) {
            return;
        }
        if (!h264::IterateAVCC(data, len, _videoConfig->lengthSizeMinusOne + 1, [this](const void* data, size_t len) {
            printf("==== NALU ====\n");

            h264::bitstream bs{data, len};

            h264::nal_unit nalu;
            auto err = nalu.decode(&bs, len);
            if (err) {
                _logger.error(err.message);
                return;
            }

            switch (nalu.nal_unit_type) {
            case h264::NALUnitType::SEI: {
                printf("nalu type: %d (sei)\n", int(nalu.nal_unit_type));

                h264::sei_rbsp rbsp;
                h264::bitstream bs{nalu.rbsp_byte.data(), nalu.rbsp_byte.size()};

                auto err = rbsp.decode(&bs);
                if (err) {
                    _logger.error(err.message);
                    break;
                }

                for (auto& message : rbsp.sei_message) {
                    printf("  payload type: %d\n", message.payloadType);
                    printf("    ");
                    for (size_t i = 0; i < message.sei_payload.size(); ++i) {
                        if (i > 0 && (i % 16) == 0) {
                            printf("\n    ");
                        }
                        printf("%02x ", message.sei_payload[i]);
                    }
                    printf("\n");
                }

                break;
            }
            default:
                printf("nalu type: %d\n", int(nalu.nal_unit_type));
            }
        })) {
            _logger.error("unable to iterate avcc nalus");
        }
    }

private:
    Logger _logger;
    std::unique_ptr<AVCDecoderConfigurationRecord> _videoConfig;
};

int main(int argc, const char* argv[]) {
    Logger logger;

    args::ArgumentParser parser("This is the inspector. It spits out information about a file.");
    parser.helpParams.width = 120;
    args::HelpFlag help(parser, "help", "display this help", {'h', "help"});
    args::ValueFlag<std::string> input(parser, "input", "input path", {'i', "input"});
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

    Inspector inspector{logger};
    Demuxer demuxer{logger, input.Get(), &inspector};

    while (!demuxer.isDone()) {
        if (gSignal == SIGINT) {
            Logger{}.info("signal received");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
