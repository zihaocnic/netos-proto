#include "node/forward_queue.h"

#include "core/logger.h"

#include <utility>

namespace netos {

ForwardQueue::ForwardQueue(UdpNetwork* network, ForwardQueueConfig config)
    : network_(network), config_(std::move(config)) {}

void ForwardQueue::start(std::atomic<bool>* running_flag) {
  running_ = running_flag;
  if (!config_.enabled) {
    started_.store(true);
    return;
  }
  started_.store(true);
  int workers = config_.worker_count;
  if (workers <= 0) {
    workers = 1;
  }
  for (int i = 0; i < workers; ++i) {
    std::thread([this]() { worker_loop(); }).detach();
  }
}

bool ForwardQueue::send_direct(const sockaddr_in& addr,
                               const Message& msg,
                               std::function<void()> on_success,
                               std::function<void(const std::string&)> on_failure) {
  Item item;
  item.kind = Kind::Direct;
  item.msg = msg;
  item.addr = addr;
  item.has_addr = true;
  item.on_success = std::move(on_success);
  item.on_failure = std::move(on_failure);

  if (!config_.enabled || !started_.load()) {
    std::string error;
    bool ok = network_->send_direct(addr, msg, &error);
    if (!ok) {
      if (item.on_failure) {
        item.on_failure(error);
      }
    } else if (item.on_success) {
      item.on_success();
    }
    log_state("sent", 0);
    return ok;
  }

  return enqueue(std::move(item));
}

bool ForwardQueue::send_broadcast(const Message& msg, const sockaddr_in* exclude) {
  Item item;
  item.kind = Kind::Broadcast;
  item.msg = msg;
  if (exclude) {
    item.has_exclude = true;
    item.exclude = *exclude;
  }

  if (!config_.enabled || !started_.load()) {
    network_->send_broadcast(msg, exclude);
    log_state("sent", 0);
    return true;
  }

  return enqueue(std::move(item));
}

bool ForwardQueue::enqueue(Item item) {
  size_t depth = 0;
  bool dropped = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= config_.max_depth) {
      dropped = true;
      depth = queue_.size();
    } else {
      queue_.push_back(std::move(item));
      depth = queue_.size();
    }
  }

  if (dropped) {
    log_state("drop_queue", depth);
    if (item.on_failure) {
      item.on_failure("forward_queue_full");
    }
    return false;
  }

  log_state("queued", depth);
  cv_.notify_one();
  return true;
}

void ForwardQueue::worker_loop() {
  for (;;) {
    Item item;
    size_t depth = 0;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]() { return !queue_.empty() || !should_run(); });
      if (queue_.empty()) {
        if (!should_run()) {
          return;
        }
        continue;
      }
      item = std::move(queue_.front());
      queue_.pop_front();
      depth = queue_.size();
    }

    if (item.kind == Kind::Direct) {
      std::string error;
      if (!network_->send_direct(item.addr, item.msg, &error)) {
        if (item.on_failure) {
          item.on_failure(error);
        }
      } else if (item.on_success) {
        item.on_success();
      }
    } else {
      const sockaddr_in* exclude = item.has_exclude ? &item.exclude : nullptr;
      network_->send_broadcast(item.msg, exclude);
    }

    log_state("sent", depth);
  }
}

void ForwardQueue::log_state(const std::string& state, size_t depth) const {
  log_info("forward_state=" + state + " queue_depth=" + std::to_string(depth));
}

bool ForwardQueue::should_run() const {
  if (!running_) {
    return true;
  }
  return running_->load();
}

}
