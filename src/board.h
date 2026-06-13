#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "board_state.h"
#include "color.h"
#include "game_result.h"
#include "history_entry.h"
#include "move.h"
#include "piece_kind.h"
#include "square.h"

namespace dama {

class Board {
 public:
  static constexpr int kSize = 8;

  Board();
  Board(const Board&) = delete;
  Board& operator=(const Board&) = delete;
  Board(Board&&) = default;
  Board& operator=(Board&&) = default;

  void Reset();

  void Apply(const Move& move);
  bool IsLegal(const Move& move) const;

  const std::vector<HistoryEntry>& History() const { return history_; }
  Color SideToMove() const { return side_to_move_; }

  std::optional<PieceKind> At(int row, int col) const;

  static bool InBounds(const Square& square);
  static bool InBounds(int row, int col);

  std::vector<Move> LegalMoves() const;
  std::optional<GameResult> Result() const;

  GameResult HandleResign() const;

  void Undo();
  BoardState Snapshot() const;
  void Restore(const BoardState& board_state);

 private:
  std::uint64_t w_men_;
  std::uint64_t b_men_;
  std::uint64_t w_king_;
  std::uint64_t b_king_;

  Color side_to_move_ = Color::kWhite;
  int halfmove_clock_ = 0;
  std::vector<HistoryEntry> history_;

  void Setup();
  void ApplyNoHistory(const Move& move);
};

}  // namespace dama
