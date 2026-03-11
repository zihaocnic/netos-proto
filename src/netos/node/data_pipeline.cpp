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

DataDecision make_drop_decision(DataState state, std::string reason) {
  DataDecision decision;
  decision.state = state;
  decision.reason = std::move(reason);
  return decision;
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

DataPipeline::DataPipeline(const Config& config) : config_(config) {}

DataDecision DataPipeline::run(const Message& msg) const {
  DataDecision decision;
  // Pipeline order: validate fields -> check origin -> store local.
  if (!validate_fields(msg, &decision)) {
    return decision;
  }
  if (!check_origin(msg, &decision)) {
    return decision;
  }
  decision.state = DataState::StoreLocal;
  return decision;
}

bool DataPipeline::validate_fields(const Message& msg, DataDecision* decision) const {
  auto invalid_reason = validate_data_fields(msg);
  if (invalid_reason.empty()) {
    return true;
  }
  *decision = make_drop_decision(DataState::DropInvalid, invalid_reason);
  return false;
}

bool DataPipeline::check_origin(const Message& msg, DataDecision* decision) const {
  if (msg.origin == config_.node_id) {
    return true;
  }
  *decision = make_drop_decision(DataState::DropNotOrigin, "");
  return false;
}

DataDecision run_data_pipeline(const Config& config, const Message& msg) {
  DataPipeline pipeline(config);
  return pipeline.run(msg);
}

}
