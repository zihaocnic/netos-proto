#include "core/config.h"

#include "core/logger.h"
#include "core/string_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

namespace netos {

namespace {
using EnvMap = std::unordered_map<std::string, std::string>;

std::string get_env_only(const char* key) {
  const char* value = std::getenv(key);
  if (!value) {
    return "";
  }
  return std::string(value);
}

std::string strip_quotes(const std::string& value) {
  if (value.size() < 2) {
    return value;
  }
  char start = value.front();
  char end = value.back();
  if ((start == '"' && end == '"') || (start == '\'' && end == '\'')) {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

bool load_env_file(const std::string& path, EnvMap* out, std::string* error) {
  std::ifstream in(path);
  if (!in.is_open()) {
    if (error) {
      *error = "unable to open config file " + path;
    }
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line.rfind("export ", 0) == 0) {
      line = trim(line.substr(7));
    }
    auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }
    std::string key = trim(line.substr(0, pos));
    std::string value = trim(line.substr(pos + 1));
    if (key.empty()) {
      continue;
    }
    out->insert_or_assign(key, strip_quotes(value));
  }
  return true;
}

std::string get_setting(const EnvMap& env_file, const char* key, const std::string& fallback) {
  const char* env = std::getenv(key);
  if (env && !std::string(env).empty()) {
    return env;
  }
  auto it = env_file.find(key);
  if (it != env_file.end() && !it->second.empty()) {
    return it->second;
  }
  return fallback;
}

int get_setting_int(const EnvMap& env_file, const char* key, int fallback) {
  auto value = get_setting(env_file, key, "");
  if (value.empty()) {
    return fallback;
  }
  char* end = nullptr;
  long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str() || *end != '\0') {
    return fallback;
  }
  return static_cast<int>(parsed);
}

double get_setting_double(const EnvMap& env_file, const char* key, double fallback) {
  auto value = get_setting(env_file, key, "");
  if (value.empty()) {
    return fallback;
  }
  char* end = nullptr;
  double parsed = std::strtod(value.c_str(), &end);
  if (end == value.c_str() || *end != '\0') {
    return fallback;
  }
  return parsed;
}

bool get_setting_bool(const EnvMap& env_file, const char* key, bool fallback) {
  auto value = get_setting(env_file, key, "");
  if (value.empty()) {
    return fallback;
  }
  std::string lowered = value;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
    return true;
  }
  if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
    return false;
  }
  return fallback;
}
}

Config load_config() {
  Config cfg;
  EnvMap env_file;
  cfg.config_source = "env";

  auto explicit_path = trim(get_env_only("NETOS_CONFIG_FILE"));
  std::string config_path;
  if (!explicit_path.empty()) {
    config_path = explicit_path;
  } else {
    auto topology_dir = trim(get_env_only("NETOS_TOPOLOGY_DIR"));
    if (!topology_dir.empty()) {
      auto node_id = trim(get_env_only("NETOS_NODE_ID"));
      if (node_id.empty()) {
        log_warn("NETOS_TOPOLOGY_DIR set but NETOS_NODE_ID is empty; skipping config file load.");
      } else {
        if (topology_dir.back() == '/') {
          config_path = topology_dir + node_id + ".env";
        } else {
          config_path = topology_dir + "/" + node_id + ".env";
        }
      }
    }
  }

  if (!config_path.empty()) {
    std::string error;
    if (load_env_file(config_path, &env_file, &error)) {
      cfg.config_source = config_path;
    } else {
      log_warn("config file load failed: " + error);
    }
  }

  cfg.node_id = get_setting(env_file, "NETOS_NODE_ID", "node");
  cfg.bind_ip = get_setting(env_file, "NETOS_BIND_IP", "0.0.0.0");
  cfg.bind_port = get_setting_int(env_file, "NETOS_BIND_PORT", 9000);
  cfg.redis_socket = get_setting(env_file, "NETOS_REDIS_SOCKET", "/var/run/redis/redis.sock");
  cfg.request_delay_ms = get_setting_int(env_file, "NETOS_REQUEST_DELAY_MS", 800);
  cfg.request_ttl = get_setting_int(env_file, "NETOS_REQUEST_TTL", 3);
  cfg.query_ttl_ms = get_setting_int(env_file, "NETOS_QUERY_TTL_MS", 1500);
  cfg.sync_table_capacity = get_setting_int(env_file, "NETOS_SYNC_TABLE_CAPACITY", 128);
  cfg.content_bf_bits = get_setting_int(env_file, "NETOS_CONTENT_BF_BITS", 512);
  cfg.content_bf_hashes = get_setting_int(env_file, "NETOS_CONTENT_BF_HASHES", 3);
  cfg.content_bf_exchange_ms = get_setting_int(env_file, "NETOS_CONTENT_BF_EXCHANGE_MS", 1000);
  cfg.content_bf_ttl_ms = get_setting_int(env_file, "NETOS_CONTENT_BF_TTL_MS", 3000);
  cfg.content_bf_fallback_ms = get_setting_int(env_file, "NETOS_CONTENT_BF_FALLBACK_MS", 300);
  cfg.query_bf_bits = get_setting_int(env_file, "NETOS_QUERY_BF_BITS", 1024);
  cfg.query_bf_hashes = get_setting_int(env_file, "NETOS_QUERY_BF_HASHES", 4);
  cfg.query_bf_aggregation_ms = get_setting_int(env_file, "NETOS_QUERY_BF_AGGREGATION_MS",
                                                get_setting_int(env_file, "NETOS_AGGREGATION_WINDOW_MS", 200));
  cfg.query_bf_ttl_ms = get_setting_int(env_file, "NETOS_QUERY_BF_TTL_MS", 2000);
  cfg.query_bf_max_fill_ratio = get_setting_double(env_file, "NETOS_QUERY_BF_MAX_FILL_RATIO", 0.9);
  cfg.query_bf_max_keys = get_setting_int(env_file, "NETOS_QUERY_BF_MAX_KEYS", 0);
  cfg.broadcast_attempt_limit = get_setting_int(env_file, "NETOS_BROADCAST_ATTEMPT_LIMIT", 3);
  cfg.broadcast_window_ms = get_setting_int(env_file, "NETOS_BROADCAST_WINDOW_MS", 1000);
  cfg.async_forward_enable = get_setting_bool(env_file, "NETOS_ASYNC_ENABLE", true);
  cfg.forward_workers = get_setting_int(env_file, "NETOS_FORWARD_WORKERS", 1);
  cfg.forward_queue_max = get_setting_int(env_file, "NETOS_FORWARD_QUEUE_MAX", 1024);
  cfg.forward_drop_policy = trim(get_setting(env_file, "NETOS_FORWARD_DROP_POLICY", "drop_newest"));
  cfg.log_level = trim(get_setting(env_file, "NETOS_LOG_LEVEL", ""));

  auto neighbors_raw = trim(get_setting(env_file, "NETOS_NEIGHBORS", ""));
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

  auto seed_raw = trim(get_setting(env_file, "NETOS_SEED_KEYS", ""));
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

  auto request_raw = trim(get_setting(env_file, "NETOS_REQUEST_KEYS", ""));
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
