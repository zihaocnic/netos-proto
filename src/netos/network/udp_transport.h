#pragma once

#include <functional>
#include <string>
#include <thread>

#include <netinet/in.h>

namespace netos {

class UdpTransport {
 public:
  using ReceiveHandler = std::function<void(const std::string&, const sockaddr_in&)>;

  UdpTransport(std::string bind_ip, int bind_port);
  ~UdpTransport();

  bool start(ReceiveHandler handler, std::string* error);
  void stop();

  bool send_to(const sockaddr_in& addr, const std::string& payload, std::string* error);

 private:
  void run();

  std::string bind_ip_;
  int bind_port_;
  int fd_;
  bool running_;
  ReceiveHandler handler_;
  std::thread thread_;
};

}
