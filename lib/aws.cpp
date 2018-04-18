#include "aws.hpp"

#include <cstdarg>
#include <mutex>
#include <vector>

#include <aws/core/Aws.h>
#include <aws/core/utils/logging/FormattedLogSystem.h>

#include <openssl/ssl.h>

#include "logger.hpp"

namespace {
    std::once_flag gInitAWSOnce;
}

class AWSLogger : public Aws::Utils::Logging::LogSystemInterface {
public:
    explicit AWSLogger(Aws::Utils::Logging::LogLevel level) : _level{level} {}
    virtual ~AWSLogger() {}

    virtual Aws::Utils::Logging::LogLevel GetLogLevel(void) const override {
        return _level;
    }

    virtual void Log(Aws::Utils::Logging::LogLevel level, const char* tag, const char* format, ...) override {
        va_list args;
        va_start(args, format);

        char buf[500];

        va_list tmp;
        va_copy(tmp, args);
        auto n = vsnprintf(buf, sizeof(buf), format, tmp);
        va_end(tmp);

        if (n < 0) {
            Logger{}.error("rtmp log error: {}", format);
        } else if (static_cast<size_t>(n) < sizeof(buf)) {
            _log(level, tag, buf);
        } else {
            auto buf = std::vector<char>(n+1);
            vsnprintf(&buf[0], buf.size(), format, args);
            _log(level, tag, buf.data());
        }

        va_end(args);
    }

    virtual void LogStream(Aws::Utils::Logging::LogLevel level, const char* tag, const Aws::OStringStream &messageStream) override {
        auto s = messageStream.rdbuf()->str();
        _log(level, tag, s.c_str());
    }

private:
    const Aws::Utils::Logging::LogLevel _level;
    const Logger _logger;

    void _log(Aws::Utils::Logging::LogLevel level, const char* tag, const char* str) {
        switch (level) {
        case Aws::Utils::Logging::LogLevel::Error:
        case Aws::Utils::Logging::LogLevel::Fatal:
            _logger.error("[aws][{}] {}", tag, str);
            break;
        case Aws::Utils::Logging::LogLevel::Warn:
            _logger.warn("[aws][{}] {}", tag, str);
            break;
        case Aws::Utils::Logging::LogLevel::Info:
        case Aws::Utils::Logging::LogLevel::Debug:
            _logger.info("[aws][{}] {}", tag, str);
            break;
        default:
            break;
        }
    }
};

void InitAWS() {
    std::call_once(gInitAWSOnce, [] {
        OPENSSL_init_ssl(0, nullptr);

        Aws::SDKOptions options;
        options.cryptoOptions.initAndCleanupOpenSSL = false;
        options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
        options.loggingOptions.logger_create_fn = [&] {
            return std::make_shared<AWSLogger>(options.loggingOptions.logLevel);
        };
        Aws::InitAPI(options);
    });
}
