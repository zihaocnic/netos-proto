#include "cache/redis_client.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>

namespace netos {

RedisClient::RedisClient(std::string socket_path) : socket_path_(std::move(socket_path)), fd_(-1) {}

RedisClient::~RedisClient() {
  if (fd_ >= 0) {
    ::close(fd_);
  }
}

bool RedisClient::connect(std::string* error) {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }

  fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd_ < 0) {
    if (error) {
      *error = std::string("socket() failed: ") + std::strerror(errno);
    }
    return false;
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  if (socket_path_.size() >= sizeof(addr.sun_path)) {
    if (error) {
      *error = "socket path too long";
    }
    return false;
  }
  std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    if (error) {
      *error = std::string("connect() failed: ") + std::strerror(errno);
    }
    return false;
  }

  return true;
}

bool RedisClient::set(const std::string& key, const std::string& value, std::string* error) {
  if (!send_command({"SET", key, value}, error)) {
    return false;
  }
  auto resp = read_resp(error);
  if (!resp.has_value()) {
    return false;
  }
  if (resp->type == '-') {
    if (error) {
      *error = "redis error: " + resp->payload;
    }
    return false;
  }
  return true;
}

std::optional<std::string> RedisClient::get(const std::string& key, std::string* error) {
  if (!send_command({"GET", key}, error)) {
    return std::nullopt;
  }
  auto resp = read_resp(error);
  if (!resp.has_value()) {
    return std::nullopt;
  }
  if (resp->type == '$') {
    if (resp->is_null) {
      return std::nullopt;
    }
    return resp->payload;
  }
  if (resp->type == '-') {
    if (error) {
      *error = "redis error: " + resp->payload;
    }
    return std::nullopt;
  }
  return std::nullopt;
}

bool RedisClient::send_command(const std::initializer_list<std::string>& args, std::string* error) {
  if (fd_ < 0) {
    if (error) {
      *error = "redis socket not connected";
    }
    return false;
  }
  std::ostringstream oss;
  oss << "*" << args.size() << "\r\n";
  for (const auto& arg : args) {
    oss << "$" << arg.size() << "\r\n" << arg << "\r\n";
  }
  auto payload = oss.str();
  size_t total_sent = 0;
  while (total_sent < payload.size()) {
    ssize_t sent = ::send(fd_, payload.data() + total_sent, payload.size() - total_sent, 0);
    if (sent <= 0) {
      if (error) {
        *error = std::string("send() failed: ") + std::strerror(errno);
      }
      return false;
    }
    total_sent += static_cast<size_t>(sent);
  }
  return true;
}

std::optional<RedisClient::RespValue> RedisClient::read_resp(std::string* error) {
  std::string line;
  if (!read_line(&line, error)) {
    return std::nullopt;
  }
  if (line.empty()) {
    if (error) {
      *error = "empty response";
    }
    return std::nullopt;
  }

  RespValue resp;
  resp.type = line[0];
  resp.payload = line.substr(1);

  if (resp.type == '$') {
    int len = 0;
    try {
      len = std::stoi(resp.payload);
    } catch (...) {
      if (error) {
        *error = "invalid bulk length";
      }
      return std::nullopt;
    }
    if (len < 0) {
      resp.is_null = true;
      resp.payload.clear();
      return resp;
    }
    std::string bulk;
    if (!read_exact(static_cast<size_t>(len) + 2, &bulk, error)) {
      return std::nullopt;
    }
    if (bulk.size() >= 2) {
      resp.payload = bulk.substr(0, bulk.size() - 2);
    } else {
      resp.payload.clear();
    }
  }

  return resp;
}

bool RedisClient::read_line(std::string* out, std::string* error) {
  out->clear();
  char ch = 0;
  while (true) {
    ssize_t n = ::recv(fd_, &ch, 1, 0);
    if (n <= 0) {
      if (error) {
        *error = std::string("recv() failed: ") + std::strerror(errno);
      }
      return false;
    }
    if (ch == '\r') {
      char next = 0;
      ssize_t n2 = ::recv(fd_, &next, 1, 0);
      if (n2 <= 0 || next != '\n') {
        if (error) {
          *error = "invalid line ending";
        }
        return false;
      }
      break;
    }
    out->push_back(ch);
  }
  return true;
}

bool RedisClient::read_exact(size_t len, std::string* out, std::string* error) {
  out->clear();
  out->reserve(len);
  while (out->size() < len) {
    char buffer[256];
    size_t remaining = len - out->size();
    size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
    ssize_t n = ::recv(fd_, buffer, chunk, 0);
    if (n <= 0) {
      if (error) {
        *error = std::string("recv() failed: ") + std::strerror(errno);
      }
      return false;
    }
    out->append(buffer, buffer + n);
  }
  return true;
}

}
