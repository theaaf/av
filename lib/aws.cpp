#include "aws.hpp"

#include <mutex>

#include <aws/core/Aws.h>
#include <openssl/ssl.h>

namespace {
    std::once_flag gInitAWSOnce;
}

void InitAWS() {
    std::call_once(gInitAWSOnce, [] {
        OPENSSL_init_ssl(0, nullptr);

        Aws::SDKOptions options;
        options.cryptoOptions.initAndCleanupOpenSSL = false;
        Aws::InitAPI(options);
    });
}
