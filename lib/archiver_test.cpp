#include <gtest/gtest.h>

#include "archiver.hpp"
#include "file_storage_test.hpp"
#include "logger_test.hpp"

TEST(Archiver, archiving) {
    TestFileStorage storage;
    TestLogDestination logDestination;

    {
        Archiver archiver{&logDestination, &storage, "foo/{}"};
        std::vector<uint8_t> kilobyte(1024, 1);
        for (int i = 0; i < 30 * 1024; ++i) {
            archiver.receiveEncodedAudioConfig(kilobyte.data(), kilobyte.size());
        }
    }

    EXPECT_EQ(1, storage.files.size());
    EXPECT_NE(nullptr, storage.files["foo/0"]);
    EXPECT_TRUE(storage.files["foo/0"]->isClosed);
    EXPECT_GT(storage.files["foo/0"]->contents.size(), 30 * 1024);
}
