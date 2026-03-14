#include "tables/content_bf_fallback.h"

namespace netos {

ContentBfFallbackTable::ContentBfFallbackTable(std::chrono::milliseconds window) : window_(window) {}

std::string ContentBfFallbackTable::make_key(const std::string& origin,
                                             const std::string& key) const {
  return origin + "|" + key;
}

bool ContentBfFallbackTable::schedule(const std::string& key, const std::string& origin) {
  if (window_.count() <= 0) {
    return false;
  }
  Entry entry;
  entry.key = key;
  entry.origin = origin;
  entry.deadline = std::chrono::steady_clock::now() + window_;
  std::lock_guard<std::mutex> lock(mutex_);
  entries_[make_key(origin, key)] = entry;
  return true;
}

bool ContentBfFallbackTable::cancel(const std::string& key, const std::string& origin) {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_.erase(make_key(origin, key)) > 0;
}

std::vector<ContentBfFallbackTable::Pending> ContentBfFallbackTable::take_due() {
  if (window_.count() <= 0) {
    return {};
  }
  auto now = std::chrono::steady_clock::now();
  std::vector<Pending> due;
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = entries_.begin(); it != entries_.end();) {
    if (now >= it->second.deadline) {
      due.push_back({it->second.key, it->second.origin});
      it = entries_.erase(it);
    } else {
      ++it;
    }
  }
  return due;
}

size_t ContentBfFallbackTable::pending_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_.size();
}

}
