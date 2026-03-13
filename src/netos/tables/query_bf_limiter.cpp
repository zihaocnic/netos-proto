#include "tables/query_bf_limiter.h"

namespace netos {

QueryBloomLimiter::QueryBloomLimiter(size_t attempt_limit, std::chrono::milliseconds window)
    : attempt_limit_(attempt_limit), window_(window) {}

std::string QueryBloomLimiter::make_key(const std::string& origin, uint64_t digest) const {
  return origin + ":" + std::to_string(digest);
}

bool QueryBloomLimiter::allow(const std::string& origin, uint64_t digest) {
  if (attempt_limit_ == 0 || window_.count() <= 0) {
    return true;
  }
  auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  prune_locked(now);
  auto key = make_key(origin, digest);
  auto& entry = entries_[key];
  if (entry.window_start.time_since_epoch().count() == 0) {
    entry.window_start = now;
  }
  if (now - entry.window_start > window_) {
    entry.window_start = now;
    entry.attempts = 0;
  }
  if (entry.attempts >= attempt_limit_) {
    return false;
  }
  ++entry.attempts;
  return true;
}

size_t QueryBloomLimiter::entry_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_.size();
}

void QueryBloomLimiter::prune_locked(std::chrono::steady_clock::time_point now) {
  if (window_.count() <= 0) {
    return;
  }
  for (auto it = entries_.begin(); it != entries_.end();) {
    if (now - it->second.window_start > window_ * 2) {
      it = entries_.erase(it);
    } else {
      ++it;
    }
  }
}

}
