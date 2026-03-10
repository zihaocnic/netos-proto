#pragma once

#include "core/config.h"

#include <string>

namespace netos {

bool validate_config(const Config& config, std::string* error);
std::string config_summary(const Config& config);

}
