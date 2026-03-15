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
                                           std::chrono::milliseconds ttl,
                                           double max_fill_ratio,
                                           size_t max_keys)
    : bit_count_(bit_count),
      hash_count_(hash_count),
      aggregation_window_(aggregation_window),
      ttl_(ttl),
      max_fill_ratio_(max_fill_ratio),
      max_keys_(max_keys) {}

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
  buffer->key_count = 0;
}

QueryBloomAddResult QueryBloomAggregator::add_key(const std::string& origin,
                                                  const std::string& key) {
  QueryBloomAddResult result;
  result.origin = origin;
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
  buffer.key_count += 1;

  result.accepted = true;
  result.created_ms = buffer.created_ms;
  result.batch_id = make_batch_id(origin, buffer.created_ms);
  result.key_count = buffer.key_count;
  result.fill_ratio = buffer.filter.fill_ratio();

  bool split = false;
  if (max_keys_ > 0 && buffer.key_count >= max_keys_) {
    split = true;
    result.split_reason = "max_keys";
  }
  if (max_fill_ratio_ > 0.0 && result.fill_ratio >= max_fill_ratio_) {
    split = true;
    result.split_reason = "fill_ratio";
  }
  if (split) {
    QueryBloomBatch batch;
    batch.origin = origin;
    batch.filter = buffer.filter;
    batch.created_ms = buffer.created_ms;
    batch.batch_id = result.batch_id;
    batch.key_count = buffer.key_count;
    pending_batches_.push_back(std::move(batch));
    reset_buffer(&buffer, now);
    result.split = true;
  }

  return result;
}

std::vector<QueryBloomBatch> QueryBloomAggregator::flush_due() {
  auto now = std::chrono::steady_clock::now();
  std::vector<QueryBloomBatch> batches;
  std::lock_guard<std::mutex> lock(mutex_);
  if (!pending_batches_.empty()) {
    batches.reserve(pending_batches_.size() + buffers_.size());
    for (auto& batch : pending_batches_) {
      batches.push_back(std::move(batch));
    }
    pending_batches_.clear();
  }
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
    batch.key_count = buffer.key_count;
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
