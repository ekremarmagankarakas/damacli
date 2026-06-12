#pragma once

#include <optional>
#include <string>

class InputSource {
 public:
  virtual ~InputSource() = default;
  virtual std::optional<std::string> ReadLine() = 0;
};
