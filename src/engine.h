#pragma once

#include "move.h"

namespace dama {

class Board;

class Engine {
 public:
  virtual ~Engine() = default;
  virtual Move Choose(Board& board) = 0;
};

}  // namespace dama
