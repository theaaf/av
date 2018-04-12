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
