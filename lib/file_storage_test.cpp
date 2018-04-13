#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/DefaultRetryStrategy.h>

#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>

#include "archiver.hpp"
#include "aws.hpp"
#include "logger_test.hpp"

TEST(FileStorageForURI, fileScheme) {
    TestLogDestination logDestination;
    EXPECT_TRUE(FileStorageForURI(&logDestination, "file:foo"));
    EXPECT_TRUE(FileStorageForURI(&logDestination, "file://foo"));
}

TEST(FileStorageForURI, s3Scheme) {
    TestLogDestination logDestination;
    EXPECT_TRUE(FileStorageForURI(&logDestination, "s3:foo"));
    EXPECT_TRUE(FileStorageForURI(&logDestination, "s3://foo"));
}

TEST(FileStorageForURI, unknownScheme) {
    TestLogDestination logDestination;
    EXPECT_FALSE(FileStorageForURI(&logDestination, "unknown"));
    EXPECT_FALSE(FileStorageForURI(&logDestination, "unknown:foo"));
    EXPECT_FALSE(FileStorageForURI(&logDestination, "unknown://foo"));
}

TEST(LocalFileStorage, storage) {
    std::string directory = ".LocalFileStorage-storage-test";
    system(("rm -rf " + directory).c_str());

    TestLogDestination logDestination;
    LocalFileStorage storage(&logDestination, directory);

    auto file = storage.createFile("foo");
    EXPECT_TRUE(file);
    if (file) {
        std::vector<uint8_t> kilobyte(1024, 1);
        for (int i = 0; i < 5; ++i) {
            EXPECT_TRUE(file->write(kilobyte.data(), kilobyte.size()));
        }
        EXPECT_TRUE(file->close());
    }

    auto f = std::fopen((directory + "/foo").c_str(), "rb");
    EXPECT_TRUE(f);
    if (f) {
        fclose(f);
    }

    system(("rm -rf " + directory).c_str());
}

// Creates an S3 client that connects to the Minio container started via `docker-compose up minio`.
std::shared_ptr<Aws::S3::S3Client> MinioS3Client(const char* bucket) {
    InitAWS();

    Aws::Auth::AWSCredentials credentials{"RE9CMYOVFI9Y26GM20M6", "tVeklgWcGHV+6Bc4nrkN1pbs6odQyzMrc40ESH1c"};
    Aws::Client::ClientConfiguration clientConfiguration;
    clientConfiguration.scheme = Aws::Http::Scheme::HTTP;
    clientConfiguration.endpointOverride = "127.0.0.1:9000";
    clientConfiguration.retryStrategy = std::make_shared<Aws::Client::DefaultRetryStrategy>(1, 1);
    clientConfiguration.verifySSL = false;

    auto client = std::make_shared<Aws::S3::S3Client>(credentials, clientConfiguration, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);

    {
        auto outcome = client->ListObjects(Aws::S3::Model::ListObjectsRequest{}.WithBucket(bucket));
        if (outcome.IsSuccess()) {
            for (auto& obj : outcome.GetResult().GetContents()) {
                client->DeleteObject(Aws::S3::Model::DeleteObjectRequest{}.WithBucket(bucket).WithKey(obj.GetKey()));
            }
        } else if (outcome.GetError().GetExceptionName() != "NoSuchBucket") {
            Logger{}.error("unable to list objects in bucket: {}: {}", outcome.GetError().GetExceptionName(), outcome.GetError().GetMessage());
            return nullptr;
        }
    }

    {
        auto outcome = client->DeleteBucket(Aws::S3::Model::DeleteBucketRequest{}.WithBucket(bucket));
        if (!outcome.IsSuccess() && outcome.GetError().GetExceptionName() != "NoSuchBucket") {
            Logger{}.error("unable to delete bucket: {}: {}", outcome.GetError().GetExceptionName(), outcome.GetError().GetMessage());
            return nullptr;
        }
    }

    {
        auto outcome = client->CreateBucket(Aws::S3::Model::CreateBucketRequest{}.WithBucket(bucket));
        if (!outcome.IsSuccess() && outcome.GetError().GetExceptionName() != "BucketAlreadyOwnedByYou") {
            Logger{}.error("unable to create bucket: {}: {}", outcome.GetError().GetExceptionName(), outcome.GetError().GetMessage());
            return nullptr;
        }
    }

    return client;
}

TEST(S3FileStorage, storage) {
    auto bucket = "file-storage-test";
    auto client = MinioS3Client(bucket);
    if (!client) {
        Logger{}.warn("Unable to connect to Minio. Run `docker-compose up minio` to start it and perform this test.");
        return;
    }

    TestLogDestination logDestination;
    S3FileStorage storage(&logDestination, bucket, client);

    auto file = storage.createFile("foo");
    ASSERT_NE(file, nullptr);
    std::vector<uint8_t> kilobyte(1024, 1);
    for (int i = 0; i < 30 * 1024; ++i) {
        ASSERT_TRUE(file->write(kilobyte.data(), kilobyte.size()));
    }
    ASSERT_TRUE(file->close());

    {
        auto outcome = client->GetObject(Aws::S3::Model::GetObjectRequest{}.WithBucket(bucket).WithKey("foo"));
        ASSERT_TRUE(outcome.IsSuccess());
    }
}

TEST(AsyncFile, file) {
    std::string directory = ".AsyncFile-storage-test";
    system(("rm -rf " + directory).c_str());

    TestLogDestination logDestination;
    LocalFileStorage storage(&logDestination, directory);

    {
        AsyncFile file{&storage, "foo"};
        file.write(std::make_shared<std::vector<uint8_t>>(1024, 1));
        EXPECT_FALSE(file.isComplete());
        file.close();
        while (!file.isComplete()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    auto f = std::fopen((directory + "/foo").c_str(), "rb");
    EXPECT_TRUE(f);
    if (f) {
        fclose(f);
    }

    system(("rm -rf " + directory).c_str());
}
