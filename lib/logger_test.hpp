#pragma once

#include <mutex>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "logger.hpp"

// TestLogDestination is a silent log destination that creates Google Test failures when warnings or errors are logged.
class TestLogDestination : public Logger::Destination {
public:
    virtual ~TestLogDestination() {
        _expectNoWarnings();
    }

    virtual void log(Logger::Severity severity, const std::string& message, const std::vector<Logger::Field>& fields) override {
        std::lock_guard<std::mutex> l{_mutex};
        _entries.emplace_back(severity, message, fields);
    }

private:
    std::mutex _mutex;
    std::vector<std::tuple<Logger::Severity, std::string, std::vector<Logger::Field>>> _entries;

    void _expectNoWarnings() {
        for (auto& e : _entries) {
            if (std::get<Logger::Severity>(e) >= Logger::Severity::Warning) {
                auto output = "logged issue: " + std::get<std::string>(e);
                for (auto& field : std::get<std::vector<Logger::Field>>(e)) {
                    output += ", " + field.name + "=" + field.formattedValue;
                }
                ADD_FAILURE() << output;
            }
        }
    }
};
