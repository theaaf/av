#include "utility.hpp"

#include "aws.hpp"

#include <aws/core/utils/UUID.h>

std::string GenerateUUID() {
    InitAWS();
    std::string uuid = Aws::String(Aws::Utils::UUID::RandomUUID()).c_str();
    std::transform(uuid.begin(), uuid.end(), uuid.begin(), ::tolower);
    return uuid;
}
