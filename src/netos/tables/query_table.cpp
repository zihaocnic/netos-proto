#include "tables/query_table.h"

namespace netos {

QueryTable::QueryTable(std::chrono::milliseconds ttl) : ttl_(ttl) {}

bool QueryTable::record_if_new(const std::string& request_id) {
  ++attempts_;
  pruned_ += prune();
  auto it = table_.find(request_id);
  if (it != table_.end()) {
    ++duplicates_;
    return false;
  }
  table_[request_id] = std::chrono::steady_clock::now();
  return true;
}

QueryTable::Stats QueryTable::stats() const {
  Stats stats;
  stats.size = table_.size();
  stats.attempts = attempts_;
  stats.duplicates = duplicates_;
  stats.pruned = pruned_;
  return stats;
}

size_t QueryTable::prune() {
  auto now = std::chrono::steady_clock::now();
  size_t removed = 0;
  for (auto it = table_.begin(); it != table_.end();) {
    if (now - it->second > ttl_) {
      it = table_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }
  return removed;
}

}
