#include "node/node.h"

#include "core/config_validation.h"
#include "core/logger.h"
#include "node/data_pipeline.h"
#include "node/request_pipeline.h"
#include "network/topology.h"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <thread>

namespace netos {
namespace {

std::string bool_label(bool value) {
  return value ? "true" : "false";
}

std::string query_stats_suffix(const QueryTable::Stats& stats) {
  return " query_table_size=" + std::to_string(stats.size) +
         " query_table_attempts=" + std::to_string(stats.attempts) +
         " query_table_duplicates=" + std::to_string(stats.duplicates) +
         " query_table_pruned=" + std::to_string(stats.pruned);
}

uint64_t now_millis() {
  auto now = std::chrono::system_clock::now();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

bool parse_uint64(const std::string& value, uint64_t* out) {
  if (!out || value.empty()) {
    return false;
  }
  char* end = nullptr;
  unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
  if (end == value.c_str() || *end != '\0') {
    return false;
  }
  *out = static_cast<uint64_t>(parsed);
  return true;
}

}  // namespace

Node::Node(Config config)
    : config_(std::move(config)),
      redis_(config_.redis_socket),
      network_(config_.bind_ip, config_.bind_port),
      forward_queue_(&network_,
                     ForwardQueueConfig{config_.async_forward_enable,
                                        config_.forward_workers,
                                        static_cast<size_t>(config_.forward_queue_max),
                                        config_.forward_drop_policy}),
      query_table_(std::chrono::milliseconds(config_.query_ttl_ms)),
      sync_table_(static_cast<size_t>(config_.sync_table_capacity)),
      content_bloom_(static_cast<size_t>(config_.content_bf_bits),
                     static_cast<size_t>(config_.content_bf_hashes),
                     std::chrono::milliseconds(config_.content_bf_ttl_ms)),
      content_bf_fallback_(std::chrono::milliseconds(config_.content_bf_fallback_ms)),
      query_bloom_aggregator_(static_cast<size_t>(config_.query_bf_bits),
                              static_cast<size_t>(config_.query_bf_hashes),
                              std::chrono::milliseconds(config_.query_bf_aggregation_ms),
                              std::chrono::milliseconds(config_.query_bf_ttl_ms),
                              config_.query_bf_max_fill_ratio,
                              static_cast<size_t>(config_.query_bf_max_keys)),
      query_bloom_limiter_(static_cast<size_t>(config_.broadcast_attempt_limit),
                           std::chrono::milliseconds(config_.broadcast_window_ms)),
      request_counter_(0),
      running_(false) {}

bool Node::init(std::string* error) {
  if (!validate_config(config_, error)) {
    return false;
  }
  if (!redis_.connect(error)) {
    return false;
  }

  Topology topology;
  if (!build_topology(config_, &topology, error)) {
    return false;
  }
  network_.set_topology(std::move(topology));

  return true;
}

void Node::run() {
  std::string error;
  if (!network_.start([this](const std::string& wire, const sockaddr_in& from) {
        handle_wire_message(wire, from);
      },
      &error)) {
    log_error("udp transport start failed: " + error);
    return;
  }

  log_info("config " + config_summary(config_));
  log_info("node " + config_.node_id + " listening on " + config_.bind_ip + ":" +
           std::to_string(config_.bind_port));

  running_.store(true);
  forward_queue_.start(&running_);
  seed_cache();
  start_background_tasks();
  schedule_requests();
  while (running_.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void Node::handle_wire_message(const std::string& wire, const sockaddr_in& from) {
  auto msg_opt = Message::from_wire(wire);
  if (!msg_opt.has_value()) {
    log_warn("received malformed message from " + addr_to_string(from));
    return;
  }
  auto msg = msg_opt.value();
  switch (msg.type) {
    case MessageType::Request:
      handle_request(msg, from);
      break;
    case MessageType::Data:
      handle_data(msg, from);
      break;
    case MessageType::ContentBloom:
      handle_content_bloom(msg, from);
      break;
    case MessageType::QueryBloom:
      handle_query_bloom(msg, from);
      break;
    default:
      log_warn("received unknown message type from " + addr_to_string(from));
      break;
  }
}

void Node::handle_request(const Message& msg, const sockaddr_in& from) {
  RequestPipeline pipeline(query_table_, redis_);
  auto decision = pipeline.run(msg);
  switch (decision.state) {
    case RequestState::DropInvalid:
      log_warn("req_state=" + request_state_label(decision.state) + " reason=" + decision.reason +
               " id=" + msg.request_id + " key=" + msg.key +
               " ttl=" + std::to_string(msg.ttl) + " origin=" + msg.origin +
               " from=" + addr_to_string(from));
      return;
    case RequestState::DropTtl:
      log_info("req_state=" + request_state_label(decision.state) + " reason=" + decision.reason +
               " id=" + msg.request_id + " key=" + msg.key +
               " ttl=" + std::to_string(msg.ttl) + " origin=" + msg.origin +
               " from=" + addr_to_string(from));
      return;
    case RequestState::DropDuplicate:
      {
        auto stats = query_table_.stats();
        log_info("req_state=" + request_state_label(decision.state) + " id=" + msg.request_id +
                 " key=" + msg.key + " origin=" + msg.origin +
                 " from=" + addr_to_string(from) + query_stats_suffix(stats));
      }
      return;
    case RequestState::ServeLocal: {
      Message resp;
      resp.type = MessageType::Data;
      resp.request_id = msg.request_id;
      resp.origin = msg.origin;
      resp.ttl = config_.request_ttl;
      resp.key = msg.key;
      resp.value = decision.value.value_or("");
      record_local_key(msg.key);
      auto query_stats = query_table_.stats();
      std::string query_stats_log = query_stats_suffix(query_stats);
      std::string from_label = addr_to_string(from);
      auto on_success = [this,
                         request_id = resp.request_id,
                         key = resp.key,
                         origin = resp.origin,
                         from_label,
                         query_stats_log]() {
        SyncTable::Update update;
        SyncTable::Stats sync_stats;
        {
          std::lock_guard<std::mutex> lock(sync_table_mutex_);
          update = sync_table_.record_destination(key, origin);
          sync_stats = sync_table_.stats();
        }
        log_info("req_state=serve_local id=" + request_id +
                 " key=" + key + " dest=" + from_label +
                 " origin=" + origin + " from=" + from_label +
                 query_stats_log);
        log_info("sync_table=update key=" + key + " origin=" + origin +
                 " new_key=" + bool_label(update.key_added) +
                 " new_destination=" + bool_label(update.destination_added) +
                 " duplicate_destination=" + bool_label(update.destination_duplicate) +
                 " destinations=" + std::to_string(update.destination_count) +
                 " size=" + std::to_string(update.key_count) +
                 " evicted=" + std::to_string(update.evicted) +
                 " sync_table_updates=" + std::to_string(sync_stats.updates) +
                 " sync_table_duplicate_hits=" + std::to_string(sync_stats.duplicate_destinations) +
                 " sync_table_destinations_total=" + std::to_string(sync_stats.destination_total) +
                 " sync_table_evicted_total=" + std::to_string(sync_stats.evicted));
      };
      auto on_failure = [request_id = resp.request_id,
                         key = resp.key,
                         origin = resp.origin,
                         from_label](const std::string& error) {
        log_warn("req_state=serve_failed id=" + request_id + " key=" + key +
                 " origin=" + origin + " dest=" + from_label +
                 " from=" + from_label + " error=" + error);
      };
      forward_queue_.send_direct(from, resp, std::move(on_success), std::move(on_failure));
      return;
    }
    case RequestState::Forward: {
      Message forward = msg;
      forward.ttl = decision.forward_ttl;
      log_debug("req_state=" + request_state_label(decision.state) + " id=" + msg.request_id +
                " key=" + msg.key + " ttl_in=" + std::to_string(msg.ttl) +
                " ttl=" + std::to_string(forward.ttl) + " origin=" + msg.origin +
                " from=" + addr_to_string(from));
      forward_queue_.send_broadcast(forward, &from);
      return;
    }
  }
}

void Node::handle_data(const Message& msg, const sockaddr_in& from) {
  DataPipeline pipeline(config_);
  auto decision = pipeline.run(msg);
  if (decision.state == DataState::DropInvalid) {
    log_warn("data_state=" + data_state_label(decision.state) + " reason=" + decision.reason +
             " id=" + msg.request_id + " key=" + msg.key + " ttl=" +
             std::to_string(msg.ttl) + " origin=" + msg.origin +
             " from=" + addr_to_string(from));
    return;
  }
  if (decision.state == DataState::DropNotOrigin) {
    log_info("data_state=" + data_state_label(decision.state) + " id=" + msg.request_id +
             " key=" + msg.key + " origin=" + msg.origin +
             " ttl=" + std::to_string(msg.ttl) + " from=" + addr_to_string(from) +
             " at=" + config_.node_id);
    return;
  }

  std::string error;
  if (!redis_.set(msg.key, msg.value, &error)) {
    log_warn("data_state=store_failed id=" + msg.request_id + " key=" + msg.key +
             " origin=" + msg.origin + " from=" + addr_to_string(from) +
             " error=" + error);
    return;
  }

  content_bf_fallback_.cancel(msg.key, msg.origin);
  record_local_key(msg.key);
  log_info("data_state=" + data_state_label(decision.state) + " id=" + msg.request_id +
           " key=" + msg.key + " origin=" + msg.origin +
           " ttl=" + std::to_string(msg.ttl) + " from=" + addr_to_string(from));
}

void Node::handle_content_bloom(const Message& msg, const sockaddr_in& from) {
  if (msg.origin.empty()) {
    log_warn("bf_state=content_drop reason=missing_origin from=" + addr_to_string(from));
    return;
  }
  if (msg.key.empty()) {
    log_warn("bf_state=content_drop reason=missing_bloom from=" + addr_to_string(from) +
             " origin=" + msg.origin);
    return;
  }
  auto filter_opt = BloomFilter::from_hex(static_cast<size_t>(config_.content_bf_bits),
                                          static_cast<size_t>(config_.content_bf_hashes), msg.key);
  if (!filter_opt.has_value()) {
    log_warn("bf_state=content_drop reason=bad_bloom from=" + addr_to_string(from) +
             " origin=" + msg.origin);
    return;
  }
  content_bloom_.update_neighbor(from, *filter_opt);
  log_info("bf_state=content_update origin=" + msg.origin +
           " from=" + addr_to_string(from) +
           " bf_bits=" + std::to_string(config_.content_bf_bits) +
           " bf_hashes=" + std::to_string(config_.content_bf_hashes) +
           " bf_bytes=" + std::to_string(filter_opt->byte_count()) +
           " bf_fill=" + std::to_string(filter_opt->fill_ratio()) +
           " neighbor_summaries=" + std::to_string(content_bloom_.neighbor_count()));
}

void Node::handle_query_bloom(const Message& msg, const sockaddr_in& from) {
  if (msg.origin.empty()) {
    log_warn("req_state=drop_invalid reason=missing_origin bf_type=query id=" + msg.request_id +
             " from=" + addr_to_string(from));
    return;
  }
  if (msg.ttl <= 0) {
    log_info("req_state=drop_ttl reason=ttl_expired bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " ttl=" + std::to_string(msg.ttl) +
             " from=" + addr_to_string(from));
    return;
  }
  if (msg.key.empty()) {
    log_warn("req_state=drop_invalid reason=missing_bloom bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " from=" + addr_to_string(from));
    return;
  }
  if (msg.value.empty()) {
    log_warn("req_state=drop_invalid reason=missing_created_ms bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " from=" + addr_to_string(from));
    return;
  }
  uint64_t created_ms = 0;
  if (!parse_uint64(msg.value, &created_ms)) {
    log_warn("req_state=drop_invalid reason=bad_created_ms bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " from=" + addr_to_string(from));
    return;
  }
  uint64_t now_ms = now_millis();
  uint64_t age_ms = now_ms >= created_ms ? (now_ms - created_ms) : 0;
  if (age_ms > static_cast<uint64_t>(config_.query_bf_ttl_ms)) {
    log_info("req_state=drop_suppressed reason=bf_expired bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " bf_age_ms=" + std::to_string(age_ms) +
             " from=" + addr_to_string(from));
    return;
  }
  auto filter_opt = BloomFilter::from_hex(static_cast<size_t>(config_.query_bf_bits),
                                          static_cast<size_t>(config_.query_bf_hashes), msg.key);
  if (!filter_opt.has_value()) {
    log_warn("req_state=drop_invalid reason=bad_bloom bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " from=" + addr_to_string(from));
    return;
  }
  auto filter = *filter_opt;
  double bf_fill = filter.fill_ratio();

  std::vector<std::string> keys;
  {
    std::lock_guard<std::mutex> lock(local_keys_mutex_);
    keys.assign(local_keys_.begin(), local_keys_.end());
  }
  std::string from_label = addr_to_string(from);
  for (const auto& key : keys) {
    if (!filter.maybe_contains(key)) {
      continue;
    }
    std::string error;
    auto value = redis_.get(key, &error);
    if (!error.empty()) {
      log_warn("redis get error: " + error);
    }
    if (!value.has_value()) {
      continue;
    }
    Message resp;
    resp.type = MessageType::Data;
    resp.request_id = make_request_id();
    resp.origin = msg.origin;
    resp.ttl = config_.request_ttl;
    resp.key = key;
    resp.value = value.value();
    auto on_success = [this,
                       request_id = resp.request_id,
                       key,
                       origin = msg.origin,
                       bf_id = msg.request_id,
                       bf_age_ms = age_ms,
                       from_label]() {
      SyncTable::Update update;
      SyncTable::Stats sync_stats;
      {
        std::lock_guard<std::mutex> lock(sync_table_mutex_);
        update = sync_table_.record_destination(key, origin);
        sync_stats = sync_table_.stats();
      }
      log_info("req_state=serve_local bf_type=query bf_id=" + bf_id +
               " id=" + request_id + " key=" + key +
               " dest=" + from_label + " origin=" + origin +
               " from=" + from_label + " bf_age_ms=" + std::to_string(bf_age_ms));
      log_info("sync_table=update key=" + key + " origin=" + origin +
               " new_key=" + bool_label(update.key_added) +
               " new_destination=" + bool_label(update.destination_added) +
               " duplicate_destination=" + bool_label(update.destination_duplicate) +
               " destinations=" + std::to_string(update.destination_count) +
               " size=" + std::to_string(update.key_count) +
               " evicted=" + std::to_string(update.evicted) +
               " sync_table_updates=" + std::to_string(sync_stats.updates) +
               " sync_table_duplicate_hits=" + std::to_string(sync_stats.duplicate_destinations) +
               " sync_table_destinations_total=" + std::to_string(sync_stats.destination_total) +
               " sync_table_evicted_total=" + std::to_string(sync_stats.evicted));
    };
    auto on_failure = [request_id = resp.request_id,
                       key,
                       origin = msg.origin,
                       bf_id = msg.request_id,
                       from_label](const std::string& error) {
      log_warn("req_state=serve_failed bf_type=query bf_id=" + bf_id +
               " id=" + request_id + " key=" + key + " origin=" + origin +
               " dest=" + from_label + " from=" + from_label +
               " error=" + error);
    };
    forward_queue_.send_direct(from, resp, std::move(on_success), std::move(on_failure));
  }

  int forward_ttl = msg.ttl - 1;
  if (forward_ttl <= 0) {
    log_info("req_state=drop_ttl reason=ttl_expired bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " ttl_in=" + std::to_string(msg.ttl) +
             " from=" + addr_to_string(from));
    return;
  }
  if (!query_bloom_limiter_.allow(msg.origin, filter.digest())) {
    log_info("req_state=drop_suppressed reason=attempt_limit bf_type=query id=" + msg.request_id +
             " origin=" + msg.origin + " bf_age_ms=" + std::to_string(age_ms) +
             " from=" + addr_to_string(from));
    return;
  }
  Message forward = msg;
  forward.ttl = forward_ttl;
  log_info("req_state=forward bf_type=query id=" + msg.request_id +
           " origin=" + msg.origin + " ttl_in=" + std::to_string(msg.ttl) +
           " ttl=" + std::to_string(forward.ttl) +
           " bf_age_ms=" + std::to_string(age_ms) +
           " bf_fill=" + std::to_string(bf_fill) +
           " from=" + addr_to_string(from));
  forward_queue_.send_broadcast(forward, &from);
}

void Node::start_background_tasks() {
  start_content_bloom_exchange();
  start_content_bf_fallbacks();
  start_query_bloom_flush();
}

void Node::start_content_bloom_exchange() {
  std::thread([this]() {
    auto interval = std::chrono::milliseconds(config_.content_bf_exchange_ms);
    while (running_.load()) {
      auto snapshot = content_bloom_.local_snapshot();
      Message msg;
      msg.type = MessageType::ContentBloom;
      msg.origin = config_.node_id;
      msg.ttl = config_.content_bf_ttl_ms;
      msg.key = snapshot.to_hex();
      msg.value.clear();
      log_debug("bf_state=content_send origin=" + config_.node_id +
                " bf_bits=" + std::to_string(config_.content_bf_bits) +
                " bf_hashes=" + std::to_string(config_.content_bf_hashes) +
                " bf_bytes=" + std::to_string(snapshot.byte_count()) +
                " bf_fill=" + std::to_string(snapshot.fill_ratio()));
      forward_queue_.send_broadcast(msg, nullptr);
      std::this_thread::sleep_for(interval);
    }
  }).detach();
}

void Node::start_content_bf_fallbacks() {
  std::thread([this]() {
    int interval_ms = config_.content_bf_fallback_ms / 2;
    if (interval_ms < 25) {
      interval_ms = 25;
    }
    auto interval = std::chrono::milliseconds(interval_ms);
    while (running_.load()) {
      auto due = content_bf_fallback_.take_due();
      for (const auto& entry : due) {
        auto add_result = query_bloom_aggregator_.add_key(entry.origin, entry.key);
        if (add_result.split) {
          log_info("bf_state=query_split bf_type=query origin=" + add_result.origin +
                   " bf_id=" + add_result.batch_id +
                   " reason=" + add_result.split_reason +
                   " bf_fill=" + std::to_string(add_result.fill_ratio) +
                   " bf_fill_max=" + std::to_string(config_.query_bf_max_fill_ratio) +
                   " bf_keys=" + std::to_string(add_result.key_count) +
                   " bf_keys_max=" + std::to_string(config_.query_bf_max_keys) +
                   " from=local");
        }
        log_info("req_state=drop_suppressed reason=content_fallback bf_type=query key=" +
                 entry.key + " origin=" + entry.origin + " from=local");
      }
      std::this_thread::sleep_for(interval);
    }
  }).detach();
}

void Node::start_query_bloom_flush() {
  std::thread([this]() {
    int interval_ms = config_.query_bf_aggregation_ms / 2;
    if (interval_ms < 50) {
      interval_ms = 50;
    }
    auto interval = std::chrono::milliseconds(interval_ms);
    while (running_.load()) {
      auto batches = query_bloom_aggregator_.flush_due();
      uint64_t now_ms = now_millis();
      for (const auto& batch : batches) {
        Message msg;
        msg.type = MessageType::QueryBloom;
        msg.request_id = batch.batch_id;
        msg.origin = batch.origin;
        msg.ttl = config_.request_ttl;
        msg.key = batch.filter.to_hex();
        msg.value = std::to_string(batch.created_ms);
        uint64_t age_ms = now_ms >= batch.created_ms ? (now_ms - batch.created_ms) : 0;
        double bf_fill = batch.filter.fill_ratio();
        log_info("req_state=forward bf_type=query id=" + msg.request_id +
                 " origin=" + msg.origin + " ttl=" + std::to_string(msg.ttl) +
                 " bf_age_ms=" + std::to_string(age_ms) +
                 " bf_bits=" + std::to_string(config_.query_bf_bits) +
                 " bf_hashes=" + std::to_string(config_.query_bf_hashes) +
                 " bf_fill=" + std::to_string(bf_fill) +
                 " bf_keys=" + std::to_string(batch.key_count));
        forward_queue_.send_broadcast(msg, nullptr);
      }
      std::this_thread::sleep_for(interval);
    }
  }).detach();
}

void Node::record_local_key(const std::string& key) {
  {
    std::lock_guard<std::mutex> lock(local_keys_mutex_);
    local_keys_.insert(key);
  }
  content_bloom_.add_local_key(key);
}

std::string Node::make_request_id() {
  auto now = std::chrono::system_clock::now();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  uint64_t counter = request_counter_.fetch_add(1);
  return config_.node_id + "-" + std::to_string(millis) + "-" + std::to_string(counter);
}

void Node::seed_cache() {
  if (config_.seed_keys.empty()) {
    return;
  }
  for (const auto& kv : config_.seed_keys) {
    std::string error;
    if (!redis_.set(kv.key, kv.value, &error)) {
      log_warn("seed failed for key " + kv.key + ": " + error);
    } else {
      record_local_key(kv.key);
      log_info("seeded key " + kv.key);
    }
  }
}

void Node::schedule_requests() {
  if (config_.request_keys.empty()) {
    return;
  }
  std::thread([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(config_.request_delay_ms));
    for (const auto& key : config_.request_keys) {
      std::string error;
      auto local_value = redis_.get(key, &error);
      if (!error.empty()) {
        log_warn("redis get error: " + error);
      }
      if (local_value.has_value()) {
        record_local_key(key);
        auto stats = query_table_.stats();
        log_info("req_state=serve_local id=" + make_request_id() + " key=" + key +
                 " origin=" + config_.node_id + " from=local" + query_stats_suffix(stats));
        continue;
      }

      Message req;
      req.type = MessageType::Request;
      req.request_id = make_request_id();
      req.origin = config_.node_id;
      req.ttl = config_.request_ttl;
      req.key = key;
      query_table_.record_if_new(req.request_id);
      auto stats = query_table_.stats();
      log_info("req_state=originated id=" + req.request_id + " key=" + key +
               " ttl=" + std::to_string(req.ttl) + " origin=" + req.origin +
               " from=local" + query_stats_suffix(stats));

      auto neighbor = content_bloom_.select_neighbor(key);
      if (neighbor.has_value()) {
        std::string neighbor_label = addr_to_string(*neighbor);
        log_info("bf_state=content_direct id=" + req.request_id + " key=" + key +
                 " origin=" + req.origin + " neighbor=" + neighbor_label);
        auto on_failure = [request_id = req.request_id,
                           key,
                           origin = req.origin,
                           neighbor_label](const std::string& error) {
          log_warn("bf_state=content_direct_failed id=" + request_id + " key=" + key +
                   " origin=" + origin + " dest=" + neighbor_label +
                   " error=" + error);
        };
        forward_queue_.send_direct(*neighbor, req, std::function<void()>(), std::move(on_failure));
        content_bf_fallback_.schedule(key, req.origin);
      } else {
        auto add_result = query_bloom_aggregator_.add_key(req.origin, key);
        if (add_result.split) {
          log_info("bf_state=query_split bf_type=query origin=" + add_result.origin +
                   " bf_id=" + add_result.batch_id +
                   " reason=" + add_result.split_reason +
                   " bf_fill=" + std::to_string(add_result.fill_ratio) +
                   " bf_fill_max=" + std::to_string(config_.query_bf_max_fill_ratio) +
                   " bf_keys=" + std::to_string(add_result.key_count) +
                   " bf_keys_max=" + std::to_string(config_.query_bf_max_keys) +
                   " from=local");
        }
        log_info("req_state=drop_suppressed reason=aggregate_window bf_type=query id=" +
                 req.request_id + " key=" + key + " origin=" + req.origin + " from=local");
      }
    }
  }).detach();
}

}
