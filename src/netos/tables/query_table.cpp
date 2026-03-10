#include "tables/query_table.h"

namespace netos {

QueryTable::QueryTable(std::chrono::milliseconds ttl) : ttl_(ttl) {}

bool QueryTable::seen(const std::string& request_id) {
  prune();
  return table_.find(request_id) != table_.end();
}

void QueryTable::record(const std::string& request_id) {
  prune();
  table_[request_id] = std::chrono::steady_clock::now();
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
