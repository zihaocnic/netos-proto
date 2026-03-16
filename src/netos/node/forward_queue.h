#pragma once

#include "core/message.h"
#include "network/udp_network.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace netos {

struct ForwardQueueConfig {
  bool enabled = true;
  int worker_count = 1;
  size_t max_depth = 1024;
  std::string drop_policy = "drop_newest";
};

class ForwardQueue {
 public:
  ForwardQueue(UdpNetwork* network, ForwardQueueConfig config);

  void start(std::atomic<bool>* running_flag);

  bool send_direct(const sockaddr_in& addr,
                   const Message& msg,
                   std::function<void()> on_success,
                   std::function<void(const std::string&)> on_failure);

  bool send_broadcast(const Message& msg, const sockaddr_in* exclude);

 private:
  enum class Kind {
    Direct,
    Broadcast
  };

  struct Item {
    Kind kind = Kind::Direct;
    Message msg;
    sockaddr_in addr{};
    bool has_addr = false;
    sockaddr_in exclude{};
    bool has_exclude = false;
    std::function<void()> on_success;
    std::function<void(const std::string&)> on_failure;
  };

  bool enqueue(Item item);
  void worker_loop();
  void log_state(const std::string& state, size_t depth) const;
  bool should_run() const;

  UdpNetwork* network_;
  ForwardQueueConfig config_;
  std::atomic<bool>* running_ = nullptr;
  std::atomic<bool> started_ = false;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<Item> queue_;
};

}
