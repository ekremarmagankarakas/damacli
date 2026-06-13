#pragma once

#include <string>
#include <string_view>

#include "game_result.h"
#include "parse_error.h"

namespace dama {

class Board;

class View {
 public:
  virtual ~View() = default;

  // Write the current board (and "to move" line if !game_over) to stdout.
  // Implementations may emit ANSI escapes for clearing/coloring.
  virtual void Render(const Board& board, bool game_over) = 0;

  // Same content as Render but returned as a string and free of any
  // terminal-control escapes that mutate global terminal state (e.g. screen
  // clears). ANSI color escapes for piece glyphs are allowed; callers that
  // want a plain board can strip them.
  virtual std::string RenderToString(const Board& board,
                                     bool game_over) const = 0;

  virtual void ShowMessage(std::string_view msg) = 0;
  virtual void ShowHistory(const Board& board) = 0;
  virtual void ShowIllegalMove(std::string_view input) = 0;
  virtual void ShowParseError(ParseError err, std::string_view input) = 0;
  virtual void ShowResult(GameResult result) = 0;
};

}  // namespace dama
