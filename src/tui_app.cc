#include "tui_app.h"

#include <algorithm>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "board.h"
#include "command.h"
#include "config.h"
#include "game_result.h"
#include "move.h"
#include "parser.h"
#include "piece_kind.h"
#include "square.h"

namespace dama {

namespace {

template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

// ---- Braille pixel art for pieces ----
//
// Each piece is drawn on an 8-wide × 16-tall pixel grid. Braille block
// (U+2800-U+28FF) packs a 2×4 pixel sub-cell into one glyph, so the grid
// maps to 4 chars wide × 4 chars tall — four text lines that sit inside
// the cell's vbox.

constexpr int kArtRows = 16;
constexpr int kArtBraille = 4;  // braille chars per side (4×4 grid)

struct PieceArt {
  const char* rows[kArtRows];
};

constexpr PieceArt kManArt = {{
    "........",
    "........",
    "...##...",
    "..####..",
    ".######.",
    "########",
    "########",
    "########",
    "########",
    "########",
    "########",
    "########",
    ".######.",
    ".######.",
    "########",
    "########",
}};

constexpr PieceArt kKingArt = {{
    "##.##.##",
    "########",
    ".######.",
    ".######.",
    "########",
    "########",
    "########",
    "########",
    "########",
    "########",
    "########",
    "########",
    ".######.",
    ".######.",
    "########",
    "########",
}};

// UTF-8 encoding of braille codepoint 0x2800 + mask (mask is 0-255).
std::string BrailleChar(uint8_t mask) {
  std::string s;
  s += static_cast<char>(0xE2);
  s += static_cast<char>(0xA0 + (mask >> 6));
  s += static_cast<char>(0x80 + (mask & 0x3F));
  return s;
}

// Render a 16×8 art grid to four lines of 4 braille glyphs each.
std::vector<std::string> RenderArt(const PieceArt& art) {
  std::vector<std::string> lines;
  lines.reserve(kArtBraille);
  for (int row_block = 0; row_block < kArtBraille; ++row_block) {
    std::string line;
    for (int col_block = 0; col_block < kArtBraille; ++col_block) {
      int r = row_block * 4;
      int c = col_block * 2;
      uint8_t mask = 0;
      if (art.rows[r + 0][c + 0] == '#') {
        mask |= 0x01;
      }
      if (art.rows[r + 1][c + 0] == '#') {
        mask |= 0x02;
      }
      if (art.rows[r + 2][c + 0] == '#') {
        mask |= 0x04;
      }
      if (art.rows[r + 3][c + 0] == '#') {
        mask |= 0x40;
      }
      if (art.rows[r + 0][c + 1] == '#') {
        mask |= 0x08;
      }
      if (art.rows[r + 1][c + 1] == '#') {
        mask |= 0x10;
      }
      if (art.rows[r + 2][c + 1] == '#') {
        mask |= 0x20;
      }
      if (art.rows[r + 3][c + 1] == '#') {
        mask |= 0x80;
      }
      line += BrailleChar(mask);
    }
    lines.push_back(line);
  }
  return lines;
}

std::vector<std::string> ArtForPieceKind(PieceKind pk) {
  switch (pk) {
    case PieceKind::kWMan:
    case PieceKind::kBMan:
      return RenderArt(kManArt);
    case PieceKind::kWKing:
    case PieceKind::kBKing:
      return RenderArt(kKingArt);
    case PieceKind::kEmpty: {
      std::string blank;
      for (int i = 0; i < kArtBraille; ++i) {
        blank += BrailleChar(0);
      }
      return std::vector<std::string>(kArtBraille, blank);
    }
  }
  return {};
}

const char* ResultLabel(GameResult r) {
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

std::string ParseErrorLabel(ParseError e, const std::string& raw) {
  switch (e) {
    case ParseError::kEmpty:
      return "Empty input";
    case ParseError::kBadSyntax:
      return "Bad input: " + raw;
    case ParseError::kBadSquare:
      return "Unknown square: " + raw;
    case ParseError::kIllegal:
      return "Illegal: " + raw;
  }
  return "";
}

}  // namespace

struct TuiApp::Impl {
  Board board;
  std::unique_ptr<Engine> engine;
  Color engine_side;
  bool game_over = false;
  std::optional<GameResult> result;

  int cursor_row = 2;
  int cursor_col = 0;

  // Incremental move builder: empty = no selection. First entry = origin,
  // subsequent entries = landings already chosen for the in-progress chain.
  std::vector<Square> path;

