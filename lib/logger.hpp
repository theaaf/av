#pragma once

#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ostream.h>

class Logger {
public:
    template <typename... Args>
    void error(Args&&... args) const {
        return _log("\033[1;31mERROR\033[0m", std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(Args&&... args) const {
        return _log("\033[1;36mINFO\033[0m", std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(Args&&... args) const {
        return _log("\033[1;33mWARN\033[0m", std::forward<Args>(args)...);
    }

    Logger with() const { return *this; }

    template <typename T, typename... Rem>
    Logger with(const char* name, T&& value, Rem&&... rem) const {
        Logger ret = *this;
        ret._fields.emplace_back(Field{name, fmt::format("{}", std::forward<T>(value))});
        return ret.with(std::forward<Rem>(rem)...);
    }

private:
    struct Field {
        std::string name;
        std::string formattedValue;
    };
    std::vector<Field> _fields;

    template <typename... Args>
    void _log(const char* level, Args&&... args) const {
        auto message = fmt::format(std::forward<Args>(args)...);
        std::string fields;
        for (auto& f : _fields) {
            fields += " \033[1;1m" + f.name + "\033[0m=\033[1;1m" + f.formattedValue + "\033[0m";
        }
        fmt::print("[{}]{} {}\n", level, fields, message);
    }
};
