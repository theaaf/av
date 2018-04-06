#include "init.hpp"

#include <mutex>

#include <aws/core/Aws.h>

namespace {
    std::once_flag gInitAWSOnce;
}

void InitAWS() {
    std::call_once(gInitAWSOnce, [] {
        Aws::SDKOptions options;
        Aws::InitAPI(options);
    });
}
