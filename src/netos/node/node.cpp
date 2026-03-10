#include "node/node.h"

#include "core/config_validation.h"
#include "core/logger.h"

#include <chrono>
#include <thread>

namespace netos {

Node::Node(Config config)
    : config_(std::move(config)),
      redis_(config_.redis_socket),
      transport_(config_.bind_ip, config_.bind_port),
      query_table_(std::chrono::milliseconds(config_.query_ttl_ms)),
      sync_table_(static_cast<size_t>(config_.sync_table_capacity)),
      request_counter_(0),
      running_(false) {}

bool Node::init(std::string* error) {
  if (!validate_config(config_, error)) {
    return false;
  }
  if (!redis_.connect(error)) {
    return false;
  }

  for (const auto& neighbor : config_.neighbors) {
    NeighborAddress addr;
    addr.host = neighbor.host;
    addr.port = neighbor.port;
    if (!resolve_address(neighbor.host, neighbor.port, &addr.addr, error)) {
      return false;
    }
    neighbors_.push_back(addr);
  }

  return true;
}

void Node::run() {
  std::string error;
  if (!transport_.start([this](const std::string& wire, const sockaddr_in& from) {
        handle_wire_message(wire, from);
      },
      &error)) {
    log_error("udp transport start failed: " + error);
    return;
  }

  log_info("config " + config_summary(config_));
  log_info("node " + config_.node_id + " listening on " + config_.bind_ip + ":" +
           std::to_string(config_.bind_port));

  seed_cache();
  schedule_requests();

  running_ = true;
  while (running_) {
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
    default:
      log_warn("received unknown message type from " + addr_to_string(from));
      break;
  }
}

void Node::handle_request(const Message& msg, const sockaddr_in& from) {
  if (msg.ttl <= 0) {
    log_debug("drop request (ttl) " + msg.request_id);
    return;
  }
  if (query_table_.seen(msg.request_id)) {
    log_debug("drop duplicate request " + msg.request_id);
    return;
  }
  query_table_.record(msg.request_id);

  std::string error;
  auto value = redis_.get(msg.key, &error);
  if (!error.empty()) {
    log_warn("redis get error: " + error);
  }

  if (value.has_value()) {
    Message resp;
    resp.type = MessageType::Data;
    resp.request_id = msg.request_id;
    resp.origin = msg.origin;
    resp.ttl = config_.request_ttl;
    resp.key = msg.key;
    resp.value = value.value();
    std::string send_error;
    if (!transport_.send_to(from, resp.to_wire(), &send_error)) {
      log_warn("failed to send data to " + addr_to_string(from) + ": " + send_error);
    } else {
      sync_table_.record_destination(msg.key, msg.origin);
      log_info("served key " + msg.key + " to " + addr_to_string(from));
    }
    return;
  }

  Message forward = msg;
  forward.ttl = msg.ttl - 1;
  broadcast_request(forward, &from);
}

void Node::handle_data(const Message& msg, const sockaddr_in& from) {
  if (msg.origin != config_.node_id) {
    log_info("data for origin " + msg.origin + " received at " + config_.node_id +
             " (forwarding not implemented)");
    return;
  }

  std::string error;
  if (!redis_.set(msg.key, msg.value, &error)) {
    log_warn("failed to store key " + msg.key + ": " + error);
    return;
  }

  log_info("stored key " + msg.key + " from " + addr_to_string(from));
}

void Node::broadcast_request(const Message& msg, const sockaddr_in* exclude) {
  for (const auto& neighbor : neighbors_) {
    if (exclude && same_address(neighbor.addr, *exclude)) {
      continue;
    }
    send_to_neighbor(neighbor, msg);
  }
}

bool Node::send_to_neighbor(const NeighborAddress& neighbor, const Message& msg) {
  std::string error;
  if (!transport_.send_to(neighbor.addr, msg.to_wire(), &error)) {
    log_warn("send failed to " + neighbor.host + ":" + std::to_string(neighbor.port) +
             " - " + error);
    return false;
  }
  return true;
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
      Message req;
      req.type = MessageType::Request;
      req.request_id = make_request_id();
      req.origin = config_.node_id;
      req.ttl = config_.request_ttl;
      req.key = key;
      query_table_.record(req.request_id);
      log_info("broadcast request for key " + key);
      broadcast_request(req, nullptr);
    }
  }).detach();
}

}
