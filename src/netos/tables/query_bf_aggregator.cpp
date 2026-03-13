#include "tables/query_bf_aggregator.h"

#include <chrono>

namespace netos {
namespace {

uint64_t now_millis() {
  auto now = std::chrono::system_clock::now();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

std::string make_batch_id(const std::string& origin, uint64_t created_ms) {
  return "qbf-" + origin + "-" + std::to_string(created_ms);
}

}  // namespace

QueryBloomAggregator::QueryBloomAggregator(size_t bit_count, size_t hash_count,
                                           std::chrono::milliseconds aggregation_window,
                                           std::chrono::milliseconds ttl)
    : bit_count_(bit_count),
      hash_count_(hash_count),
      aggregation_window_(aggregation_window),
      ttl_(ttl) {}

QueryBloomAggregator::Buffer QueryBloomAggregator::make_buffer() const {
  Buffer buffer;
  buffer.filter = BloomFilter(bit_count_, hash_count_);
  return buffer;
}

void QueryBloomAggregator::reset_buffer(Buffer* buffer,
                                        std::chrono::steady_clock::time_point now) const {
  if (!buffer) {
    return;
  }
  buffer->filter = BloomFilter(bit_count_, hash_count_);
  buffer->created_at = now;
  buffer->created_ms = now_millis();
  buffer->has_data = false;
}

bool QueryBloomAggregator::add_key(const std::string& origin, const std::string& key) {
  auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  auto& buffer = buffers_[origin];
  if (buffer.created_ms == 0) {
    reset_buffer(&buffer, now);
  }
  if (ttl_.count() > 0 && now - buffer.created_at > ttl_) {
    reset_buffer(&buffer, now);
  }
  buffer.filter.add(key);
  buffer.has_data = true;
  return true;
}

std::vector<QueryBloomBatch> QueryBloomAggregator::flush_due() {
  auto now = std::chrono::steady_clock::now();
  std::vector<QueryBloomBatch> batches;
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& kv : buffers_) {
    auto& buffer = kv.second;
    if (!buffer.has_data) {
      continue;
    }
    if (aggregation_window_.count() > 0 && now - buffer.created_at < aggregation_window_) {
      continue;
    }
    QueryBloomBatch batch;
    batch.origin = kv.first;
    batch.filter = buffer.filter;
    batch.created_ms = buffer.created_ms;
    batch.batch_id = make_batch_id(batch.origin, batch.created_ms);
    batches.push_back(std::move(batch));
    reset_buffer(&buffer, now);
  }
  return batches;
}

size_t QueryBloomAggregator::pending_origins() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return buffers_.size();
}

}
