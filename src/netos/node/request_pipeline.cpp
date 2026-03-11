#include "node/request_pipeline.h"

#include "core/logger.h"

namespace netos {
namespace {

std::string validate_request_fields(const Message& msg) {
  if (msg.request_id.empty()) {
    return "missing_request_id";
  }
  if (msg.origin.empty()) {
    return "missing_origin";
  }
  if (msg.key.empty()) {
    return "missing_key";
  }
  return "";
}

RequestDecision make_drop_decision(RequestState state, std::string reason) {
  RequestDecision decision;
  decision.state = state;
  decision.reason = std::move(reason);
  return decision;
}

RequestDecision make_serve_decision(const std::string& value) {
  RequestDecision decision;
  decision.state = RequestState::ServeLocal;
  decision.value = value;
  return decision;
}

RequestDecision make_forward_decision(int ttl) {
  RequestDecision decision;
  decision.state = RequestState::Forward;
  decision.forward_ttl = ttl;
  return decision;
}

}  // namespace

std::string request_state_label(RequestState state) {
  switch (state) {
    case RequestState::DropInvalid:
      return "drop_invalid";
    case RequestState::DropTtl:
      return "drop_ttl";
    case RequestState::DropDuplicate:
      return "drop_duplicate";
    case RequestState::ServeLocal:
      return "serve_local";
    case RequestState::Forward:
      return "forward";
  }
  return "unknown";
}

RequestPipeline::RequestPipeline(QueryTable& query_table, RedisClient& redis)
    : query_table_(query_table), redis_(redis) {}

RequestDecision RequestPipeline::run(const Message& msg) {
  RequestDecision decision;
  // Pipeline order: validate fields -> validate ttl -> dedupe -> cache lookup -> forward.
  if (!validate_fields(msg, &decision)) {
    return decision;
  }
  if (!validate_ttl(msg, &decision)) {
    return decision;
  }
  if (!dedupe_request(msg, &decision)) {
    return decision;
  }
  if (!check_cache(msg, &decision)) {
    return decision;
  }
  return make_forward_decision(msg.ttl - 1);
}

bool RequestPipeline::validate_fields(const Message& msg, RequestDecision* decision) const {
  auto invalid_reason = validate_request_fields(msg);
  if (invalid_reason.empty()) {
    return true;
  }
  *decision = make_drop_decision(RequestState::DropInvalid, invalid_reason);
  return false;
}

bool RequestPipeline::validate_ttl(const Message& msg, RequestDecision* decision) const {
  if (msg.ttl > 0) {
    return true;
  }
  *decision = make_drop_decision(RequestState::DropTtl, "ttl_expired");
  return false;
}

bool RequestPipeline::dedupe_request(const Message& msg, RequestDecision* decision) {
  if (query_table_.record_if_new(msg.request_id)) {
    return true;
  }
  *decision = make_drop_decision(RequestState::DropDuplicate, "duplicate_request_id");
  return false;
}

bool RequestPipeline::check_cache(const Message& msg, RequestDecision* decision) {
  std::string error;
  auto value = redis_.get(msg.key, &error);
  if (!error.empty()) {
    log_warn("redis get error: " + error);
  }
  if (!value.has_value()) {
    return true;
  }
  *decision = make_serve_decision(*value);
  return false;
}

}
