#include "core/logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace netos {

namespace {
std::mutex g_log_mutex;

const char* level_label(LogLevel level) {
  switch (level) {
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
  }
  return "INFO";
}

std::string now_timestamp() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &now_time);
#else
  localtime_r(&now_time, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}
}  // namespace

Logger::Logger() : level_(LogLevel::Info) {}

Logger& Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::set_level(LogLevel level) { level_ = level; }

void Logger::log(LogLevel level, const std::string& message) {
  if (static_cast<int>(level) < static_cast<int>(level_)) {
    return;
  }
  std::lock_guard<std::mutex> lock(g_log_mutex);
  std::cout << now_timestamp() << " [" << level_label(level) << "] " << message << std::endl;
}

void log_debug(const std::string& message) { Logger::instance().log(LogLevel::Debug, message); }
void log_info(const std::string& message) { Logger::instance().log(LogLevel::Info, message); }
void log_warn(const std::string& message) { Logger::instance().log(LogLevel::Warn, message); }
void log_error(const std::string& message) { Logger::instance().log(LogLevel::Error, message); }

LogLevel parse_log_level(const std::string& input) {
  if (input == "debug") {
    return LogLevel::Debug;
  }
  if (input == "warn") {
    return LogLevel::Warn;
  }
  if (input == "error") {
    return LogLevel::Error;
  }
  return LogLevel::Info;
}

}
