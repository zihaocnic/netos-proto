#include "tables/sync_table.h"

#include <algorithm>

namespace netos {

SyncTable::SyncTable(size_t capacity) : capacity_(capacity) {}

SyncTable::Update SyncTable::record_destination(const std::string& key, const std::string& destination) {
  Update update;
  auto it = table_.find(key);
  if (it == table_.end()) {
    lru_.push_front(key);
    Entry entry;
    entry.destinations.push_back(destination);
    entry.lru_it = lru_.begin();
    table_.emplace(key, std::move(entry));
    update.key_added = true;
    update.destination_added = true;
    it = table_.find(key);
  } else {
    auto& dests = it->second.destinations;
    if (std::find(dests.begin(), dests.end(), destination) == dests.end()) {
      dests.push_back(destination);
      update.destination_added = true;
    } else {
      update.destination_duplicate = true;
    }
    touch(key);
  }
  if (it != table_.end()) {
    update.destination_count = it->second.destinations.size();
  }
  update.key_count = table_.size();
  update.evicted = evict_if_needed();
  update.key_count = table_.size();
  return update;
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

size_t SyncTable::evict_if_needed() {
  size_t evicted = 0;
  while (table_.size() > capacity_ && !lru_.empty()) {
    auto last = lru_.back();
    lru_.pop_back();
    table_.erase(last);
    ++evicted;
  }
  return evicted;
}

}
