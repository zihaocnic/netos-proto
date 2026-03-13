#pragma once

#include "cache/redis_client.h"
#include "core/config.h"
#include "core/message.h"
#include "network/address.h"
#include "network/udp_network.h"
#include "tables/bloom_filter.h"
#include "tables/content_bf_table.h"
#include "tables/query_bf_aggregator.h"
#include "tables/query_bf_limiter.h"
#include "tables/query_table.h"
#include "tables/sync_table.h"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace netos {

class Node {
 public:
  explicit Node(Config config);

  bool init(std::string* error);
  void run();

 private:
  void handle_wire_message(const std::string& wire, const sockaddr_in& from);
  void handle_request(const Message& msg, const sockaddr_in& from);
  void handle_data(const Message& msg, const sockaddr_in& from);
  void handle_content_bloom(const Message& msg, const sockaddr_in& from);
  void handle_query_bloom(const Message& msg, const sockaddr_in& from);

  std::string make_request_id();
  void seed_cache();
  void schedule_requests();
  void start_background_tasks();
  void start_content_bloom_exchange();
  void start_query_bloom_flush();
  void record_local_key(const std::string& key);

  Config config_;
  RedisClient redis_;
  UdpNetwork network_;
  QueryTable query_table_;
  SyncTable sync_table_;
  ContentBloomTable content_bloom_;
  QueryBloomAggregator query_bloom_aggregator_;
  QueryBloomLimiter query_bloom_limiter_;
  std::unordered_set<std::string> local_keys_;
  std::mutex local_keys_mutex_;
  std::atomic<uint64_t> request_counter_;
  bool running_;
};

}
