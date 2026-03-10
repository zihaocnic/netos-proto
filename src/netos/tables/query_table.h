#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace netos {

class QueryTable {
 public:
  explicit QueryTable(std::chrono::milliseconds ttl);

  bool seen(const std::string& request_id);
  void record(const std::string& request_id);

 private:
  void prune();

  std::chrono::milliseconds ttl_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> table_;
};

}
