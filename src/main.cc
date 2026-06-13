#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

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
    "Usage: damacli [--no-tui|--cli] [--text] [--engine DEPTH] [--no-engine]\n"
    "                [--play-black] [--help]\n"
    "Defaults: Full-screen TUI, minimax engine at depth 3 playing Black.\n"
    "\n"
    "  --no-tui, --cli  Use the CLI prompt instead of the full-screen TUI\n"
    "  --tui            Force the TUI (already the default)\n"
    "  --text           Use ASCII view instead of Unicode (CLI only)\n"
    "  --engine DEPTH   Set minimax search depth (default 3)\n"
    "  --no-engine      Disable engine (two-player mode)\n"
    "  --play-black     You play Black; engine plays White\n"
    "  --help, -h       Show this help\n"
    "\n"
    "In-game commands (CLI + TUI): move (e.g. a3a4 or a3xa5xc5),\n"
    "                              undo, reset, history, resign, quit,\n"
    "                              exit, help.\n";

}  // namespace

int main(int argc, char* argv[]) {
  bool unicode = true;
  bool tui = true;
  int engine_depth = 3;
  Color engine_side = Color::kBlack;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--tui") {
      tui = true;
    } else if (arg == "--no-tui" || arg == "--cli") {
      tui = false;
    } else if (arg == "--text" || arg == "--ascii") {
      unicode = false;
    } else if (arg == "--unicode") {
      unicode = true;  // explicit; already the default
    } else if (arg == "--help" || arg == "-h") {
      std::cout << kUsage;
      return 0;
    } else if (arg == "--engine") {
      if (i + 1 >= argc) {
        std::cerr << "--engine requires a depth argument\n" << kUsage;
        return 2;
      }
      ++i;
      engine_depth = std::atoi(argv[i]);
      if (engine_depth < 1) {
        std::cerr << "--engine depth must be >= 1\n";
        return 2;
      }
    } else if (arg == "--no-engine" || arg == "--solo") {
      engine_depth = 0;
    } else if (arg == "--play-black") {
      engine_side = Color::kWhite;
    } else {
      std::cerr << "Unknown option: " << arg << '\n' << kUsage;
      return 2;
    }
  }

  std::unique_ptr<Engine> engine;
  if (engine_depth > 0) {
    engine = std::make_unique<MinimaxEngine>(engine_depth);
  }

  if (tui) {
    TuiApp(std::move(engine), engine_side).Run();
    return 0;
  }

  std::unique_ptr<View> view =
      unicode ? std::unique_ptr<View>(std::make_unique<UnicodeView>())
              : std::unique_ptr<View>(std::make_unique<TextView>());

  Game(std::move(view), std::make_unique<StdinInput>(), std::move(engine),
       engine_side)
      .Play();
  return 0;
}
