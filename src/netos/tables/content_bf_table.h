#pragma once

#include "network/address.h"
#include "tables/bloom_filter.h"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace netos {

class ContentBloomTable {
 public:
  ContentBloomTable(size_t bit_count, size_t hash_count, std::chrono::milliseconds ttl);

  void add_local_key(const std::string& key);
  BloomFilter local_snapshot() const;

  void update_neighbor(const sockaddr_in& addr, const BloomFilter& filter);
  std::optional<sockaddr_in> select_neighbor(const std::string& key);

  size_t neighbor_count() const;

 private:
  struct NeighborEntry {
    BloomFilter filter;
    std::chrono::steady_clock::time_point updated;
    sockaddr_in addr{};
  };

  void prune_locked(std::chrono::steady_clock::time_point now);

  BloomFilter local_filter_;
  std::chrono::milliseconds ttl_;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, NeighborEntry> neighbors_;
};

}
