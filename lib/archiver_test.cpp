#include <gtest/gtest.h>

#include "archiver.hpp"
#include "init.hpp"

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/DefaultRetryStrategy.h>

#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>

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

TEST(Archiver, archiving) {
    auto bucket = "archiving-test";
    auto client = MinioS3Client(bucket);
    if (!client) {
        FAIL() << "Unable to connect to Minio. Run `docker-compose up minio` to start it and complete this test.";
    }

    {
        Archiver archiver{Logger::Void, bucket, "foo/{}"};
        archiver.overrideS3Client(client);
        std::vector<uint8_t> kilobyte(1024, 1);
        for (int i = 0; i < 30 * 1024; ++i) {
            archiver.receiveEncodedAudioConfig(kilobyte.data(), kilobyte.size());
        }
    }


    {
        auto outcome = client->GetObject(Aws::S3::Model::GetObjectRequest{}.WithBucket(bucket).WithKey("foo/0"));
        ASSERT_TRUE(outcome.IsSuccess());
    }
}
