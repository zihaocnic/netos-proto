#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace netos {

class SyncTable {
 public:
  explicit SyncTable(size_t capacity);

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
