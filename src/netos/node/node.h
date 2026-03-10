#pragma once

#include "cache/redis_client.h"
#include "core/config.h"
#include "core/message.h"
#include "network/udp_transport.h"
#include "tables/query_table.h"
#include "tables/sync_table.h"

#include <atomic>
#include <string>
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

  void broadcast_request(const Message& msg, const sockaddr_in* exclude);
  bool send_to_neighbor(const NeighborAddress& neighbor, const Message& msg);

  std::string make_request_id();
  void seed_cache();
  void schedule_requests();

  Config config_;
  RedisClient redis_;
  UdpTransport transport_;
  QueryTable query_table_;
  SyncTable sync_table_;
  std::vector<NeighborAddress> neighbors_;
  std::atomic<uint64_t> request_counter_;
  bool running_;
};

}
