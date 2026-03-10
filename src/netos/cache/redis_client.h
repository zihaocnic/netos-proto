#pragma once

#include <optional>
#include <string>

namespace netos {

class RedisClient {
 public:
  explicit RedisClient(std::string socket_path);
  ~RedisClient();

  bool connect(std::string* error);
  bool set(const std::string& key, const std::string& value, std::string* error);
  std::optional<std::string> get(const std::string& key, std::string* error);

 private:
  struct RespValue {
    char type = 0;
    std::string payload;
    bool is_null = false;
  };

  bool send_command(const std::initializer_list<std::string>& args, std::string* error);
  std::optional<RespValue> read_resp(std::string* error);

  bool read_line(std::string* out, std::string* error);
  bool read_exact(size_t len, std::string* out, std::string* error);

  std::string socket_path_;
  int fd_;
};

}
