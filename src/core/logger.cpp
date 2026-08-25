#include "apmanager/core/logger.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace apm {

void Logger::set_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    file_.open(path, std::ios::app);
}

std::string Logger::timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_buf{};
    localtime_r(&time_t_now, &tm_buf);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::level_string(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
    }
    return "?????";
}

std::string Logger::level_color(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "\033[36m";   // cyan
        case LogLevel::Info:    return "\033[32m";   // green
        case LogLevel::Warning: return "\033[33m";   // yellow
        case LogLevel::Error:   return "\033[31m";   // red
    }
    return "";
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < min_level_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    std::string ts = timestamp();
    std::string lvl = level_string(level);

    // Console output
    if (color_) {
        std::string col = level_color(level);
        std::string reset = "\033[0m";
        std::cerr << "\033[90m" << ts << reset
                  << " " << col << "[" << lvl << "]" << reset
                  << " " << msg << "\n";
    } else {
        std::cerr << ts << " [" << lvl << "] " << msg << "\n";
    }

    // File output
    if (file_.is_open()) {
        file_ << ts << " [" << lvl << "] " << msg << "\n";
        file_.flush();
    }
}

} // namespace apm
