#pragma once

#include <string>
#include <unordered_map>

struct HTTPRequest {
    std::string url;
    std::string method;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HTTPResult {
    int statusCode = -1;
    std::string error;
    std::string body;
};

struct HTTPClient {
    virtual HTTPResult request(const HTTPRequest& request) = 0;
};

extern HTTPClient* const DefaultHTTPClient;
