#pragma once

#include "move.h"

class Board;

class Engine {
 public:
  virtual ~Engine() = default;
  virtual Move Choose(Board& board) = 0;
};
