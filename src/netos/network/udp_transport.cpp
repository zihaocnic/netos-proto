#include "network/udp_transport.h"

#include "core/logger.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace netos {

bool resolve_address(const std::string& host, int port, sockaddr_in* out, std::string* error) {
  if (!out) {
    if (error) {
      *error = "output address is null";
    }
    return false;
  }
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  addrinfo* result = nullptr;
  auto port_str = std::to_string(port);
  int rc = ::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
  if (rc != 0 || !result) {
    if (error) {
      *error = std::string("getaddrinfo failed: ") + gai_strerror(rc);
    }
    return false;
  }
  auto* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
  *out = *addr;
  ::freeaddrinfo(result);
  return true;
}

std::string addr_to_string(const sockaddr_in& addr) {
  char buf[INET_ADDRSTRLEN];
  const char* res = ::inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
  if (!res) {
    return "unknown";
  }
  int port = ntohs(addr.sin_port);
  return std::string(buf) + ":" + std::to_string(port);
}

bool same_address(const sockaddr_in& a, const sockaddr_in& b) {
  return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

UdpTransport::UdpTransport(std::string bind_ip, int bind_port)
    : bind_ip_(std::move(bind_ip)),
      bind_port_(bind_port),
      fd_(-1),
      running_(false) {}

UdpTransport::~UdpTransport() { stop(); }

bool UdpTransport::start(ReceiveHandler handler, std::string* error) {
  handler_ = std::move(handler);
  fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd_ < 0) {
    if (error) {
      *error = std::string("socket() failed: ") + std::strerror(errno);
    }
    return false;
  }
  int reuse = 1;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(bind_port_));
  if (bind_ip_ == "0.0.0.0") {
    addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    if (::inet_pton(AF_INET, bind_ip_.c_str(), &addr.sin_addr) != 1) {
      if (error) {
        *error = "invalid bind ip";
      }
      return false;
    }
  }

  if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    if (error) {
      *error = std::string("bind() failed: ") + std::strerror(errno);
    }
    return false;
  }

  running_ = true;
  thread_ = std::thread(&UdpTransport::run, this);
  return true;
}

void UdpTransport::stop() {
  if (!running_) {
    return;
  }
  running_ = false;
  if (fd_ >= 0) {
    ::shutdown(fd_, SHUT_RDWR);
    ::close(fd_);
    fd_ = -1;
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

bool UdpTransport::send_to(const sockaddr_in& addr, const std::string& payload, std::string* error) {
  if (fd_ < 0) {
    if (error) {
      *error = "socket not started";
    }
    return false;
  }
  ssize_t sent = ::sendto(fd_, payload.data(), payload.size(), 0,
                          reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
  if (sent < 0) {
    if (error) {
      *error = std::string("sendto failed: ") + std::strerror(errno);
    }
    return false;
  }
  return true;
}

void UdpTransport::run() {
  while (running_) {
    char buffer[2048];
    sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    ssize_t n = ::recvfrom(fd_, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
    if (n <= 0) {
      if (!running_) {
        break;
      }
      continue;
    }
    buffer[n] = '\0';
    if (handler_) {
      handler_(std::string(buffer, static_cast<size_t>(n)), from);
    }
  }
}

}
