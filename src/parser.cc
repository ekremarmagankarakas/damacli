#include "parser.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "board.h"
#include "move.h"
#include "square.h"

namespace {

// Tokenize a move string into squares. Forms accepted:
//   "a3a4"         (quiet, exactly 4 chars, no 'x')
//   "a3xa5"        (single capture)
//   "a3xa5xc5x..." (capture chain)
std::optional<std::vector<Square>> Tokenize(std::string_view s) {
  std::vector<Square> sqs;

  if (s.find('x') == std::string_view::npos) {
    if (s.size() != 4) return std::nullopt;
    auto a = Square::FromAlgebraic(s.substr(0, 2));
    auto b = Square::FromAlgebraic(s.substr(2, 2));
    if (!a || !b) return std::nullopt;
    return std::vector<Square>{*a, *b};
  }

  std::size_t i = 0;
  bool need_x = false;
  while (i < s.size()) {
    if (need_x) {
      if (s[i] != 'x') return std::nullopt;
      ++i;
    }
    if (i + 2 > s.size()) return std::nullopt;
    auto sq = Square::FromAlgebraic(s.substr(i, 2));
    if (!sq) return std::nullopt;
    sqs.push_back(*sq);
    i += 2;
    need_x = true;
  }
  if (sqs.size() < 2) return std::nullopt;
  return sqs;
}

}  // namespace

Command Parse(std::string_view input, const Board& board) {
  if (input.empty()) {
    return ParseError::kEmpty;
  }
  if (input == "quit" || input == "exit") return QuitCmd{};
  if (input == "reset") return ResetCmd{};
  if (input == "undo") return UndoCmd{};
  if (input == "resign") return ResignCmd{};
  if (input == "history") return HistoryCmd{};
  if (input == "help") return HelpCmd{};

  auto sqs = Tokenize(input);
  if (!sqs) return ParseError::kBadSyntax;

  bool wants_capture = input.find('x') != std::string_view::npos;
  Square from = sqs->front();
  Square to = sqs->back();
  std::vector<Square> via(sqs->begin() + 1, sqs->end() - 1);

  for (const auto& m : board.LegalMoves()) {
    if (m.from != from || m.to != to || m.via != via) continue;
    if (wants_capture != !m.captures.empty()) continue;
    return m;
  }
  return ParseError::kIllegal;
}
