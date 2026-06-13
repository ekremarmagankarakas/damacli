#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>

#include "config.h"
#include "engine.h"
#include "game.h"
#include "minimax_engine.h"
#include "stdin_input.h"
#include "text_view.h"
#include "tui_app.h"
#include "unicode_view.h"

using namespace dama;

namespace {

const char* kUsage =
    "Usage: damacli [--tui] [--no-tui|--cli] [--text] [--engine DEPTH]\n"
    "                [--no-engine] [--play-black] [--configure] [--help]\n"
    "Defaults: read from config file; fallback: TUI, depth 3, engine plays "
    "Black.\n"
    "\n"
    "  --tui            Force full-screen TUI mode\n"
    "  --no-tui, --cli  Force CLI view instead of TUI\n"
    "  --text           Use ASCII view instead of Unicode (CLI only)\n"
    "  --engine DEPTH   Set minimax search depth (default 3)\n"
    "  --no-engine      Disable engine (two-player mode)\n"
    "  --play-black     You play Black; engine plays White\n"
    "  --configure      Open config editor, save, and exit\n"
    "  --help, -h       Show this help\n"
    "\n"
    "In-game commands (CLI + TUI): move (e.g. a3a4 or a3xa5xc5),\n"
    "                              undo, reset, history, resign, quit,\n"
    "                              exit, help.\n";

}  // namespace

int main(int argc, char* argv[]) {
  std::optional<bool> override_tui;
  std::optional<bool> override_unicode;
  std::optional<int> override_engine_depth;
  std::optional<bool> override_play_black;
  bool configure = false;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--tui") {
      override_tui = true;
    } else if (arg == "--no-tui" || arg == "--cli") {
      override_tui = false;
    } else if (arg == "--text" || arg == "--ascii") {
      override_unicode = false;
    } else if (arg == "--unicode") {
      override_unicode = true;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << kUsage;
      return 0;
    } else if (arg == "--engine") {
      if (i + 1 >= argc) {
        std::cerr << "--engine requires a depth argument\n" << kUsage;
        return 2;
      }
      ++i;
      int d = std::atoi(argv[i]);
      if (d < 1) {
        std::cerr << "--engine depth must be >= 1\n";
        return 2;
      }
      override_engine_depth = d;
    } else if (arg == "--no-engine" || arg == "--solo") {
      override_engine_depth = 0;
    } else if (arg == "--play-black") {
      override_play_black = true;
    } else if (arg == "--configure") {
      configure = true;
    } else {
      std::cerr << "Unknown option: " << arg << '\n' << kUsage;
      return 2;
    }
  }

  auto config_path = DefaultConfigPath();
  Config cfg = LoadConfig(config_path);

  // CLI flags override config-file values.
  if (override_tui.has_value()) {
    cfg.tui = *override_tui;
  }
  if (override_unicode.has_value()) {
    cfg.unicode = *override_unicode;
  }
  if (override_engine_depth.has_value()) {
    cfg.engine_depth = *override_engine_depth;
  }
  if (override_play_black.has_value()) {
    cfg.play_black = *override_play_black;
  }

  if (configure) {
    RunConfigureScreen(config_path.string());
    return 0;
  }

  std::unique_ptr<Engine> engine;
  if (cfg.engine_depth > 0) {
    engine = std::make_unique<MinimaxEngine>(cfg.engine_depth);
  }

  Color engine_side = cfg.play_black ? Color::kWhite : Color::kBlack;

  if (cfg.tui) {
    TuiApp(std::move(engine), engine_side).Run();
    return 0;
  }

  std::unique_ptr<View> view =
      cfg.unicode ? std::unique_ptr<View>(std::make_unique<UnicodeView>())
                  : std::unique_ptr<View>(std::make_unique<TextView>());

  Game(std::move(view), std::make_unique<StdinInput>(), std::move(engine),
       engine_side)
      .Play();
  return 0;
}
