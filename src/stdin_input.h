#pragma once

#include "input_source.h"

class StdinInput : public InputSource {
 public:
  std::optional<std::string> ReadLine() override;
};
