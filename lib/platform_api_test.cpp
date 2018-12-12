#include <gtest/gtest.h>

#include "platform_api.hpp"

TEST(PlatformAPI, createAVStream) {
    struct TestHTTPClient : HTTPClient {
        virtual HTTPResult request(const HTTPRequest& request) override {
            EXPECT_EQ("https://example.com/v1/graphql", request.url);
            EXPECT_EQ("token access-token", request.headers.at("Authorization"));
            HTTPResult result;
            result.statusCode = 200;
            result.body = R"response(
                {
                    "data": {
                        "createAVStream": {
                            "id": "the-new-id"
                        }
                    }
                }
            )response";
            return result;
        }
    } httpClient;

    PlatformAPI api{"https://example.com", "access-token", &httpClient};

    PlatformAPI::AVStream stream;
    stream.bitrate = 6000000;

    auto result = api.createAVStream(stream);
    EXPECT_TRUE(result.requestError.empty()) << result.requestError;
    EXPECT_EQ("the-new-id", result.data.id);
}

TEST(PlatformAPI, patchAVStream) {
    struct TestHTTPClient : HTTPClient {
        virtual HTTPResult request(const HTTPRequest& request) override {
            EXPECT_EQ("https://example.com/v1/graphql", request.url);
            EXPECT_EQ("token access-token", request.headers.at("Authorization"));
            HTTPResult result;
            result.statusCode = 200;
            result.body = R"response(
                {
                    "data": {
                        "patchAVStream": {
                            "id": "the-id"
                        }
                    }
                }
            )response";
            return result;
        }
    } httpClient;

    PlatformAPI api{"https://example.com", "access-token", &httpClient};

    PlatformAPI::AVStreamPatch patch;
    patch.isLive = false;

    auto result = api.patchAVStreamById("the-id", patch);
    EXPECT_TRUE(result.requestError.empty()) << result.requestError;
}

TEST(PlatformAPI, createAVStreamSegmentReplica) {
    struct TestHTTPClient : HTTPClient {
        virtual HTTPResult request(const HTTPRequest& request) override {
            EXPECT_EQ("https://example.com/v1/graphql", request.url);
            EXPECT_EQ("token access-token", request.headers.at("Authorization"));
            HTTPResult result;
            result.statusCode = 200;
            result.body = R"response(
                {
                    "data": {
                        "createAVStreamSegmentReplica": {
                            "id": "the-new-id"
                        }
                    }
                }
            )response";
            return result;
        }
    } httpClient;

    PlatformAPI api{"https://example.com", "access-token", &httpClient};

    PlatformAPI::AVStreamSegmentReplica replica;
    replica.streamId = "stream-id";
    replica.segmentNumber = 1;
    replica.url = "http://foo/bar";
    replica.duration = std::chrono::milliseconds(123);

    auto result = api.createAVStreamSegmentReplica(replica);
    EXPECT_TRUE(result.requestError.empty()) << result.requestError;
    EXPECT_EQ("the-new-id", result.data.id);
}
