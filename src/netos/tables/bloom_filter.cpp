#include "tables/bloom_filter.h"

#include <algorithm>
#include <cstring>

namespace netos {
namespace {

uint64_t fnv1a64(const std::string& value) {
  uint64_t hash = 1469598103934665603ULL;
  for (unsigned char c : value) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

uint64_t mix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

int hex_value(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return 10 + (c - 'a');
  }
  if (c >= 'A' && c <= 'F') {
    return 10 + (c - 'A');
  }
  return -1;
}

}  // namespace

BloomFilter::BloomFilter(size_t bit_count, size_t hash_count)
    : bit_count_(bit_count), hash_count_(hash_count) {
  size_t bytes = (bit_count_ + 7) / 8;
  data_.assign(bytes, 0);
}

void BloomFilter::clear() {
  std::fill(data_.begin(), data_.end(), 0);
}

void BloomFilter::set_bit(size_t idx) {
  if (bit_count_ == 0) {
    return;
  }
  size_t byte_index = idx / 8;
  uint8_t mask = static_cast<uint8_t>(1u << (idx % 8));
  data_[byte_index] |= mask;
}

bool BloomFilter::get_bit(size_t idx) const {
  if (bit_count_ == 0) {
    return false;
  }
  size_t byte_index = idx / 8;
  uint8_t mask = static_cast<uint8_t>(1u << (idx % 8));
  return (data_[byte_index] & mask) != 0;
}

void BloomFilter::add(const std::string& value) {
  if (bit_count_ == 0 || hash_count_ == 0) {
    return;
  }
  uint64_t h1 = fnv1a64(value);
  uint64_t h2 = mix64(h1);
  for (size_t i = 0; i < hash_count_; ++i) {
    uint64_t combined = h1 + i * h2;
    size_t idx = static_cast<size_t>(combined % bit_count_);
    set_bit(idx);
  }
}

bool BloomFilter::maybe_contains(const std::string& value) const {
  if (bit_count_ == 0 || hash_count_ == 0) {
    return false;
  }
  uint64_t h1 = fnv1a64(value);
  uint64_t h2 = mix64(h1);
  for (size_t i = 0; i < hash_count_; ++i) {
    uint64_t combined = h1 + i * h2;
    size_t idx = static_cast<size_t>(combined % bit_count_);
    if (!get_bit(idx)) {
      return false;
    }
  }
  return true;
}

void BloomFilter::merge(const BloomFilter& other) {
  if (bit_count_ != other.bit_count_ || hash_count_ != other.hash_count_) {
    return;
  }
  if (data_.size() != other.data_.size()) {
    return;
  }
  for (size_t i = 0; i < data_.size(); ++i) {
    data_[i] |= other.data_[i];
  }
}

std::string BloomFilter::to_hex() const {
  static const char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(data_.size() * 2);
  for (uint8_t byte : data_) {
    out.push_back(kHex[(byte >> 4) & 0x0f]);
    out.push_back(kHex[byte & 0x0f]);
  }
  return out;
}

std::optional<BloomFilter> BloomFilter::from_hex(size_t bit_count, size_t hash_count,
                                                 const std::string& hex) {
  size_t expected_bytes = (bit_count + 7) / 8;
  if (hex.size() != expected_bytes * 2) {
    return std::nullopt;
  }
  BloomFilter filter(bit_count, hash_count);
  for (size_t i = 0; i < expected_bytes; ++i) {
    int hi = hex_value(hex[i * 2]);
    int lo = hex_value(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) {
      return std::nullopt;
    }
    filter.data_[i] = static_cast<uint8_t>((hi << 4) | lo);
  }
  return filter;
}

uint64_t BloomFilter::digest() const {
  uint64_t hash = 1469598103934665603ULL;
  for (uint8_t byte : data_) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
}

}
