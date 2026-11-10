#include "core/Logger.h"

namespace SmartCity {

void Logger::Init() {
    // Initialized logging subsystem
}

void Logger::Log(LogLevel level, const std::string& message) {
    const char* levelStr = "INFO";
    switch (level) {
        case LogLevel::Debug: levelStr = "DEBUG"; break;
        case LogLevel::Info:  levelStr = "INFO "; break;
        case LogLevel::Warn:  levelStr = "WARN "; break;
        case LogLevel::Error: levelStr = "ERROR"; break;
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm timeInfo{};
#if defined(_WIN32)
    localtime_s(&timeInfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeInfo);
#endif

    std::cout << "[" << std::put_time(&timeInfo, "%H:%M:%S") << "." 
              << std::setfill('0') << std::setw(3) << ms.count() << "] "
              << "[" << levelStr << "] " << message << std::endl;
}

} // namespace SmartCity
