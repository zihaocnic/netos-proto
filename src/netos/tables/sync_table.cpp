#include "tables/sync_table.h"

#include <algorithm>

namespace netos {

SyncTable::SyncTable(size_t capacity) : capacity_(capacity) {}

SyncTable::Update SyncTable::record_destination(const std::string& key, const std::string& destination) {
  Update update;
  ++updates_;
  auto it = table_.find(key);
  if (it == table_.end()) {
    lru_.push_front(key);
    Entry entry;
    entry.destinations.push_back(destination);
    entry.lru_it = lru_.begin();
    table_.emplace(key, std::move(entry));
    update.key_added = true;
    update.destination_added = true;
    ++destination_total_;
    it = table_.find(key);
  } else {
    auto& dests = it->second.destinations;
    if (std::find(dests.begin(), dests.end(), destination) == dests.end()) {
      dests.push_back(destination);
      update.destination_added = true;
      ++destination_total_;
    } else {
      update.destination_duplicate = true;
      ++duplicate_destinations_;
    }
    touch(key);
  }
  if (it != table_.end()) {
    update.destination_count = it->second.destinations.size();
  }
  update.key_count = table_.size();
  update.evicted = evict_if_needed();
  if (update.evicted > 0) {
    evicted_total_ += update.evicted;
  }
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

SyncTable::Stats SyncTable::stats() const {
  Stats stats;
  stats.key_count = table_.size();
  stats.destination_total = destination_total_;
  stats.updates = updates_;
  stats.duplicate_destinations = duplicate_destinations_;
  stats.evicted = evicted_total_;
  return stats;
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
    auto it = table_.find(last);
    if (it != table_.end()) {
      size_t removed = it->second.destinations.size();
      if (destination_total_ >= removed) {
        destination_total_ -= removed;
      } else {
        destination_total_ = 0;
      }
      table_.erase(it);
    }
    ++evicted;
  }
  return evicted;
}

}
