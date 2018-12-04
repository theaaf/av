#pragma once

#include <string>
#include <vector>

#include <unistd.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

class Logger {
public:
    struct Field {
        std::string name;
        std::string formattedValue;
    };

    enum class Severity {
        Info,
        Warning,
        Error,
    };

    struct Destination {
        virtual void log(Severity severity, const std::string& message, const std::vector<Field>& fields) = 0;
    };

    Logger() {
        if (isatty(STDOUT_FILENO)) {
            _destination = Console;
        } else {
            _destination = Json;
        }
    }

    Logger(Destination* destination) : _destination{destination} {}

    template <typename... Args>
    void info(Args&&... args) const {
        return _log(Severity::Info, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(Args&&... args) const {
        return _log(Severity::Warning, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(Args&&... args) const {
        return _log(Severity::Error, std::forward<Args>(args)...);
    }

    Logger with() const { return *this; }

    template <typename T, typename... Rem>
    Logger with(const char* name, T&& value, Rem&&... rem) const {
        Logger ret = *this;
        ret._fields.emplace_back(Field{name, fmt::format("{}", std::forward<T>(value))});
        return ret.with(std::forward<Rem>(rem)...);
    }

    static Destination* const Console;
    static Destination* const Json;
    static Destination* const Void;

private:
    Destination* _destination = Console;

    std::vector<Field> _fields;

    template <typename... Args>
    void _log(Severity severity, Args&&... args) const {
        auto message = fmt::format(std::forward<Args>(args)...);
        _destination->log(severity, message, _fields);
    }
};
