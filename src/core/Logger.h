#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace SmartCity {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
public:
    static void Init();
    static void Log(LogLevel level, const std::string& message);

    template<typename... Args>
    static void Info(Args&&... args) {
        Log(LogLevel::Info, Format(std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Warn(Args&&... args) {
        Log(LogLevel::Warn, Format(std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Error(Args&&... args) {
        Log(LogLevel::Error, Format(std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Debug(Args&&... args) {
        Log(LogLevel::Debug, Format(std::forward<Args>(args)...));
    }

private:
    template<typename T>
    static void FormatHelper(std::ostringstream& oss, T&& val) {
        oss << val;
    }

    template<typename T, typename... Rest>
    static void FormatHelper(std::ostringstream& oss, T&& first, Rest&&... rest) {
        oss << first;
        FormatHelper(oss, std::forward<Rest>(rest)...);
    }

    template<typename... Args>
    static std::string Format(Args&&... args) {
        std::ostringstream oss;
        FormatHelper(oss, std::forward<Args>(args)...);
        return oss.str();
    }
};

} // namespace SmartCity
