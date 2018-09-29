#include <gtest/gtest.h>

#include "http.hpp"

TEST(DefaultHTTPClient, request) {
    HTTPRequest req;
    req.url = "https://httpbin.org/post";
    req.method = "POST";
    req.headers = {
        {"Foo", "IAmUnlikelyToOccurRandomlyHeader"},
    };
    req.body = "baz";

    auto resp = DefaultHTTPClient->request(req);
    ASSERT_EQ(200, resp.statusCode);
    EXPECT_NE(std::string::npos, resp.body.find("IAmUnlikelyToOccurRandomlyHeader")) << resp.body;
    EXPECT_NE(std::string::npos, resp.body.find("\"baz\"")) << resp.body;
}

TEST(DefaultHTTPClient, badDomainRequest) {
    HTTPRequest req;
    req.url = "https://this-domain-shouldnt-exist.aaf.com/get";

    auto resp = DefaultHTTPClient->request(req);
    EXPECT_EQ(-1, resp.statusCode);
    EXPECT_FALSE(resp.error.empty());
}
