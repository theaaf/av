#include "logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ConsoleLogDestination : Logger::Destination {
    void log(Logger::Severity severity, const std::string& message, const std::vector<Logger::Field>& fields) override {
        auto now = std::chrono::system_clock::now();
        auto nowT = std::chrono::system_clock::to_time_t(now);
        struct tm tm{};
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
        std::cout << std::flush;
    }
} gConsoleLogDestination;

Logger::Destination* const Logger::Console{&gConsoleLogDestination};

struct JsonLogDestination : Logger::Destination {
    void log(Logger::Severity severity, const std::string& message, const std::vector<Logger::Field>& fields) override {
        auto now = std::chrono::system_clock::now();
        auto nowT = std::chrono::system_clock::to_time_t(now);
        struct tm tm{};
        gmtime_r(&nowT, &tm);

        std::stringstream timeString;
        timeString << std::put_time(&tm, "%FT%TZ");

        std::string severityString;
        switch(severity) {
        case Logger::Severity::Info:
            severityString = "info";
            break;
        case Logger::Severity::Warning:
            severityString = "warning";
            break;
        case Logger::Severity ::Error:
            severityString = "error";
            break;
        }

        json log = {
                {"level", severityString},
                {"msg", message},
                {"time", timeString.str()}
        };

        for (const auto& field : fields) {
            log[field.name] = field.formattedValue;
        }

        std::cout << log << std::endl << std::flush;
    }
} gJsonLogDestination;

Logger::Destination* const Logger::Json{&gJsonLogDestination};

struct VoidLogDestination : Logger::Destination {

    // NOLINTNEXTLINE(misc-unused-parameters)
    void log(Logger::Severity severity, const std::string& message, const std::vector<Logger::Field>& fields) override {}
} gVoidLogDestination;

Logger::Destination* const Logger::Void{&gVoidLogDestination};
