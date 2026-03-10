#pragma once

#include <netinet/in.h>

#include <string>

namespace netos {

struct NeighborAddress {
  std::string host;
  int port = 0;
  sockaddr_in addr{};
};

bool resolve_address(const std::string& host, int port, sockaddr_in* out, std::string* error);
std::string addr_to_string(const sockaddr_in& addr);
bool same_address(const sockaddr_in& a, const sockaddr_in& b);

}
