#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace netos {

// SyncTable remembers which origins have requested a key that this node served.
// It is a bounded LRU stub for future push/subscription logic (key -> origin list).
class SyncTable {
 public:
  explicit SyncTable(size_t capacity);

  // Record that the given origin requested this key (deduped within the list).
  void record_destination(const std::string& key, const std::string& destination);
  std::vector<std::string> destinations(const std::string& key) const;

 private:
  struct Entry {
    std::vector<std::string> destinations;
    std::list<std::string>::iterator lru_it;
  };

  void touch(const std::string& key);
  void evict_if_needed();

  size_t capacity_;
  std::list<std::string> lru_;
  std::unordered_map<std::string, Entry> table_;
};

}
