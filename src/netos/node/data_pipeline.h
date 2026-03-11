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

class DataPipeline {
 public:
  explicit DataPipeline(const Config& config);

  DataDecision run(const Message& msg) const;

 private:
  bool validate_fields(const Message& msg, DataDecision* decision) const;
  bool check_origin(const Message& msg, DataDecision* decision) const;

  const Config& config_;
};

DataDecision run_data_pipeline(const Config& config, const Message& msg);

}
