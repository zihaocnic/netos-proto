#include "tables/sync_table.h"

#include <algorithm>

namespace netos {

SyncTable::SyncTable(size_t capacity) : capacity_(capacity) {}

void SyncTable::record_destination(const std::string& key, const std::string& destination) {
  auto it = table_.find(key);
  if (it == table_.end()) {
    lru_.push_front(key);
    Entry entry;
    entry.destinations.push_back(destination);
    entry.lru_it = lru_.begin();
    table_.emplace(key, std::move(entry));
  } else {
    auto& dests = it->second.destinations;
    if (std::find(dests.begin(), dests.end(), destination) == dests.end()) {
      dests.push_back(destination);
    }
    touch(key);
  }
  evict_if_needed();
}

std::vector<std::string> SyncTable::destinations(const std::string& key) const {
  auto it = table_.find(key);
  if (it == table_.end()) {
    return {};
  }
  return it->second.destinations;
}

void SyncTable::touch(const std::string& key) {
  auto it = table_.find(key);
  if (it == table_.end()) {
    return;
  }
  lru_.erase(it->second.lru_it);
  lru_.push_front(key);
  it->second.lru_it = lru_.begin();
}

void SyncTable::evict_if_needed() {
  while (table_.size() > capacity_ && !lru_.empty()) {
    auto last = lru_.back();
    lru_.pop_back();
    table_.erase(last);
  }
}

}
