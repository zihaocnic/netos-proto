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

class RequestPipeline {
 public:
  RequestPipeline(QueryTable& query_table, RedisClient& redis);

  RequestDecision run(const Message& msg);

 private:
  bool validate_fields(const Message& msg, RequestDecision* decision) const;
  bool validate_ttl(const Message& msg, RequestDecision* decision) const;
  bool dedupe_request(const Message& msg, RequestDecision* decision);
  bool check_cache(const Message& msg, RequestDecision* decision);

  QueryTable& query_table_;
  RedisClient& redis_;
};

RequestDecision run_request_pipeline(QueryTable& query_table,
                                     RedisClient& redis,
                                     const Message& msg);

}
