#include "platform_api.hpp"

using json = nlohmann::json;

template <typename T>
PlatformAPI::Result<T> PlatformAPI::_doGraphQL(const json& body, std::function<void(json&, T*)> f) {
    HTTPRequest httpRequest;
    httpRequest.url = _url + "/v1/graphql";
    httpRequest.body = body.dump();
    httpRequest.headers["Authorization"] = "token " + _accessToken;
    httpRequest.headers["Content-Type"] = "application/json";

    auto httpResult = _httpClient->request(httpRequest);

    Result<T> result;
    if (httpResult.statusCode != 200) {
        result.requestError = "status code " + std::to_string(httpResult.statusCode);
        return result;
    }

    try {
        auto resp = json::parse(httpResult.body);
        for (auto& errJSON : resp["errors"]) {
            Error err;
            err.message = errJSON["message"].get<std::string>();
            result.errors.emplace_back(err);
        }
        auto data = resp["data"];
        if (result.errors.empty() && !data.is_null()) {
            f(data, &result.data);
        }
    } catch (const std::exception& e) {
        result.requestError = std::string("invalid json: ") + e.what();
        return result;
    } catch (...) {
        result.requestError = "invalid json";
        return result;
    }

    return result;
}

PlatformAPI::Result<PlatformAPI::CreateAVStreamData> PlatformAPI::createAVStream(const PlatformAPI::AVStream& stream) {
    auto query = R"query(
      mutation CreateAVStream($gameId: ID!, $codecs: [String]!, $bitrate: Int!, $videoWidth: Int!, $videoHeight: Int!, $maximumSegmentDurationMilliseconds: Int!, $isLive: Boolean!) {
        createAVStream(
          stream: {
            gameId: $gameId,
            codecs: $codecs,
            bitrate: $bitrate,
            videoWidth: $videoWidth,
            videoHeight: $videoHeight,
            maximumSegmentDurationMilliseconds: $maximumSegmentDurationMilliseconds,
            isLive: $isLive,
          },
        ) {
          id
        }
      }
    )query";

    json body = {
        {"operationName", "CreateAVStream"},
        {"query", query},
        {"variables", {
            {"codecs", stream.codecs},
            {"bitrate", stream.bitrate},
            {"videoWidth", stream.videoWidth},
            {"videoHeight", stream.videoHeight},
            {"gameId", stream.gameId},
            {"maximumSegmentDurationMilliseconds", std::chrono::milliseconds(stream.maximumSegmentDuration).count()},
            {"isLive", stream.isLive},
        }},
    };

    return _doGraphQL<CreateAVStreamData>(body, [](json& in, CreateAVStreamData* out) {
        out->id = in["createAVStream"]["id"].get<std::string>();
    });
}

PlatformAPI::Result<PlatformAPI::PatchAVStreamData> PlatformAPI::patchAVStreamById(const std::string& id, const PlatformAPI::AVStreamPatch& patch) {
    auto query = R"query(
      mutation PatchAVStream($id: ID!, $patch: AVStreamPatchInput!) {
        patchAVStream(
          id: $id,
          patch: $patch,
        ) {
          id
        }
      }
    )query";

    json::object_t patchObj;
    if (patch.isLive) {
        patchObj["isLive"] = *patch.isLive;
    }

    json body = {
        {"operationName", "PatchAVStream"},
        {"query", query},
        {"variables", {
            {"id", id},
            {"patch", patchObj},
        }},
    };

    return _doGraphQL<PatchAVStreamData>(body, [](json& in, PatchAVStreamData* out) {});
}

PlatformAPI::Result<PlatformAPI::CreateAVStreamSegmentReplicaData> PlatformAPI::createAVStreamSegmentReplica(const PlatformAPI::AVStreamSegmentReplica& replica) {
    auto query = R"query(
      mutation CreateAVStreamSegmentReplica($streamId: ID!, $segmentNumber: Int!, $url: String!, $durationMilliseconds: Int!, $discontinuity: Boolean, $time: DateTime!) {
        createAVStreamSegmentReplica(
          replica: {
            streamId: $streamId,
            segmentNumber: $segmentNumber,
            url: $url,
            durationMilliseconds: $durationMilliseconds,
            discontinuity: $discontinuity,
            time: $time,
          },
        ) {
          id
        }
      }
    )query";

    auto timeT = std::chrono::system_clock::to_time_t(replica.time);
    struct tm tm{};
    gmtime_r(&timeT, &tm);

    std::stringstream timeString;
    timeString << std::put_time(&tm, "%FT%TZ");

    json body = {
        {"operationName", "CreateAVStreamSegmentReplica"},
        {"query", query},
        {"variables", {
            {"streamId", replica.streamId},
            {"segmentNumber", replica.segmentNumber},
            {"url", replica.url},
            {"durationMilliseconds", std::chrono::milliseconds(replica.duration).count()},
            {"discontinuity", replica.discontinuity},
            {"time", timeString.str()},
        }},
    };

    return _doGraphQL<CreateAVStreamSegmentReplicaData>(body, [](json& in, CreateAVStreamSegmentReplicaData* out) {
        out->id = in["createAVStreamSegmentReplica"]["id"].get<std::string>();
    });
}

PlatformAPI::Result<PlatformAPI::NodeTypenameData> PlatformAPI::nodeTypename(const std::string &id) {
    auto query = R"query(
        query TestExistence($nodeId: ID!) {
            node (id: $nodeId) {
                __typename
            }
        }
    )query";

    json body = {
        {"operationName", "TestExistence"},
        {"query", query},
        {"variables", {
              {"nodeId", id}
        }}
    };

    return _doGraphQL<NodeTypenameData>(body, [](json& resp, NodeTypenameData* out) {
        if (!resp["node"].is_null()) {
            out->exists = true;
            out->type = resp["node"]["__typename"].get<std::string>();
        }
    });
};
