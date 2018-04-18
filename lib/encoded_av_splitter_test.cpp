#include <gtest/gtest.h>

#include "encoded_av_splitter.hpp"
#include "encoded_av_handler_test.hpp"

TEST(EncodedAudioSplitter, splitting) {
    TestEncodedAVHandler handlerA, handlerB;

    EncodedAudioSplitter splitter(&handlerA, &handlerB);

    splitter.handleEncodedAudioConfig("foo", 3);
    EXPECT_EQ(1, handlerA.handledAudioConfig.size());
    EXPECT_EQ(1, handlerB.handledAudioConfig.size());

    splitter.handleEncodedAudio(std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, handlerA.handledAudio.size());
    EXPECT_EQ(1, handlerB.handledAudio.size());

    splitter.handleEncodedAudio(std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, handlerA.handledAudio.size());
    EXPECT_EQ(2, handlerB.handledAudio.size());
}

TEST(EncodedVideoSplitter, splitting) {
    TestEncodedAVHandler handlerA, handlerB;

    EncodedVideoSplitter splitter(&handlerA, &handlerB);

    splitter.handleEncodedVideoConfig("foo", 3);
    EXPECT_EQ(1, handlerA.handledVideoConfig.size());
    EXPECT_EQ(1, handlerB.handledVideoConfig.size());

    splitter.handleEncodedVideo(std::chrono::microseconds(1), std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, handlerA.handledVideo.size());
    EXPECT_EQ(1, handlerB.handledVideo.size());

    splitter.handleEncodedVideo(std::chrono::microseconds(2), std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, handlerA.handledVideo.size());
    EXPECT_EQ(2, handlerB.handledVideo.size());
}

TEST(EncodedAVSplitter, splitting) {
    TestEncodedAVHandler handlerA, handlerB;

    EncodedAVSplitter splitter(&handlerA, &handlerB);
    EncodedAVHandler* abstractSplitter{&splitter};

    abstractSplitter->handleEncodedAudioConfig("foo", 3);
    EXPECT_EQ(1, handlerA.handledAudioConfig.size());
    EXPECT_EQ(1, handlerB.handledAudioConfig.size());

    abstractSplitter->handleEncodedAudio(std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, handlerA.handledAudio.size());
    EXPECT_EQ(1, handlerB.handledAudio.size());

    abstractSplitter->handleEncodedAudio(std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, handlerA.handledAudio.size());
    EXPECT_EQ(2, handlerB.handledAudio.size());

    abstractSplitter->handleEncodedVideoConfig("foo", 3);
    EXPECT_EQ(1, handlerA.handledVideoConfig.size());
    EXPECT_EQ(1, handlerB.handledVideoConfig.size());

    abstractSplitter->handleEncodedVideo(std::chrono::microseconds(1), std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, handlerA.handledVideo.size());
    EXPECT_EQ(1, handlerB.handledVideo.size());

    abstractSplitter->handleEncodedVideo(std::chrono::microseconds(2), std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, handlerA.handledVideo.size());
    EXPECT_EQ(2, handlerB.handledVideo.size());
}
