#pragma once

#include <array>
#include <cstdint>

#include "color.h"

struct BoardState {
  std::uint64_t w_men_;
  std::uint64_t b_men_;
  std::uint64_t w_king_;
  std::uint64_t b_king_;
  Color side_to_move;
  int halfmove_clock;
};

// Position equality for threefold repetition.
// halfmove_clock intentionally excluded: same position with different clock
// still counts as repetition.
inline bool operator==(const BoardState& a, const BoardState& b) {
  return a.w_men_ == b.w_men_ && a.b_men_ == b.b_men_ &&
         a.w_king_ == b.w_king_ && a.b_king_ == b.b_king_ &&
         a.side_to_move == b.side_to_move;
}
