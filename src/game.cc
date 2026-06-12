#include "game.h"

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "command.h"
#include "move.h"
#include "parser.h"

namespace {
template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

const char* kInGameHelp =
    "Commands:\n"
    "  <from><to>  Make a move (e.g. e2e4 or e7e8)\n"
    "  undo               Undo last move\n"
    "  reset              Reset game to starting position\n"
    "  history            Show move history\n"
    "  resign             Resign current side\n"
    "  help               Show this help\n"
    "  quit, exit         Exit";
}  // namespace

Game::Game(std::unique_ptr<View> view,
           std::unique_ptr<InputSource> input_source,
           std::unique_ptr<Engine> engine, Color engine_side)
    : view_(std::move(view)),
      input_source_(std::move(input_source)),
      engine_(std::move(engine)),
      engine_side_(engine_side) {}

void Game::Play() {
  view_->Render(board_, is_game_over_);
  while (!quit_) {
    // Engine's turn: ask engine, apply directly, re-render.
    if (engine_ && !is_game_over_ && board_.SideToMove() == engine_side_) {
      Move m = engine_->Choose(board_);
      view_->ShowMessage("Engine plays " + MoveToUCI(m));
      HandleMove(m, MoveToUCI(m));
      view_->Render(board_, is_game_over_);
      continue;
    }

    std::optional<std::string> raw_input = input_source_->ReadLine();
    if (!raw_input) {
      break;
    }
    const std::string& raw = *raw_input;
    Command cmd = Parse(raw, board_);
    std::visit(
        Overloaded{
            [this, &raw](const Move& m) {
              if (is_game_over_) {
                view_->ShowMessage("Game over. Type 'reset' or 'quit'.");
                return;
              }
              HandleMove(m, raw);
            },
            [this](QuitCmd) {
              is_game_over_ = true;
              quit_ = true;
            },
            [this](UndoCmd) { board_.Undo(); },
            [this](ResetCmd) {
              board_.Reset();
              is_game_over_ = false;
              view_->ShowMessage("Game Reset");
            },
            [this](HistoryCmd) { view_->ShowHistory(board_); },
            [this](HelpCmd) { view_->ShowMessage(kInGameHelp); },
            [this](ResignCmd) {
              view_->ShowResult(board_.HandleResign());
              is_game_over_ = true;
            },
            [this, &raw](ParseError e) { view_->ShowParseError(e, raw); },
        },
        cmd);
    if (!quit_) {
      view_->Render(board_, is_game_over_);
    }
  }
}

void Game::HandleMove(const Move& move, std::string_view raw) {
  if (!board_.IsLegal(move)) {
    view_->ShowIllegalMove(raw);
    return;
  }
  board_.Apply(move);

  if (auto result = board_.Result()) {
    view_->ShowResult(*result);
    is_game_over_ = true;
  }
}
