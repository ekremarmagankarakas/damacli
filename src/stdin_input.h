#pragma once

#include "input_source.h"

namespace dama {

class StdinInput : public InputSource {
 public:
  std::optional<std::string> ReadLine() override;
};

}  // namespace dama
