#include <gtest/gtest.h>

#include <string>

#include "encoded_av_handler_test.hpp"
#include "encoded_av_splitter.hpp"
#include "file_storage.hpp"
#include "logger_test.hpp"
#include "packager.hpp"
#include "segment_storage.hpp"
#include "video_decoder.hpp"
#include "video_encoder.hpp"

#include <h264/h264.hpp>

TEST(VideoEncoder, encoding) {
    struct Handler : EncodedAVHandler {
        virtual ~Handler() {}

        virtual void handleEncodedVideoConfig(const void* data, size_t len) override {
            auto config = std::make_unique<AVCDecoderConfigurationRecord>();
            ASSERT_TRUE(config->decode(data, len));
            this->config = std::move(config);
        }

        virtual void handleEncodedVideo(std::chrono::microseconds pts, std::chrono::microseconds dts, const void* data, size_t len) override {
            ASSERT_TRUE(config);
            ++frameCount;
            EXPECT_TRUE(h264::IterateAVCC(data, len, config->lengthSizeMinusOne + 1, [&](const void* data, size_t len) {
                EXPECT_GT(len, 0);
            }));
        }

        std::unique_ptr<AVCDecoderConfigurationRecord> config;
        size_t frameCount = 0;
    };

    TestLogDestination logDestination;

    Handler before, after;
    ExerciseEncodedAVHandler(&before);

    {
        VideoEncoder::Configuration configuration;
        configuration.width = 600;
        configuration.height = 480;
        configuration.handler = &after;
        VideoEncoder encoder{&logDestination, configuration};

        {
            VideoDecoder decoder{&logDestination, &encoder};

            EncodedAVSplitter avHandler;
            avHandler.addHandler(&decoder);
            ExerciseEncodedAVHandler(&avHandler);
        }

        EXPECT_LT(before.frameCount - after.frameCount, 30);
    }

    EXPECT_EQ(before.frameCount, after.frameCount);
}

// Want to write a quick before and after to disk to test encodings?
//
// bazel build lib:test && ./bazel-bin/lib/test --gtest_also_run_disabled_tests --gtest_filter='VideoEncoder.*beforeAndAfter'
TEST(VideoEncoder, DISABLED_beforeAndAfter) {
    struct LocalSegmentStorage : SegmentStorage {
        explicit LocalSegmentStorage(std::string directory) : storage{Logger{}, std::move(directory)} {}

        struct Segment : SegmentStorage::Segment {
            explicit Segment(std::shared_ptr<FileStorage::File> f) : f{f} {}

            virtual bool write(const void* data, size_t len) override {
                return f->write(data, len);
            }

            virtual bool close(std::chrono::microseconds duration) override {
                return f->close();
            }

            std::shared_ptr<FileStorage::File> f;
        };

        virtual std::shared_ptr<SegmentStorage::Segment> createSegment(const std::string& extension) override {
            return std::make_shared<Segment>(storage.createFile("segment-" + std::to_string(counter++) + "." + extension));
        }

        int counter = 0;
        LocalFileStorage storage;
    };

    {
        LocalSegmentStorage storage{"test-output/VideoEncoder-before"};
        Packager packager{Logger{}, &storage};
        ExerciseEncodedAVHandler(&packager);
    }

    {
        LocalSegmentStorage storage{"test-output/VideoEncoder-after"};
        Packager packager{Logger{}, &storage};

        VideoEncoder::Configuration configuration;
        configuration.width = 600;
        configuration.height = 480;
        configuration.handler = &packager;
        VideoEncoder encoder{Logger{}, configuration};

        VideoDecoder decoder{Logger{}, &encoder};

        EncodedAVSplitter avHandler;
        avHandler.addHandler(dynamic_cast<EncodedAudioHandler*>(&packager));
        avHandler.addHandler(&decoder);

        ExerciseEncodedAVHandler(&avHandler);
    }
}
