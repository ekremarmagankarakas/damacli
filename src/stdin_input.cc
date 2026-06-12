#include "stdin_input.h"

#include <iostream>

std::optional<std::string> StdinInput::ReadLine() {
  std::string input;
  if (!std::getline(std::cin, input)) {
    return std::nullopt;
  }
  return input;
}
