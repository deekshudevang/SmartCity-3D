#include "core/Logger.h"
#include <filesystem>

namespace SmartCity::core {

void Logger::Init(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_initialized) {
        std::filesystem::path path(logFilePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        s_logFile.open(logFilePath, std::ios::out | std::ios::app);
        s_initialized = true;
    }
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_logFile.is_open()) {
        s_logFile.close();
    }
    s_initialized = false;
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(s_mutex);

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

    std::ostringstream ss;
    ss << "[" << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S") << "." 
       << std::setfill('0') << std::setw(3) << ms.count() << "] "
       << "[" << levelStr << "] " << message;

    std::string formatted = ss.str();

    // Console output
    std::cout << formatted << std::endl;

    // File output
    if (s_logFile.is_open()) {
        s_logFile << formatted << std::endl;
        s_logFile.flush();
    }
}

} // namespace SmartCity::core
