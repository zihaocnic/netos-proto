#include "network/topology.h"

namespace netos {

bool build_topology(const Config& config, Topology* topology, std::string* error) {
  if (!topology) {
    if (error) {
      *error = "topology output is null";
    }
    return false;
  }

  topology->neighbors.clear();
  topology->neighbors.reserve(config.neighbors.size());
  for (const auto& neighbor : config.neighbors) {
    NeighborAddress addr;
    addr.host = neighbor.host;
    addr.port = neighbor.port;
    if (!resolve_address(neighbor.host, neighbor.port, &addr.addr, error)) {
      return false;
    }
    topology->neighbors.push_back(addr);
  }
  return true;
}

}
