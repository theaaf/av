#include "logger.hpp"

#include <chrono>
#include <iomanip>

struct ConsoleLogDestination : Logger::Destination {
    virtual void log(Logger::Severity severity, const std::string& message, const std::vector<Logger::Field>& fields) override {
        auto now = std::chrono::system_clock::now();
        auto nowT = std::chrono::system_clock::to_time_t(now);
        struct tm tm;
        localtime_r(&nowT, &tm);

        std::string severityString;
        switch (severity) {
        case Logger::Severity::Info:
            severityString = "\033[1;36mINFO\033[0m";
            break;
        case Logger::Severity::Warning:
            severityString = "\033[1;33mWARN\033[0m";
            break;
        case Logger::Severity::Error:
            severityString = "\033[1;31mERROR\033[0m";
            break;
        }

        std::string fieldsString;
        for (auto& f : fields) {
            fieldsString += " \033[1;1m" + f.name + "\033[0m=\033[1;1m" + f.formattedValue + "\033[0m";
        }

        fmt::print("[{}][{}]{} {}\n", severityString, std::put_time(&tm, "%F %T"), fieldsString, message);
    }
} gConsoleLogDestination;

Logger::Destination* const Logger::Console{&gConsoleLogDestination};

struct VoidLogDestination : Logger::Destination {
    virtual void log(Logger::Severity severity, const std::string& message, const std::vector<Logger::Field>& fields) override {}
} gVoidLogDestination;

Logger::Destination* const Logger::Void{&gVoidLogDestination};
