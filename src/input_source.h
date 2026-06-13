#pragma once

#include <optional>
#include <string>

namespace dama {

class InputSource {
 public:
  virtual ~InputSource() = default;
  virtual std::optional<std::string> ReadLine() = 0;
};

}  // namespace dama
