#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "view.h"

class UnicodeView : public View {
 public:
  void Render(const Board& board, bool game_over) override;
  void ShowMessage(std::string_view msg) override;
  void ShowHistory(const Board& board) override;
  void ShowIllegalMove(std::string_view input) override;
  void ShowParseError(ParseError err, std::string_view input) override;
  void ShowResult(GameResult result) override;

 private:
  std::vector<std::string> pending_;
};
