#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "http.hpp"

class PlatformAPI {
public:
    // url should be the platform scheme and address, such as http://127.0.0.1:8080
    PlatformAPI(std::string url, std::string accessToken, HTTPClient* clientOverride = nullptr)
        : _url{url.back() == '/' ? url.substr(0, url.size()-1) : std::move(url)}, _accessToken{std::move(accessToken)}, _httpClient{clientOverride ? clientOverride : DefaultHTTPClient} {}

    struct Error {
        std::string message;
    };

    template <typename Data>
    struct Result {
        Data data;
        std::vector<Error> errors;
        std::string requestError;
    };

    struct AVStream {
        std::vector<std::string> codecs;
        std::chrono::milliseconds maximumSegmentDuration{0};
        int videoHeight = 0;
        int videoWidth = 0;
        int bitrate = 0;
        std::string gameId;
    };

    struct CreateAVStreamData {
        std::string id;
    };

    Result<CreateAVStreamData> createAVStream(const AVStream& stream);

    struct AVStreamSegmentReplica {
        std::string streamId;
        int64_t segmentNumber = 0;
        std::string url;
        std::chrono::milliseconds duration;
        std::chrono::system_clock::time_point startTime;
        std::string gameId;
        bool discontinuity = false;
    };

    struct CreateAVStreamSegmentReplicaData {
        std::string id;
    };

    Result<CreateAVStreamSegmentReplicaData> createAVStreamSegmentReplica(const AVStreamSegmentReplica& replica);

private:
    const std::string _url;
    const std::string _accessToken;
    HTTPClient* const _httpClient;

    template <typename T>
    Result<T> _doGraphQL(const nlohmann::json& body, std::function<void(nlohmann::json&, T*)> f);
};
