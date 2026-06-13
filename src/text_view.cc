#include "text_view.h"

#include <iostream>
#include <sstream>

#include "board.h"
#include "game_result.h"
#include "piece_kind.h"

namespace dama {
namespace {
char Glyph(PieceKind k) {
  switch (k) {
    case PieceKind::WMan:  return 'o';
    case PieceKind::WKing: return 'O';
    case PieceKind::BMan:  return 'x';
    case PieceKind::BKing: return 'X';
    case PieceKind::Empty: return '.';
  }
  return '.';
}
}  // namespace

std::string TextView::RenderToString(const Board& board, bool game_over) const {
  std::ostringstream os;
  for (int row = Board::kSize - 1; row >= 0; --row) {
    os << (row + 1) << ' ';
    for (int col = 0; col < Board::kSize; ++col) {
      auto pk = board.At(row, col);
      os << (pk ? Glyph(*pk) : '?') << ' ';
    }
    os << '\n';
  }
  os << "  a b c d e f g h\n";
  if (!game_over) {
    os << (board.SideToMove() == Color::kWhite ? "White" : "Black")
       << " to move\n";
  }
  return os.str();
}

void TextView::Render(const Board& board, bool game_over) {
  std::cout << RenderToString(board, game_over);
}

void TextView::ShowMessage(std::string_view msg) { std::cout << msg << '\n'; }

void TextView::ShowHistory(const Board& board) {
  for (const auto& entry : board.History()) {
    std::cout << MoveToUCI(entry.move) << '\n';
  }
}

void TextView::ShowIllegalMove(std::string_view input) {
  std::cout << "Illegal move: " << input << '\n';
}

void TextView::ShowParseError(ParseError err, std::string_view input) {
  switch (err) {
    case ParseError::kEmpty:
      std::cout << "Empty input.\n";
      return;
    case ParseError::kBadSyntax:
      std::cout << "Bad input: " << input << '\n';
      return;
    case ParseError::kBadSquare:
      std::cout << "Unknown square: " << input << '\n';
      return;
    case ParseError::kIllegal:
      std::cout << "Illegal move: " << input << '\n';
      return;
  }
}

void TextView::ShowResult(GameResult result) {
  switch (result) {
    case GameResult::kWhiteWins:
      std::cout << "WHITE WON!\n";
      return;
    case GameResult::kBlackWins:
      std::cout << "BLACK WON!\n";
      return;
    case GameResult::kDraw:
      std::cout << "DRAW!\n";
      return;
  }
}

}  // namespace dama
