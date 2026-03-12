#pragma once

#include "core/config.h"
#include "network/address.h"

#include <string>
#include <vector>

namespace netos {

struct Topology {
  std::vector<NeighborAddress> neighbors;
};

bool build_topology(const Config& config, Topology* topology, std::string* error);

}
