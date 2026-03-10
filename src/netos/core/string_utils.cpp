#include "core/string_utils.h"

#include <cctype>

namespace netos {

std::vector<std::string> split(const std::string& input, char delim, bool skip_empty) {
  std::vector<std::string> parts;
  std::string current;
  for (char ch : input) {
    if (ch == delim) {
      if (!current.empty() || !skip_empty) {
        parts.push_back(current);
      }
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty() || !skip_empty) {
    parts.push_back(current);
  }
  return parts;
}

std::string trim(const std::string& input) {
  size_t start = 0;
  while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
    ++start;
  }
  if (start == input.size()) {
    return "";
  }
  size_t end = input.size() - 1;
  while (end > start && std::isspace(static_cast<unsigned char>(input[end]))) {
    --end;
  }
  return input.substr(start, end - start + 1);
}

}
