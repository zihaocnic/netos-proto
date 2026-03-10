#pragma once

#include <string>

namespace netos {

enum class LogLevel {
  Debug = 0,
  Info = 1,
  Warn = 2,
  Error = 3
};

class Logger {
 public:
  static Logger& instance();

  void set_level(LogLevel level);
  void log(LogLevel level, const std::string& message);

 private:
  Logger();

  LogLevel level_;
};

void log_debug(const std::string& message);
void log_info(const std::string& message);
void log_warn(const std::string& message);
void log_error(const std::string& message);

LogLevel parse_log_level(const std::string& input);

}
