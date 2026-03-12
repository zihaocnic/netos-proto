#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace netos {

// QueryTable tracks recently seen request IDs for TTL-based duplicate suppression.
// It does not store keys/values; it only remembers request IDs for a short window.
// Design note: QueryTable is single-source per request_id. It does not track multiple
// requesters or coalesce duplicates across different origins; any repeat request_id
// is treated as a duplicate regardless of sender.
class QueryTable {
 public:
  explicit QueryTable(std::chrono::milliseconds ttl);

  // Returns true if the request_id was not seen recently and is now recorded.
  // No per-origin tracking: request_id alone defines uniqueness.
  bool record_if_new(const std::string& request_id);
  struct Stats {
    size_t size = 0;
    uint64_t attempts = 0;
    uint64_t duplicates = 0;
    uint64_t pruned = 0;
  };
  Stats stats() const;

 private:
  size_t prune();

  std::chrono::milliseconds ttl_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> table_;
  uint64_t attempts_ = 0;
  uint64_t duplicates_ = 0;
  uint64_t pruned_ = 0;
};

}
