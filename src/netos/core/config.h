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
  int content_bf_bits = 512;
  int content_bf_hashes = 3;
  int content_bf_exchange_ms = 1000;
  int content_bf_ttl_ms = 3000;
  int content_bf_fallback_ms = 300;
  int query_bf_bits = 1024;
  int query_bf_hashes = 4;
  int query_bf_aggregation_ms = 200;
  int query_bf_ttl_ms = 2000;
  double query_bf_max_fill_ratio = 0.9;
  int query_bf_max_keys = 0;
  int broadcast_attempt_limit = 3;
  int broadcast_window_ms = 1000;
  int topology_reload_ms = 0;
  bool async_forward_enable = true;
  int forward_workers = 1;
  int forward_queue_max = 1024;
  std::string forward_drop_policy = "drop_newest";
};

Config load_config();
Config load_config(std::string* error, std::string* source);

}
