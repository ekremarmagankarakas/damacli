#include "dama.h"

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "board.h"
#include "command.h"
#include "engine.h"
#include "game_result.h"
#include "minimax_engine.h"
#include "move.h"
#include "parser.h"
#include "text_view.h"
#include "unicode_view.h"
#include "view.h"

namespace dama {

namespace {

template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

Color FromSide(Side s) {
  return s == Side::White ? Color::kWhite : Color::kBlack;
}

Side ToSide(Color c) { return c == Color::kWhite ? Side::White : Side::Black; }

Result ToResult(GameResult r) {
  switch (r) {
    case GameResult::kWhiteWins:
      return Result::WhiteWins;
    case GameResult::kBlackWins:
      return Result::BlackWins;
    case GameResult::kDraw:
      return Result::Draw;
  }
  return Result::Draw;
}

const char* ResultText(GameResult r) {
  switch (r) {
    case GameResult::kWhiteWins:
      return "White wins";
    case GameResult::kBlackWins:
      return "Black wins";
    case GameResult::kDraw:
      return "Draw";
  }
  return "";
}

std::string ParseErrorText(ParseError e, std::string_view raw) {
  switch (e) {
    case ParseError::kEmpty:
      return "Empty input";
    case ParseError::kBadSyntax:
      return "Bad input: " + std::string(raw);
    case ParseError::kBadSquare:
      return "Unknown square: " + std::string(raw);
    case ParseError::kIllegal:
      return "Illegal move: " + std::string(raw);
  }
  return "";
}

std::unique_ptr<View> MakeView(ViewMode mode) {
  if (mode == ViewMode::Ascii) {
    return std::make_unique<TextView>();
  }
  return std::make_unique<UnicodeView>();
}

}  // namespace

struct Session::Impl {
  Board board;
  std::unique_ptr<View> view;
  std::unique_ptr<Engine> engine;
  Color engine_color;
  bool is_over = false;
  std::optional<GameResult> result;
};

Session::Session(Config cfg) : impl_(std::make_unique<Impl>()) {
  impl_->view = MakeView(cfg.view);
  impl_->engine_color = FromSide(cfg.engine_side);
  if (cfg.engine_depth > 0) {
    impl_->engine = std::make_unique<MinimaxEngine>(cfg.engine_depth);
  }
}

Session::~Session() = default;
Session::Session(Session&&) noexcept = default;
Session& Session::operator=(Session&&) noexcept = default;

std::string Session::Render() const {
  return impl_->view->RenderToString(impl_->board, impl_->is_over);
}

CommandResult Session::Apply(std::string_view cmd) {
  CommandResult out;

  if (impl_->is_over) {
    out.ok = false;
    out.message = "Game is over. Reset to start a new game.";
    return out;
  }

  Command parsed = Parse(cmd, impl_->board);

  std::visit(Overloaded{
                 [&](const Move& m) {
                   // Parser already matched against LegalMoves(); IsLegal is
                   // defensive.
                   if (!impl_->board.IsLegal(m)) {
                     out.ok = false;
                     out.message = "Illegal move: " + std::string(cmd);
                     return;
                   }
                   impl_->board.Apply(m);
                   out.message = MoveToUCI(m);
                   if (auto r = impl_->board.Result()) {
                     impl_->result = *r;
                     impl_->is_over = true;
                     out.message += "\n";
                     out.message += ResultText(*r);
                     return;
                   }
                   if (impl_->engine &&
                       impl_->board.SideToMove() == impl_->engine_color) {
                     Move em = impl_->engine->Choose(impl_->board);
                     impl_->board.Apply(em);
                     out.message += "\nEngine: ";
                     out.message += MoveToUCI(em);
                     if (auto r = impl_->board.Result()) {
                       impl_->result = *r;
                       impl_->is_over = true;
                       out.message += "\n";
                       out.message += ResultText(*r);
                     }
                   }
                 },
                 [&](QuitCmd) {
                   out.ok = false;
                   out.message = "quit is not supported via Apply";
                 },
                 [&](UndoCmd) {
                   impl_->board.Undo();
                   impl_->is_over = false;
                   impl_->result.reset();
                   out.message = "undo";
                 },
                 [&](ResetCmd) {
                   impl_->board.Reset();
                   impl_->is_over = false;
                   impl_->result.reset();
                   out.message = "reset";
                 },
                 [&](HistoryCmd) {
                   std::string s;
                   for (const auto& e : impl_->board.History()) {
                     if (!s.empty()) {
                       s += '\n';
                     }
                     s += MoveToUCI(e.move);
                   }
                   out.message = std::move(s);
                 },
                 [&](HelpCmd) {
                   out.ok = false;
                   out.message = "help is not supported via Apply";
                 },
                 [&](ResignCmd) {
                   auto r = impl_->board.HandleResign();
                   impl_->result = r;
                   impl_->is_over = true;
                   out.message = ResultText(r);
                 },
                 [&](ParseError e) {
                   out.ok = false;
                   out.message = ParseErrorText(e, cmd);
                 },
             },
             parsed);

  return out;
}

bool Session::IsOver() const { return impl_->is_over; }

Result Session::Outcome() const {
  if (!impl_->is_over) {
    return Result::Ongoing;
  }
  if (!impl_->result) {
    return Result::Draw;
  }
  return ToResult(*impl_->result);
}

Side Session::ToMove() const { return ToSide(impl_->board.SideToMove()); }

std::vector<std::string> Session::History() const {
  std::vector<std::string> out;
  for (const auto& e : impl_->board.History()) {
    out.push_back(MoveToUCI(e.move));
  }
  return out;
}

}  // namespace dama