  std::string text_input;
  std::string status =
      "arrows: cursor  enter: select/land  esc: cancel  "
      "type: a3a4 / a3xa5xc5 / undo / reset / resign / quit";
  std::vector<std::string> log;

  bool exit_requested = false;

  void AppendLog(std::string s) {
    log.push_back(std::move(s));
    while (log.size() > 200) {
      log.erase(log.begin());
    }
  }

  bool IsLastMoveSquare(int r, int c) const {
    const auto& hist = board.History();
    if (hist.empty()) {
      return false;
    }
    const Move& m = hist.back().move;
    if (m.from.row == r && m.from.col == c) {
      return true;
    }
    if (m.to.row == r && m.to.col == c) {
      return true;
    }
    for (const auto& v : m.via) {
      if (v.row == r && v.col == c) {
        return true;
      }
    }
    for (const auto& cap : m.captures) {
      if (cap.row == r && cap.col == c) {
        return true;
      }
    }
    return false;
  }

  bool IsOnPath(int r, int c) const {
    for (const auto& s : path) {
      if (s.row == r && s.col == c) {
        return true;
      }
    }
    return false;
  }

  // Squares the cursor could land on next, given the in-progress path.
  std::vector<Square> CandidateNextLandings() const {
    std::vector<Square> out;
    if (path.empty()) {
      return out;
    }
    for (const auto& m : board.LegalMoves()) {
      if (m.from != path.front()) {
        continue;
      }
      std::vector<Square> full;
      full.push_back(m.from);
      for (const auto& v : m.via) {
        full.push_back(v);
      }
      full.push_back(m.to);
      if (full.size() <= path.size()) {
        continue;
      }
      bool prefix_ok = true;
      for (std::size_t i = 0; i < path.size(); ++i) {
        if (full[i] != path[i]) {
          prefix_ok = false;
          break;
        }
      }
      if (!prefix_ok) {
        continue;
      }
      Square next = full[path.size()];
      if (std::find(out.begin(), out.end(), next) == out.end()) {
        out.push_back(next);
      }
    }
    return out;
  }

  bool IsCandidate(int r, int c) const {
    for (const auto& s : CandidateNextLandings()) {
      if (s.row == r && s.col == c) {
        return true;
      }
    }
    return false;
  }

  void AdvancePath(Square cursor) {
    if (game_over) {
      status = "Game is over. 'reset' to play again.";
      return;
    }

    if (path.empty()) {
      auto pk = board.At(cursor.row, cursor.col);
      if (!pk || *pk == PieceKind::kEmpty) {
        status = "No piece on " + cursor.ToAlgebraic();
        return;
      }
      bool white = *pk == PieceKind::kWMan || *pk == PieceKind::kWKing;
      Color side = white ? Color::kWhite : Color::kBlack;
      if (side != board.SideToMove()) {
        status = "Not your piece";
        return;
      }
      bool has_move = false;
      for (const auto& m : board.LegalMoves()) {
        if (m.from == cursor) {
          has_move = true;
          break;
        }
      }
      if (!has_move) {
        status = "No legal moves for that piece";
        return;
      }
      path.push_back(cursor);
      status = "selected " + cursor.ToAlgebraic() + " — pick destination";
      return;
    }

    // Re-selecting origin cancels.
    if (path.size() == 1 && path.front() == cursor) {
      path.clear();
      status = "selection cleared";
      return;
    }

    std::vector<Square> tentative = path;
    tentative.push_back(cursor);

    // Copy any exact match by value: holding a pointer into the temporary
    // returned by LegalMoves() would dangle once the for-range ends.
    std::optional<Move> exact;
    bool extends = false;
    for (const auto& m : board.LegalMoves()) {
      std::vector<Square> full;
      full.push_back(m.from);
      for (const auto& v : m.via) {
        full.push_back(v);
      }
      full.push_back(m.to);
      if (full.size() < tentative.size()) {
        continue;
      }
      bool prefix_ok = true;
      for (std::size_t i = 0; i < tentative.size(); ++i) {
        if (full[i] != tentative[i]) {
          prefix_ok = false;
          break;
        }
      }
      if (!prefix_ok) {
        continue;
      }
      if (full.size() == tentative.size()) {
        exact = m;
      } else {
        extends = true;
      }
    }

    if (exact) {
      CommitMove(*exact);
      return;
    }
    if (extends) {
      path = tentative;
      status = "step ok — pick next landing";
      return;
    }
    status = "illegal step from " + path.back().ToAlgebraic() + " to " +
             cursor.ToAlgebraic();
  }

