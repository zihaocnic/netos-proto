#include "core/message.h"

#include "core/string_utils.h"

#include <cstdlib>

namespace netos {

namespace {
MessageType parse_type(const std::string& token) {
  if (token == "REQ") {
    return MessageType::Request;
  }
  if (token == "DATA") {
    return MessageType::Data;
  }
  return MessageType::Unknown;
}

int parse_int(const std::string& token, int fallback) {
  if (token.empty()) {
    return fallback;
  }
  char* end = nullptr;
  long value = std::strtol(token.c_str(), &end, 10);
  if (end == token.c_str() || *end != '\0') {
    return fallback;
  }
  return static_cast<int>(value);
}
}  // namespace

std::string message_type_label(MessageType type) {
  switch (type) {
    case MessageType::Request:
      return "REQ";
    case MessageType::Data:
      return "DATA";
    case MessageType::Unknown:
      return "UNKNOWN";
  }
  return "UNKNOWN";
}

std::string Message::to_wire() const {
  std::string out;
  out.reserve(64 + key.size() + value.size());
  out.append(message_type_label(type));
  out.push_back('|');
  out.append(request_id);
  out.push_back('|');
  out.append(origin);
  out.push_back('|');
  out.append(std::to_string(ttl));
  out.push_back('|');
  out.append(key);
  out.push_back('|');
  out.append(value);
  return out;
}

std::optional<Message> Message::from_wire(const std::string& wire) {
  auto parts = split(wire, '|', false);
  if (parts.size() < 5) {
    return std::nullopt;
  }
  Message msg;
  msg.type = parse_type(parts[0]);
  msg.request_id = parts[1];
  msg.origin = parts[2];
  msg.ttl = parse_int(parts[3], 0);
  msg.key = parts[4];
  if (parts.size() >= 6) {
    msg.value = parts[5];
  }
  return msg;
}

}
