#pragma once

#include <optional>
#include <string>

namespace netos {

enum class MessageType {
  Request,
  Data,
  ContentBloom,
  QueryBloom,
  Unknown
};

struct Message {
  MessageType type = MessageType::Unknown;
  std::string request_id;
  // For both REQ and DATA, origin is the node that initiated the request.
  // DATA responses do not include the responder identity.
  std::string origin;
  // For REQ this is hop TTL. For DATA it is a sanity check only.
  int ttl = 0;
  std::string key;
  std::string value;

  std::string to_wire() const;
  static std::optional<Message> from_wire(const std::string& wire);
};

std::string message_type_label(MessageType type);

}
