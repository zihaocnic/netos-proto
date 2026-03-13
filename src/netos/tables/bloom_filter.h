#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace netos {

class BloomFilter {
 public:
  BloomFilter() = default;
  BloomFilter(size_t bit_count, size_t hash_count);

  size_t bit_count() const { return bit_count_; }
  size_t hash_count() const { return hash_count_; }
  size_t byte_count() const { return data_.size(); }

  void clear();
  void add(const std::string& value);
  bool maybe_contains(const std::string& value) const;

  void merge(const BloomFilter& other);

  std::string to_hex() const;
  static std::optional<BloomFilter> from_hex(size_t bit_count, size_t hash_count,
                                             const std::string& hex);

  uint64_t digest() const;

 private:
  void set_bit(size_t idx);
  bool get_bit(size_t idx) const;

  size_t bit_count_ = 0;
  size_t hash_count_ = 0;
  std::vector<uint8_t> data_;
};

}
