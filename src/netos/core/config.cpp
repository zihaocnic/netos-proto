#include "core/config.h"

#include "core/string_utils.h"

#include <cstdlib>

namespace netos {

namespace {
std::string get_env(const char* key, const std::string& fallback) {
  const char* value = std::getenv(key);
  if (!value || std::string(value).empty()) {
    return fallback;
  }
  return value;
}

int get_env_int(const char* key, int fallback) {
  const char* value = std::getenv(key);
  if (!value || std::string(value).empty()) {
    return fallback;
  }
  char* end = nullptr;
  long parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return fallback;
  }
  return static_cast<int>(parsed);
}
}

Config load_config() {
  Config cfg;
  cfg.node_id = get_env("NETOS_NODE_ID", "node");
  cfg.bind_ip = get_env("NETOS_BIND_IP", "0.0.0.0");
  cfg.bind_port = get_env_int("NETOS_BIND_PORT", 9000);
  cfg.redis_socket = get_env("NETOS_REDIS_SOCKET", "/var/run/redis/redis.sock");
  cfg.request_delay_ms = get_env_int("NETOS_REQUEST_DELAY_MS", 800);
  cfg.request_ttl = get_env_int("NETOS_REQUEST_TTL", 3);
  cfg.query_ttl_ms = get_env_int("NETOS_QUERY_TTL_MS", 1500);
  cfg.sync_table_capacity = get_env_int("NETOS_SYNC_TABLE_CAPACITY", 128);

  auto neighbors_raw = trim(get_env("NETOS_NEIGHBORS", ""));
  if (!neighbors_raw.empty()) {
    auto entries = split(neighbors_raw, ',', true);
    for (const auto& entry : entries) {
      auto pair = split(entry, ':', true);
      if (pair.size() != 2) {
        continue;
      }
      NeighborConfig neighbor;
      neighbor.host = trim(pair[0]);
      neighbor.port = std::strtol(pair[1].c_str(), nullptr, 10);
      if (!neighbor.host.empty() && neighbor.port > 0) {
        cfg.neighbors.push_back(neighbor);
      }
    }
  }

  auto seed_raw = trim(get_env("NETOS_SEED_KEYS", ""));
  if (!seed_raw.empty()) {
    auto entries = split(seed_raw, ';', true);
    for (const auto& entry : entries) {
      auto pos = entry.find('=');
      if (pos == std::string::npos) {
        continue;
      }
      KeyValue kv;
      kv.key = trim(entry.substr(0, pos));
      kv.value = entry.substr(pos + 1);
      if (!kv.key.empty()) {
        cfg.seed_keys.push_back(kv);
      }
    }
  }

  auto request_raw = trim(get_env("NETOS_REQUEST_KEYS", ""));
  if (!request_raw.empty()) {
    auto entries = split(request_raw, ',', true);
    for (const auto& entry : entries) {
      auto key = trim(entry);
      if (!key.empty()) {
        cfg.request_keys.push_back(key);
      }
    }
  }

  return cfg;
}

}
