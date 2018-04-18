#include <gtest/gtest.h>

#include "encoded_av_splitter.hpp"
#include "encoded_av_handler_test.hpp"
#include "logger_test.hpp"
#include "video_decoder.hpp"

TEST(VideoDecoder, decoding) {
    TestLogDestination logDestination;
    VideoDecoder decoder{&logDestination};
    EncodedAVSplitter avHandler;
    avHandler.addHandler(&decoder);

    ExerciseEncodedAVHandler(&avHandler);
}
