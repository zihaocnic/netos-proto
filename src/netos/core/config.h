#pragma once

#include <string>
#include <vector>

namespace netos {

struct KeyValue {
  std::string key;
  std::string value;
};

struct NeighborConfig {
  std::string host;
  int port = 0;
};

struct Config {
  std::string node_id;
  std::string config_source;
  std::string log_level;
  std::string bind_ip;
  int bind_port = 0;
  std::string redis_socket;
  std::vector<NeighborConfig> neighbors;
  std::vector<KeyValue> seed_keys;
  std::vector<std::string> request_keys;
  int request_delay_ms = 500;
  int request_ttl = 3;
  int query_ttl_ms = 1500;
  int sync_table_capacity = 128;
};

Config load_config();

}
