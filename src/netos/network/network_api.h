#pragma once

#include "core/message.h"

#include <functional>
#include <netinet/in.h>
#include <string>

namespace netos {

class NetworkApi {
 public:
  using ReceiveHandler = std::function<void(const std::string&, const sockaddr_in&)>;

  virtual ~NetworkApi() = default;

  virtual bool start(ReceiveHandler handler, std::string* error) = 0;
  virtual void stop() = 0;

  virtual bool send_direct(const sockaddr_in& addr, const Message& msg, std::string* error) = 0;
  virtual void send_broadcast(const Message& msg, const sockaddr_in* exclude) = 0;
};

}
