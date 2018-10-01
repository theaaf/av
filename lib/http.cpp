#include "lib/http.hpp"

#include <sstream>

#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/stream/ResponseStream.h>

#include "aws.hpp"

struct AWSHTTPClient : HTTPClient {
    virtual ~AWSHTTPClient() {}
    virtual HTTPResult request(const HTTPRequest& request) override {
        InitAWS();

        HTTPResult result;

        Aws::Http::HttpMethod method;
        if (request.method == "GET" || (request.method == "" && request.body == "")) {
            method = Aws::Http::HttpMethod::HTTP_GET;
        } else if (request.method == "POST" || (request.method == "" && request.body != "")) {
            method = Aws::Http::HttpMethod::HTTP_POST;
        } else {
            result.error = "unsupported method";
            return result;
        }

        Aws::Client::ClientConfiguration awsClientConfig;
        auto awsClient = Aws::Http::CreateHttpClient(awsClientConfig);

        auto awsReq = Aws::Http::CreateHttpRequest(Aws::String{request.url.c_str()}, method, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
        if (request.body != "") {
            awsReq->AddContentBody(std::make_shared<std::stringstream>(request.body));
            awsReq->SetContentLength(std::to_string(request.body.size()).c_str());
        }
        for (auto& kv : request.headers) {
            awsReq->SetHeaderValue(kv.first.c_str(), kv.second.c_str());
        }

        auto awsResp = awsClient->MakeRequest(awsReq);
        if (awsResp == nullptr || awsResp->GetResponseCode() == Aws::Http::HttpResponseCode::REQUEST_NOT_MADE) {
            result.error = "unable to make request";
        } else {
            result.statusCode = static_cast<int>(awsResp->GetResponseCode());
            std::stringstream ss;
            ss << awsResp->GetResponseBody().rdbuf();
            result.body = ss.str();
        }
        return result;
    }
} gDefaultHTTPClient;

HTTPClient* const DefaultHTTPClient{&gDefaultHTTPClient};

