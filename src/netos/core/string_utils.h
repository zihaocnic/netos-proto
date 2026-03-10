#pragma once

#include <string>
#include <vector>

namespace netos {

std::vector<std::string> split(const std::string& input, char delim, bool skip_empty);
std::string trim(const std::string& input);

}
