#pragma once

#include "engine.h"

class MinimaxEngine final : public Engine {
 public:
  explicit MinimaxEngine(int depth);
  Move Choose(Board& board) override;

 private:
  int depth_;

  int Search(Board& board, int depth, int alpha, int beta);
  int Evaluate(const Board& board) const;
};
