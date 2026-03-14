#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace netos {

class ContentBfFallbackTable {
 public:
  explicit ContentBfFallbackTable(std::chrono::milliseconds window);

  struct Pending {
    std::string key;
    std::string origin;
  };

  bool schedule(const std::string& key, const std::string& origin);
  bool cancel(const std::string& key, const std::string& origin);
  std::vector<Pending> take_due();
  size_t pending_count() const;

 private:
  struct Entry {
    std::string key;
    std::string origin;
    std::chrono::steady_clock::time_point deadline{};
  };

  std::string make_key(const std::string& origin, const std::string& key) const;

  std::chrono::milliseconds window_;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, Entry> entries_;
};

}
