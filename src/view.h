#pragma once

#include <string_view>

#include "game_result.h"
#include "parse_error.h"

class Board;

class View {
 public:
  virtual ~View() = default;
  virtual void Render(const Board& board, bool game_over) = 0;
  virtual void ShowMessage(std::string_view msg) = 0;
  virtual void ShowHistory(const Board& board) = 0;
  virtual void ShowIllegalMove(std::string_view input) = 0;
  virtual void ShowParseError(ParseError err, std::string_view input) = 0;
  virtual void ShowResult(GameResult result) = 0;
};
