#include "utils/logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace sentinel {

LogLevel Logger::current_level_ = LogLevel::INFO;

void Logger::setLevel(LogLevel level) {
    current_level_ = level;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARN:    return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < current_level_) {
        return;
    }
    
    std::string timestamp = getCurrentTime();
    std::string level_str = levelToString(level);
    
    // Color codes for different log levels
    const char* color = "";
    const char* reset = "\033[0m";
    
    switch (level) {
        case LogLevel::DEBUG:
            color = "\033[36m"; // Cyan
            break;
        case LogLevel::INFO:
            color = "\033[32m"; // Green
            break;
        case LogLevel::WARN:
            color = "\033[33m"; // Yellow
            break;
        case LogLevel::ERROR:
            color = "\033[31m"; // Red
            break;
    }
    
    std::cout << "[" << timestamp << "] "
              << color << level_str << reset << " - "
              << message << std::endl;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

} // namespace sentinel