#include "network/address.h"

#include <arpa/inet.h>
#include <netdb.h>

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

}
