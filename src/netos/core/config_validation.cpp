#include "core/config_validation.h"

#include <sstream>

namespace netos {

namespace {
std::string neighbor_list(const std::vector<NeighborConfig>& neighbors) {
  if (neighbors.empty()) {
    return "none";
  }
  std::ostringstream oss;
  for (size_t i = 0; i < neighbors.size(); ++i) {
    oss << neighbors[i].host << ":" << neighbors[i].port;
    if (i + 1 < neighbors.size()) {
      oss << ",";
    }
  }
  return oss.str();
}
}  // namespace

bool validate_config(const Config& config, std::string* error) {
  if (config.node_id.empty()) {
    if (error) {
      *error = "node_id is empty";
    }
    return false;
  }
  if (config.bind_port <= 0 || config.bind_port > 65535) {
    if (error) {
      *error = "bind_port must be between 1 and 65535";
    }
    return false;
  }
  if (config.request_ttl <= 0) {
    if (error) {
      *error = "request_ttl must be positive";
    }
    return false;
  }
  if (config.request_delay_ms < 0) {
    if (error) {
      *error = "request_delay_ms must be non-negative";
    }
    return false;
  }
  if (config.query_ttl_ms <= 0) {
    if (error) {
      *error = "query_ttl_ms must be positive";
    }
    return false;
  }
  if (config.sync_table_capacity <= 0) {
    if (error) {
      *error = "sync_table_capacity must be positive";
    }
    return false;
  }
  if (config.content_bf_bits <= 0) {
    if (error) {
      *error = "content_bf_bits must be positive";
    }
    return false;
  }
  if (config.content_bf_hashes <= 0) {
    if (error) {
      *error = "content_bf_hashes must be positive";
    }
    return false;
  }
  if (config.content_bf_exchange_ms <= 0) {
    if (error) {
      *error = "content_bf_exchange_ms must be positive";
    }
    return false;
  }
  if (config.content_bf_ttl_ms <= 0) {
    if (error) {
      *error = "content_bf_ttl_ms must be positive";
    }
    return false;
  }
  if (config.content_bf_fallback_ms <= 0) {
    if (error) {
      *error = "content_bf_fallback_ms must be positive";
    }
    return false;
  }
  if (config.query_bf_bits <= 0) {
    if (error) {
      *error = "query_bf_bits must be positive";
    }
    return false;
  }
  if (config.query_bf_hashes <= 0) {
    if (error) {
      *error = "query_bf_hashes must be positive";
    }
    return false;
  }
  if (config.query_bf_aggregation_ms <= 0) {
    if (error) {
      *error = "query_bf_aggregation_ms must be positive";
    }
    return false;
  }
  if (config.query_bf_ttl_ms <= 0) {
    if (error) {
      *error = "query_bf_ttl_ms must be positive";
    }
    return false;
  }
  if (config.query_bf_max_fill_ratio < 0.0 || config.query_bf_max_fill_ratio > 1.0) {
    if (error) {
      *error = "query_bf_max_fill_ratio must be between 0 and 1";
    }
    return false;
  }
  if (config.query_bf_max_keys < 0) {
    if (error) {
      *error = "query_bf_max_keys must be non-negative";
    }
    return false;
  }
  if (config.broadcast_attempt_limit <= 0) {
    if (error) {
      *error = "broadcast_attempt_limit must be positive";
    }
    return false;
  }
  if (config.broadcast_window_ms <= 0) {
    if (error) {
      *error = "broadcast_window_ms must be positive";
    }
    return false;
  }
  if (config.forward_workers <= 0) {
    if (error) {
      *error = "forward_workers must be positive";
    }
    return false;
  }
  if (config.forward_queue_max <= 0) {
    if (error) {
      *error = "forward_queue_max must be positive";
    }
    return false;
  }
  if (config.forward_drop_policy != "drop_newest") {
    if (error) {
      *error = "forward_drop_policy must be drop_newest";
    }
    return false;
  }
  for (const auto& neighbor : config.neighbors) {
    if (neighbor.host.empty()) {
      if (error) {
        *error = "neighbor host is empty";
      }
      return false;
    }
    if (neighbor.port <= 0 || neighbor.port > 65535) {
      if (error) {
        *error = "neighbor port must be between 1 and 65535";
      }
      return false;
    }
  }
  return true;
}

std::string config_summary(const Config& config) {
  std::ostringstream oss;
  oss << "node_id=" << config.node_id;
  if (!config.config_source.empty()) {
    oss << " source=" << config.config_source;
  }
  oss << " bind=" << config.bind_ip << ":" << config.bind_port;
  oss << " neighbors=" << neighbor_list(config.neighbors);
  oss << " seed_keys=" << config.seed_keys.size();
  oss << " request_keys=" << config.request_keys.size();
  oss << " request_delay_ms=" << config.request_delay_ms;
  oss << " request_ttl=" << config.request_ttl;
  oss << " query_ttl_ms=" << config.query_ttl_ms;
  oss << " sync_table_capacity=" << config.sync_table_capacity;
  oss << " content_bf_bits=" << config.content_bf_bits;
  oss << " content_bf_hashes=" << config.content_bf_hashes;
  oss << " content_bf_exchange_ms=" << config.content_bf_exchange_ms;
  oss << " content_bf_ttl_ms=" << config.content_bf_ttl_ms;
  oss << " content_bf_fallback_ms=" << config.content_bf_fallback_ms;
  oss << " query_bf_bits=" << config.query_bf_bits;
  oss << " query_bf_hashes=" << config.query_bf_hashes;
  oss << " query_bf_aggregation_ms=" << config.query_bf_aggregation_ms;
  oss << " query_bf_ttl_ms=" << config.query_bf_ttl_ms;
  oss << " query_bf_max_fill_ratio=" << config.query_bf_max_fill_ratio;
  oss << " query_bf_max_keys=" << config.query_bf_max_keys;
  oss << " broadcast_attempt_limit=" << config.broadcast_attempt_limit;
  oss << " broadcast_window_ms=" << config.broadcast_window_ms;
  oss << " async_forward_enable=" << (config.async_forward_enable ? "true" : "false");
  oss << " forward_workers=" << config.forward_workers;
  oss << " forward_queue_max=" << config.forward_queue_max;
  oss << " forward_drop_policy=" << config.forward_drop_policy;
  if (!config.log_level.empty()) {
    oss << " log_level=" << config.log_level;
  }
  return oss.str();
}

}
