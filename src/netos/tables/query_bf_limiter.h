#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace netos {

class QueryBloomLimiter {
 public:
  QueryBloomLimiter(size_t attempt_limit, std::chrono::milliseconds window);

  bool allow(const std::string& origin, uint64_t digest);
  size_t entry_count() const;

 private:
  struct Entry {
    std::chrono::steady_clock::time_point window_start{};
    size_t attempts = 0;
  };

  void prune_locked(std::chrono::steady_clock::time_point now);
  std::string make_key(const std::string& origin, uint64_t digest) const;

  size_t attempt_limit_;
  std::chrono::milliseconds window_;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, Entry> entries_;
};

}
