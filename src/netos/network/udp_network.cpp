#include "network/udp_network.h"

#include "core/logger.h"

#include <utility>

namespace netos {

UdpNetwork::UdpNetwork(std::string bind_ip, int bind_port)
    : transport_(std::move(bind_ip), bind_port) {}

bool UdpNetwork::start(ReceiveHandler handler, std::string* error) {
  return transport_.start(std::move(handler), error);
}

void UdpNetwork::stop() { transport_.stop(); }

void UdpNetwork::set_topology(Topology topology) {
  std::lock_guard<std::mutex> lock(topology_mutex_);
  topology_ = std::move(topology);
}

bool UdpNetwork::send_direct(const sockaddr_in& addr, const Message& msg, std::string* error) {
  return transport_.send_to(addr, msg.to_wire(), error);
}

void UdpNetwork::send_broadcast(const Message& msg, const sockaddr_in* exclude) {
  std::lock_guard<std::mutex> lock(topology_mutex_);
  for (auto& neighbor : topology_.neighbors) {
    if (exclude && same_address(neighbor.addr, *exclude)) {
      continue;
    }
    send_to_neighbor(neighbor, msg);
  }
}

bool UdpNetwork::send_to_neighbor(NeighborAddress& neighbor, const Message& msg) {
  if (neighbor.addr.sin_family == 0) {
    std::string resolve_error;
    if (!resolve_address(neighbor.host, neighbor.port, &neighbor.addr, &resolve_error)) {
      log_warn("send failed to " + neighbor.host + ":" + std::to_string(neighbor.port) +
               " - " + resolve_error);
      return false;
    }
  }
  std::string error;
  if (!transport_.send_to(neighbor.addr, msg.to_wire(), &error)) {
    log_warn("send failed to " + neighbor.host + ":" + std::to_string(neighbor.port) +
             " - " + error);
    return false;
  }
  return true;
}

}
