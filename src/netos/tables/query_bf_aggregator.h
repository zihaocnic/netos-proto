#pragma once

#include "tables/bloom_filter.h"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace netos {

struct QueryBloomBatch {
  std::string origin;
  BloomFilter filter;
  uint64_t created_ms = 0;
  std::string batch_id;
  size_t key_count = 0;
};

struct QueryBloomAddResult {
  bool accepted = false;
  bool split = false;
  std::string split_reason;
  std::string origin;
  std::string batch_id;
  uint64_t created_ms = 0;
  size_t key_count = 0;
  double fill_ratio = 0.0;
};

class QueryBloomAggregator {
 public:
  QueryBloomAggregator(size_t bit_count, size_t hash_count,
                       std::chrono::milliseconds aggregation_window,
                       std::chrono::milliseconds ttl,
                       double max_fill_ratio,
                       size_t max_keys);

  QueryBloomAddResult add_key(const std::string& origin, const std::string& key);
  std::vector<QueryBloomBatch> flush_due();

  size_t pending_origins() const;

 private:
  struct Buffer {
    BloomFilter filter;
    std::chrono::steady_clock::time_point created_at{};
    uint64_t created_ms = 0;
    bool has_data = false;
    size_t key_count = 0;
  };

  Buffer make_buffer() const;
  void reset_buffer(Buffer* buffer, std::chrono::steady_clock::time_point now) const;

  size_t bit_count_;
  size_t hash_count_;
  std::chrono::milliseconds aggregation_window_;
  std::chrono::milliseconds ttl_;
  double max_fill_ratio_;
  size_t max_keys_;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, Buffer> buffers_;
  std::vector<QueryBloomBatch> pending_batches_;
};

}
