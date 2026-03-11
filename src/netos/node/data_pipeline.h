#pragma once

#include "core/config.h"
#include "core/message.h"

#include <string>

namespace netos {

enum class DataState {
  DropInvalid,
  DropNotOrigin,
  StoreLocal
};

struct DataDecision {
  DataState state = DataState::DropNotOrigin;
  std::string reason;
};

std::string data_state_label(DataState state);

DataDecision run_data_pipeline(const Config& config, const Message& msg);

}
