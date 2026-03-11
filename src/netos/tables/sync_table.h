#pragma once

#include <cstdint>
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

  struct Update {
    bool key_added = false;
    bool destination_added = false;
    bool destination_duplicate = false;
    size_t key_count = 0;
    size_t destination_count = 0;
    size_t evicted = 0;
  };

  struct Stats {
    size_t key_count = 0;
    size_t destination_total = 0;
    uint64_t updates = 0;
    uint64_t duplicate_destinations = 0;
    uint64_t evicted = 0;
  };

  // Record that the given origin requested this key (deduped within the list).
  Update record_destination(const std::string& key, const std::string& destination);
  std::vector<std::string> destinations(const std::string& key) const;
  Stats stats() const;

 private:
  struct Entry {
    std::vector<std::string> destinations;
    std::list<std::string>::iterator lru_it;
  };

  void touch(const std::string& key);
  size_t evict_if_needed();

  size_t capacity_;
  std::list<std::string> lru_;
  std::unordered_map<std::string, Entry> table_;
  size_t destination_total_ = 0;
  uint64_t updates_ = 0;
  uint64_t duplicate_destinations_ = 0;
  uint64_t evicted_total_ = 0;
};

}
