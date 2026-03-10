#pragma once

#include <optional>
#include <string>

namespace netos {

enum class MessageType {
  Request,
  Data,
  Unknown
};

struct Message {
  MessageType type = MessageType::Unknown;
  std::string request_id;
  std::string origin;
  int ttl = 0;
  std::string key;
  std::string value;

  std::string to_wire() const;
  static std::optional<Message> from_wire(const std::string& wire);
};

std::string message_type_label(MessageType type);

}
