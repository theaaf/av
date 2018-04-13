#include <gtest/gtest.h>

#include "encoded_av_splitter.hpp"
#include "encoded_av_receiver_test.hpp"

TEST(EncodedAudioSplitter, splitting) {
    TestEncodedAVReceiver receiverA, receiverB;

    EncodedAudioSplitter splitter(&receiverA, &receiverB);

    splitter.receiveEncodedAudioConfig("foo", 3);
    EXPECT_EQ(1, receiverA.receivedAudioConfig.size());
    EXPECT_EQ(1, receiverB.receivedAudioConfig.size());

    splitter.receiveEncodedAudio(std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, receiverA.receivedAudio.size());
    EXPECT_EQ(1, receiverB.receivedAudio.size());

    splitter.receiveEncodedAudio(std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, receiverA.receivedAudio.size());
    EXPECT_EQ(2, receiverB.receivedAudio.size());
}

TEST(EncodedVideoSplitter, splitting) {
    TestEncodedAVReceiver receiverA, receiverB;

    EncodedVideoSplitter splitter(&receiverA, &receiverB);

    splitter.receiveEncodedVideoConfig("foo", 3);
    EXPECT_EQ(1, receiverA.receivedVideoConfig.size());
    EXPECT_EQ(1, receiverB.receivedVideoConfig.size());

    splitter.receiveEncodedVideo(std::chrono::microseconds(1), std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, receiverA.receivedVideo.size());
    EXPECT_EQ(1, receiverB.receivedVideo.size());

    splitter.receiveEncodedVideo(std::chrono::microseconds(2), std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, receiverA.receivedVideo.size());
    EXPECT_EQ(2, receiverB.receivedVideo.size());
}

TEST(EncodedAVSplitter, splitting) {
    TestEncodedAVReceiver receiverA, receiverB;

    EncodedAVSplitter splitter(&receiverA, &receiverB);
    EncodedAVReceiver* abstractSplitter{&splitter};

    abstractSplitter->receiveEncodedAudioConfig("foo", 3);
    EXPECT_EQ(1, receiverA.receivedAudioConfig.size());
    EXPECT_EQ(1, receiverB.receivedAudioConfig.size());

    abstractSplitter->receiveEncodedAudio(std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, receiverA.receivedAudio.size());
    EXPECT_EQ(1, receiverB.receivedAudio.size());

    abstractSplitter->receiveEncodedAudio(std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, receiverA.receivedAudio.size());
    EXPECT_EQ(2, receiverB.receivedAudio.size());

    abstractSplitter->receiveEncodedVideoConfig("foo", 3);
    EXPECT_EQ(1, receiverA.receivedVideoConfig.size());
    EXPECT_EQ(1, receiverB.receivedVideoConfig.size());

    abstractSplitter->receiveEncodedVideo(std::chrono::microseconds(1), std::chrono::microseconds(1), "bar", 3);
    EXPECT_EQ(1, receiverA.receivedVideo.size());
    EXPECT_EQ(1, receiverB.receivedVideo.size());

    abstractSplitter->receiveEncodedVideo(std::chrono::microseconds(2), std::chrono::microseconds(2), "baz", 3);
    EXPECT_EQ(2, receiverA.receivedVideo.size());
    EXPECT_EQ(2, receiverB.receivedVideo.size());
}
