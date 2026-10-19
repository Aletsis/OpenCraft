#pragma once

#include <iostream>
#include <string>
#include <format>
#include <chrono>

namespace OpenCraft {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Log {
public:
    template <typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Debug, "\033[36m[DEBUG]\033[0m ", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Info, "\033[32m[INFO]\033[0m  ", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Warning, "\033[33m[WARN]\033[0m  ", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Error, "\033[31m[ERROR]\033[0m ", fmt, std::forward<Args>(args)...);
    }

private:
    template <typename... Args>
    static void log(LogLevel level, const char* prefix, std::format_string<Args...> fmt, Args&&... args) {
        (void)level;
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        std::cout << prefix << message << std::endl;
    }
};

} // namespace OpenCraft
