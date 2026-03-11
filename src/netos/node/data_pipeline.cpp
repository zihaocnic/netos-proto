#include "node/data_pipeline.h"

namespace netos {
namespace {

std::string validate_data_fields(const Message& msg) {
  if (msg.request_id.empty()) {
    return "missing_request_id";
  }
  if (msg.origin.empty()) {
    return "missing_origin";
  }
  if (msg.key.empty()) {
    return "missing_key";
  }
  if (msg.ttl <= 0) {
    return "non_positive_ttl";
  }
  return "";
}

}  // namespace

std::string data_state_label(DataState state) {
  switch (state) {
    case DataState::DropInvalid:
      return "drop_invalid";
    case DataState::DropNotOrigin:
      return "drop_not_origin";
    case DataState::StoreLocal:
      return "store_local";
  }
  return "unknown";
}

DataDecision run_data_pipeline(const Config& config, const Message& msg) {
  DataDecision decision;
  auto invalid_reason = validate_data_fields(msg);
  if (!invalid_reason.empty()) {
    decision.state = DataState::DropInvalid;
    decision.reason = invalid_reason;
    return decision;
  }
  if (msg.origin != config.node_id) {
    decision.state = DataState::DropNotOrigin;
    return decision;
  }
  decision.state = DataState::StoreLocal;
  return decision;
}

}
