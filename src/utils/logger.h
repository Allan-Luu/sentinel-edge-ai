#ifndef SENTINEL_LOGGER_H
#define SENTINEL_LOGGER_H

#include <string>
#include <chrono>

namespace sentinel {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static void setLevel(LogLevel level);
    
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    
private:
    static void log(LogLevel level, const std::string& message);
    static std::string getCurrentTime();
    static std::string levelToString(LogLevel level);
    
    static LogLevel current_level_;
};

} // namespace sentinel

#endif // SENTINEL_LOGGER_H