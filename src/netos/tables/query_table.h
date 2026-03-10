#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace netos {

// QueryTable tracks recently seen request IDs for TTL-based duplicate suppression.
// It does not store keys/values; it only remembers request IDs for a short window.
class QueryTable {
 public:
  explicit QueryTable(std::chrono::milliseconds ttl);

  // Returns true if the request_id was not seen recently and is now recorded.
  bool record_if_new(const std::string& request_id);

 private:
  void prune();

  std::chrono::milliseconds ttl_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> table_;
};

}
