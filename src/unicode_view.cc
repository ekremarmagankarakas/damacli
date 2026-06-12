#include "unicode_view.h"

#include <iostream>
#include <optional>
#include <string>

#include "board.h"
#include "game_result.h"
#include "piece_kind.h"

namespace {

// Background colors for board squares.
const char* BG_LIGHT = "\033[48;5;187m";      // warm tan
const char* BG_DARK = "\033[48;5;94m";        // brown
const char* BG_HIGHLIGHT = "\033[48;5;221m";  // yellow — last-moved squares

// Foreground colors for pieces.
const char* FG_WHITE = "\033[1;38;5;231m";  // bold bright white
const char* FG_BLACK = "\033[38;5;16m";     // deep black

// Coordinate labels.
const char* FG_LABEL = "\033[38;5;245m";  // dim gray

const char* RESET = "\033[0m";

bool IsHighlighted(int row, int col, const std::optional<Move>& last) {
  if (!last) {
    return false;
  }
  if (last->from.row == row && last->from.col == col) return true;
  if (last->to.row == row && last->to.col == col) return true;
  for (const auto& v : last->via) {
    if (v.row == row && v.col == col) return true;
  }
  for (const auto& cap : last->captures) {
    if (cap.row == row && cap.col == col) return true;
  }
  return false;
}

const char* Glyph(PieceKind k) {
  switch (k) {
    case PieceKind::WMan:  return "⛀";  // ⛀
    case PieceKind::WKing: return "⛁";  // ⛁
    case PieceKind::BMan:  return "⛂";  // ⛂
    case PieceKind::BKing: return "⛃";  // ⛃
    case PieceKind::Empty: return " ";
  }
  return " ";
}

const char* FgFor(PieceKind k) {
  switch (k) {
    case PieceKind::WMan:
    case PieceKind::WKing:
      return FG_WHITE;
    case PieceKind::BMan:
    case PieceKind::BKing:
      return FG_BLACK;
    case PieceKind::Empty:
      return "";
  }
  return "";
}

}  // namespace

void UnicodeView::Render(const Board& board, bool game_over) {
  // Clear visible screen + scrollback, move cursor to home.
  std::cout << "\033[2J\033[3J\033[H";

  std::optional<Move> last;
  const auto& history = board.History();
  if (!history.empty()) {
    last = history.back().move;
  }

  for (int row = Board::kSize - 1; row >= 0; --row) {
    std::cout << FG_LABEL << (row + 1) << RESET << ' ';
    for (int col = 0; col < Board::kSize; ++col) {
      bool light = (row + col) % 2 == 1;
      const char* bg = IsHighlighted(row, col, last)
                           ? BG_HIGHLIGHT
                           : (light ? BG_LIGHT : BG_DARK);
      auto pk = board.At(row, col).value_or(PieceKind::Empty);
      std::cout << bg;
      if (pk == PieceKind::Empty) {
        std::cout << "   ";
      } else {
        std::cout << FgFor(pk) << ' ' << Glyph(pk) << ' ';
      }
      std::cout << RESET;
    }
    std::cout << '\n';
  }
  std::cout << FG_LABEL << "   a  b  c  d  e  f  g  h" << RESET << '\n';

  if (!game_over) {
    bool white_to_move = board.SideToMove() == Color::kWhite;
    std::cout << FG_WHITE << (white_to_move ? "White" : "Black") << " to move"
              << RESET << '\n';
  }

  // Drain pending messages below the board (single composed frame).
  for (const auto& m : pending_) {
    std::cout << m << '\n';
  }
  pending_.clear();
}

void UnicodeView::ShowMessage(std::string_view msg) {
  pending_.emplace_back(msg);
}

void UnicodeView::ShowHistory(const Board& board) {
  const auto& history = board.History();
  if (history.empty()) {
    pending_.emplace_back("(no moves yet)");
    return;
  }
  for (const auto& entry : history) {
    pending_.push_back(MoveToUCI(entry.move));
  }
}

void UnicodeView::ShowIllegalMove(std::string_view input) {
  pending_.push_back("Illegal move: " + std::string(input));
}

void UnicodeView::ShowParseError(ParseError err, std::string_view input) {
  switch (err) {
    case ParseError::kEmpty:
      pending_.emplace_back("Empty input.");
      return;
    case ParseError::kBadSyntax:
      pending_.push_back("Bad input: " + std::string(input));
      return;
    case ParseError::kBadSquare:
      pending_.push_back("Unknown square: " + std::string(input));
      return;
    case ParseError::kIllegal:
      pending_.push_back("Illegal move: " + std::string(input));
      return;
  }
}

void UnicodeView::ShowResult(GameResult result) {
  switch (result) {
    case GameResult::kWhiteWins:
      pending_.emplace_back("WHITE WON!");
      return;
    case GameResult::kBlackWins:
      pending_.emplace_back("BLACK WON!");
      return;
    case GameResult::kDraw:
      pending_.emplace_back("DRAW BY STALEMATE!");
      return;
  }
}
