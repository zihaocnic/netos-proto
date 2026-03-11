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

RequestDecision run_request_pipeline(QueryTable& query_table,
                                     RedisClient& redis,
                                     const Message& msg) {
  auto invalid_reason = validate_request_fields(msg);
  if (!invalid_reason.empty()) {
    RequestDecision decision;
    decision.state = RequestState::DropInvalid;
    decision.reason = invalid_reason;
    return decision;
  }
  if (msg.ttl <= 0) {
    RequestDecision decision;
    decision.state = RequestState::DropTtl;
    decision.reason = "ttl_expired";
    return decision;
  }
  if (!query_table.record_if_new(msg.request_id)) {
    RequestDecision decision;
    decision.state = RequestState::DropDuplicate;
    decision.reason = "duplicate_request_id";
    return decision;
  }

  std::string error;
  auto value = redis.get(msg.key, &error);
  if (!error.empty()) {
    log_warn("redis get error: " + error);
  }

  if (value.has_value()) {
    RequestDecision decision;
    decision.state = RequestState::ServeLocal;
    decision.value = value;
    return decision;
  }

  RequestDecision decision;
  decision.state = RequestState::Forward;
  decision.forward_ttl = msg.ttl - 1;
  return decision;
}

}
