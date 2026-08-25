#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace apm {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) { min_level_ = level; }
    void set_file(const std::string& path);
    void set_color(bool enabled) { color_ = enabled; }

    void log(LogLevel level, const std::string& msg);
    void debug(const std::string& msg)   { log(LogLevel::Debug, msg); }
    void info(const std::string& msg)    { log(LogLevel::Info, msg); }
    void warning(const std::string& msg) { log(LogLevel::Warning, msg); }
    void error(const std::string& msg)   { log(LogLevel::Error, msg); }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string level_string(LogLevel level) const;
    std::string level_color(LogLevel level) const;
    std::string timestamp() const;

    LogLevel min_level_ = LogLevel::Info;
    bool color_ = true;
    std::mutex mutex_;
    std::ofstream file_;
};

// Convenience macros
#define APM_LOG_DEBUG(msg) apm::Logger::instance().debug(msg)
#define APM_LOG_INFO(msg)  apm::Logger::instance().info(msg)
#define APM_LOG_WARN(msg)  apm::Logger::instance().warning(msg)
#define APM_LOG_ERROR(msg) apm::Logger::instance().error(msg)

} // namespace apm
