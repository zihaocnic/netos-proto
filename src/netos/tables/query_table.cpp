#include "tables/query_table.h"

namespace netos {

QueryTable::QueryTable(std::chrono::milliseconds ttl) : ttl_(ttl) {}

bool QueryTable::record_if_new(const std::string& request_id) {
  prune();
  auto it = table_.find(request_id);
  if (it != table_.end()) {
    return false;
  }
  table_[request_id] = std::chrono::steady_clock::now();
  return true;
}

void QueryTable::prune() {
  auto now = std::chrono::steady_clock::now();
  for (auto it = table_.begin(); it != table_.end();) {
    if (now - it->second > ttl_) {
      it = table_.erase(it);
    } else {
      ++it;
    }
  }
}

}
