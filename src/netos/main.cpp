#include "core/config.h"
#include "core/logger.h"
#include "node/node.h"

int main() {
  auto config = netos::load_config();
  if (!config.log_level.empty()) {
    netos::Logger::instance().set_level(netos::parse_log_level(config.log_level));
  }

  netos::Node node(std::move(config));
  std::string error;
  if (!node.init(&error)) {
    netos::log_error("node init failed: " + error);
    return 1;
  }

  node.run();
  return 0;
}
