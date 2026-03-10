#include "network/udp_transport.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace netos {

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
