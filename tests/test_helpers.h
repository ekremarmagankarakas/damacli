#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <utility>

#include "board.h"
#include "board_state.h"
#include "color.h"
#include "piece_kind.h"
#include "square.h"

namespace dama {

// Builds a Square from algebraic notation ("a3"). Asserts the input is valid;
// tests should only pass literal squares they control.
inline Square Sq(std::string_view algebraic) {
  auto opt = Square::FromAlgebraic(algebraic);
  return *opt;
}

// Builds a BoardState from a side-to-move plus an explicit piece list. Lets
// tests pin down positions directly instead of replaying a move sequence.
inline BoardState MakeState(
    Color side_to_move,
    std::initializer_list<std::pair<Square, PieceKind>> pieces) {
  BoardState s{};
  s.side_to_move = side_to_move;
  s.halfmove_clock = 0;
  for (const auto& [square, kind] : pieces) {
    std::uint64_t bit = 1ULL << (square.row * 8 + square.col);
    switch (kind) {
      case PieceKind::WMan:
        s.w_men_ |= bit;
        break;
      case PieceKind::BMan:
        s.b_men_ |= bit;
        break;
      case PieceKind::WKing:
        s.w_king_ |= bit;
        break;
      case PieceKind::BKing:
        s.b_king_ |= bit;
        break;
      case PieceKind::Empty:
        break;
    }
  }
  return s;
}

inline PieceKind PieceAt(const Board& board, std::string_view algebraic) {
  Square s = Sq(algebraic);
  return board.At(s.row, s.col).value_or(PieceKind::Empty);
}

}  // namespace dama
