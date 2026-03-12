#pragma once

#include "network/network_api.h"
#include "network/topology.h"
#include "network/udp_transport.h"

namespace netos {

class UdpNetwork : public NetworkApi {
 public:
  UdpNetwork(std::string bind_ip, int bind_port);

  bool start(ReceiveHandler handler, std::string* error) override;
  void stop() override;

  void set_topology(Topology topology);

  bool send_direct(const sockaddr_in& addr, const Message& msg, std::string* error) override;
  void send_broadcast(const Message& msg, const sockaddr_in* exclude) override;

 private:
  bool send_to_neighbor(const NeighborAddress& neighbor, const Message& msg);

  UdpTransport transport_;
  Topology topology_;
};

}
