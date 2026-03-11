#pragma once

#include "cache/redis_client.h"
#include "core/message.h"
#include "tables/query_table.h"

#include <optional>
#include <string>

namespace netos {

enum class RequestState {
  DropInvalid,
  DropTtl,
  DropDuplicate,
  ServeLocal,
  Forward
};

struct RequestDecision {
  RequestState state = RequestState::DropTtl;
  // forward_ttl is only meaningful when state == Forward.
  int forward_ttl = 0;
  std::optional<std::string> value;
  std::string reason;
};

std::string request_state_label(RequestState state);

RequestDecision run_request_pipeline(QueryTable& query_table,
                                     RedisClient& redis,
                                     const Message& msg);

}