  void CommitMove(const Move& m) {
    const char* mover = (board.SideToMove() == Color::kWhite) ? "W" : "B";
    board.Apply(m);
    AppendLog(std::string(mover) + ": " + MoveToUCI(m));
    path.clear();
    status = std::string(mover) + " played " + MoveToUCI(m);
    if (auto r = board.Result()) {
      result = *r;
      game_over = true;
      AppendLog(ResultLabel(*r));
      status = ResultLabel(*r);
      return;
    }
    if (engine && board.SideToMove() == engine_side) {
      RunEngine();
    }
  }

  void RunEngine() {
    if (game_over || !engine) {
      return;
    }
    const char* mover = (board.SideToMove() == Color::kWhite) ? "W" : "B";
    Move em = engine->Choose(board);
    board.Apply(em);
    AppendLog(std::string("Engine ") + mover + ": " + MoveToUCI(em));
    status = std::string("Engine (") + mover + ") played " + MoveToUCI(em);
    if (auto r = board.Result()) {
      result = *r;
      game_over = true;
      AppendLog(ResultLabel(*r));
      status = ResultLabel(*r);
    }
  }

  void ProcessTextCommand(const std::string& raw) {
    if (raw.empty()) {
      return;
    }
    Command cmd = Parse(raw, board);
    std::visit(
        Overloaded{
            [&](const Move& m) {
              if (game_over) {
                status = "Game is over. 'reset' to play again.";
                return;
              }
              path.clear();
              CommitMove(m);
            },
            [&](QuitCmd) { exit_requested = true; },
            [&](UndoCmd) {
              board.Undo();
              game_over = false;
              result.reset();
              path.clear();
              AppendLog("undo");
              status = "undone";
            },
            [&](ResetCmd) {
              board.Reset();
              game_over = false;
              result.reset();
              path.clear();
              log.clear();
              AppendLog("reset");
              status = "reset";
            },
            [&](HistoryCmd) { status = "history on the right panel"; },
            [&](HelpCmd) {
              status =
                  "arrows: cursor  enter: select/land  esc: cancel  "
                  "type: a3a4 / a3xa5xc5 / undo / reset / resign / quit";
            },
            [&](ResignCmd) {
              result = board.HandleResign();
              game_over = true;
              AppendLog(std::string("resigned: ") + ResultLabel(*result));
              status = ResultLabel(*result);
            },
            [&](ParseError e) { status = ParseErrorLabel(e, raw); },
        },
        cmd);
  }
};

namespace {

ftxui::Element RenderCell(const TuiApp::Impl& s, int row, int col) {
  using namespace ftxui;

  PieceKind pk = s.board.At(row, col).value_or(PieceKind::kEmpty);

  bool is_cursor = (s.cursor_row == row && s.cursor_col == col);
  bool on_path = s.IsOnPath(row, col);
  bool is_cand = s.IsCandidate(row, col);
  bool is_last = s.IsLastMoveSquare(row, col);
  bool light = (row + col) % 2 == 1;

  // Muted RGB palette: pieces are uniform white/black so neither washes out.
  ftxui::Color bg;
  if (is_cursor) {
    bg = ftxui::Color::RGB(80, 130, 180);  // muted steel blue
  } else if (on_path) {
    bg = ftxui::Color::RGB(95, 145, 95);  // muted green
  } else if (is_cand) {
    bg = ftxui::Color::RGB(110, 160, 155);  // muted teal
  } else if (is_last) {
    bg = ftxui::Color::RGB(180, 145, 70);  // muted amber
  } else if (light) {
    bg = ftxui::Color::RGB(170, 155, 130);  // warm light tan
  } else {
    bg = ftxui::Color::RGB(110, 90, 75);  // warm dark brown
  }

  ftxui::Color fg = ftxui::Color::Default;
  if (pk != PieceKind::kEmpty) {
    bool white = pk == PieceKind::kWMan || pk == PieceKind::kWKing;
    fg = white ? ftxui::Color::White : ftxui::Color::Black;
  }

  auto art = ArtForPieceKind(pk);
  std::vector<ftxui::Element> lines;
  lines.reserve(art.size());
  for (const auto& l : art) {
    lines.push_back(text(l) | color(fg) | bold);
  }
  auto art_block = vbox(lines) | hcenter;
  return vbox({
             filler(),
             art_block,
             filler(),
         }) |
         bgcolor(bg) | flex;
}

ftxui::Element RenderBoardElement(const TuiApp::Impl& s) {
  using namespace ftxui;

  std::vector<std::vector<Element>> grid;
  for (int row = 7; row >= 0; --row) {
    std::vector<Element> row_cells;
    for (int col = 0; col < 8; ++col) {
      row_cells.push_back(RenderCell(s, row, col));
    }
    grid.push_back(row_cells);
  }
  auto board_grid = gridbox(grid) | flex;

  auto file_row = [] {
    std::vector<Element> cells;
    for (int c = 0; c < 8; ++c) {
      std::string lab(1, char('a' + c));
      cells.push_back(text(lab) | center | dim | flex);
    }
    return hbox({text("  "), hbox(cells) | flex});
  };

  std::vector<Element> rank_cells;
  for (int row = 7; row >= 0; --row) {
    rank_cells.push_back(text(std::to_string(row + 1)) | center | dim | yflex);
  }
  auto rank_col = vbox(rank_cells) | size(WIDTH, EQUAL, 2);

  auto middle = hbox({rank_col, board_grid}) | yflex;

  return vbox({file_row(), middle, file_row()}) | flex | border;
}

ftxui::Element RenderSidePanel(const TuiApp::Impl& s) {
  using namespace ftxui;

  std::string stm;
  if (s.game_over && s.result) {
    stm = std::string("Game over: ") + ResultLabel(*s.result);
  } else {
    stm = "To move: ";
    stm += (s.board.SideToMove() == Color::kWhite) ? "White ⛂" : "Black ⛀";
  }

  std::vector<Element> hist;
  hist.push_back(text("History") | bold | underlined);
  const auto& h = s.board.History();
  if (h.empty()) {
    hist.push_back(text("(no moves yet)") | dim);
  } else {
    for (std::size_t i = 0; i < h.size(); ++i) {
      std::string line;
      if (i % 2 == 0) {
        line = std::to_string(i / 2 + 1) + ". ";
      } else {
        line = "   ... ";
      }
      line += MoveToUCI(h[i].move);
      hist.push_back(text(line));
    }
  }

  std::vector<Element> log_lines;
  log_lines.push_back(text("Log") | bold | underlined);
  int start = std::max(0, static_cast<int>(s.log.size()) - 8);
  for (int i = start; i < static_cast<int>(s.log.size()); ++i) {
    log_lines.push_back(text(s.log[i]) | dim);
  }
  if (log_lines.size() == 1) {
    log_lines.push_back(text("(empty)") | dim);
  }

  return vbox({
             text("dama") | bold | center,
             separator(),
             text(stm),
             separator(),
             vbox(hist) | yflex,
             separator(),
             vbox(log_lines),
         }) |
         flex;
}

}  // namespace

TuiApp::TuiApp(std::unique_ptr<Engine> engine, Color engine_side)
    : impl_(std::make_unique<Impl>()) {
  impl_->engine = std::move(engine);
  impl_->engine_side = engine_side;
  // If engine plays White and game just started, let it move first.
  if (impl_->engine && impl_->board.SideToMove() == impl_->engine_side) {
    impl_->RunEngine();
  }
}

TuiApp::~TuiApp() = default;

void TuiApp::Run() {
  using namespace ftxui;
  auto screen = ScreenInteractive::Fullscreen();

  auto input = Input(&impl_->text_input, "type a command + Enter");

  auto root = CatchEvent(input, [this, &screen](Event e) {
    if (e == Event::ArrowUp) {
      impl_->cursor_row = std::min(7, impl_->cursor_row + 1);
      return true;
    }
    if (e == Event::ArrowDown) {
      impl_->cursor_row = std::max(0, impl_->cursor_row - 1);
      return true;
    }
    if (e == Event::ArrowLeft) {
      impl_->cursor_col = std::max(0, impl_->cursor_col - 1);
      return true;
    }
    if (e == Event::ArrowRight) {
      impl_->cursor_col = std::min(7, impl_->cursor_col + 1);
      return true;
    }
    if (e == Event::Escape) {
      if (!impl_->path.empty()) {
        impl_->path.clear();
        impl_->status = "selection cleared";
      } else if (!impl_->text_input.empty()) {
        impl_->text_input.clear();
        impl_->status = "input cleared";
      } else {
        screen.ExitLoopClosure()();
      }
      return true;
    }
    if (e == Event::Return) {
      // Handle Return ourselves so text commands always fire, regardless of
      // the Input component's focus or on_enter wiring.
      if (impl_->text_input.empty()) {
        impl_->AdvancePath(Square{impl_->cursor_row, impl_->cursor_col});
        return true;
      }
      std::string cmd = impl_->text_input;
      impl_->text_input.clear();
      if (cmd == "quit" || cmd == "exit") {
        screen.ExitLoopClosure()();
        return true;
      }
      impl_->ProcessTextCommand(cmd);
      if (impl_->exit_requested) {
        screen.ExitLoopClosure()();
      }
      return true;
    }
    return false;
  });

  constexpr int kSidePanelWidth = 36;
  constexpr int kStatusHeight = 3;
  constexpr int kInputHeight = 3;

  auto renderer = Renderer(root, [this, input] {
    auto board_el = RenderBoardElement(*impl_) | flex;
    auto status_el = hbox({text("● ") | color(ftxui::Color::Yellow),
                           text(impl_->status) | ftxui::xflex_shrink}) |
                     border | size(HEIGHT, EQUAL, kStatusHeight);
    auto input_el = hbox({text(" > ") | bold | color(ftxui::Color::Cyan),
                          input->Render() | ftxui::xflex}) |
                    border | size(HEIGHT, EQUAL, kInputHeight);
    auto left = vbox({board_el, status_el, input_el}) | flex;
    auto right = (RenderSidePanel(*impl_) | border) |
                 size(WIDTH, EQUAL, kSidePanelWidth);
    return hbox({left, right}) | flex;
  });

  screen.Loop(renderer);
}

void RunConfigureScreen(const std::string& config_path_str) {
  using namespace ftxui;

  std::filesystem::path config_path(config_path_str);
  Config cfg = LoadConfig(config_path);

  bool tui_on = cfg.tui;
  bool play_black = cfg.play_black;
  int engine_depth = cfg.engine_depth;
  std::string status_msg = "Config: " + config_path_str;

  // 0 = Unicode, 1 = Text
  int view_selected = cfg.unicode ? 0 : 1;
  std::vector<std::string> view_entries = {"Unicode (⛂⛃)", "Text (o O / x X)"};

  auto screen = ScreenInteractive::Fullscreen();

  auto tui_cb = Checkbox("Default to TUI mode", &tui_on);
  auto black_cb = Checkbox("Play as Black (engine plays White)", &play_black);
  auto view_radio = Radiobox(&view_entries, &view_selected);
  auto depth_dec = Button(" − ", [&] {
    if (engine_depth > 0) {
      --engine_depth;
    }
  });
  auto depth_inc = Button(" + ", [&] {
    if (engine_depth < 10) {
      ++engine_depth;
    }
  });

  auto save_btn = Button("Save & Exit", [&] {
    Config out;
    out.tui = tui_on;
    out.unicode = (view_selected == 0);
    out.play_black = play_black;
    out.engine_depth = engine_depth;
    try {
      SaveConfig(out, config_path);
    } catch (...) {
      status_msg = "Error: could not write " + config_path_str;
      return;
    }
    screen.ExitLoopClosure()();
  });

  auto cancel_btn = Button("Cancel", [&] { screen.ExitLoopClosure()(); });

  auto container = Container::Vertical({
      tui_cb,
      view_radio,
      black_cb,
      Container::Horizontal({depth_dec, depth_inc}),
      Container::Horizontal({save_btn, cancel_btn}),
  });

  auto renderer = Renderer(container, [&] {
    std::string depth_label =
        engine_depth == 0 ? "disabled" : std::to_string(engine_depth);
    return vbox({
               text("damacli — Configuration") | bold | center,
               separator(),
               tui_cb->Render() | xflex,
               separator(),
               text("CLI view (when TUI is off)") | dim,
               view_radio->Render(),
               separator(),
               black_cb->Render() | xflex,
               hbox({text("Engine depth: "), depth_dec->Render(),
                     text(" " + depth_label + " ") | bold | center |
                         size(WIDTH, EQUAL, 10),
                     depth_inc->Render()}),
               separator(),
               hbox({save_btn->Render(), text("  "), cancel_btn->Render()}) |
                   center,
               separator(),
               text(status_msg) | dim | center,
           }) |
           border | center;
  });

  screen.Loop(renderer);
}

}  // namespace dama
