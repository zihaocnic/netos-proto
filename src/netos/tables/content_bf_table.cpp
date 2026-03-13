#include "tables/content_bf_table.h"

namespace netos {

ContentBloomTable::ContentBloomTable(size_t bit_count, size_t hash_count,
                                     std::chrono::milliseconds ttl)
    : local_filter_(bit_count, hash_count), ttl_(ttl) {}

void ContentBloomTable::add_local_key(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  local_filter_.add(key);
}

BloomFilter ContentBloomTable::local_snapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return local_filter_;
}

void ContentBloomTable::update_neighbor(const sockaddr_in& addr, const BloomFilter& filter) {
  std::lock_guard<std::mutex> lock(mutex_);
  NeighborEntry entry;
  entry.filter = filter;
  entry.updated = std::chrono::steady_clock::now();
  entry.addr = addr;
  neighbors_[addr_to_string(addr)] = entry;
}

std::optional<sockaddr_in> ContentBloomTable::select_neighbor(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::steady_clock::now();
  prune_locked(now);
  for (const auto& kv : neighbors_) {
    if (kv.second.filter.maybe_contains(key)) {
      return kv.second.addr;
    }
  }
  return std::nullopt;
}

size_t ContentBloomTable::neighbor_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return neighbors_.size();
}

void ContentBloomTable::prune_locked(std::chrono::steady_clock::time_point now) {
  if (ttl_.count() <= 0) {
    return;
  }
  for (auto it = neighbors_.begin(); it != neighbors_.end();) {
    if (now - it->second.updated > ttl_) {
      it = neighbors_.erase(it);
    } else {
      ++it;
    }
  }
}

}
