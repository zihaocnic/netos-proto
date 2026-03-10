#include "core/config.h"
#include "core/logger.h"
#include "node/node.h"

#include <cstdlib>

int main() {
  const char* level_env = std::getenv("NETOS_LOG_LEVEL");
  if (level_env) {
    netos::Logger::instance().set_level(netos::parse_log_level(level_env));
  }

  auto config = netos::load_config();

  netos::Node node(std::move(config));
  std::string error;
  if (!node.init(&error)) {
    netos::log_error("node init failed: " + error);
    return 1;
  }

  node.run();
  return 0;
}
