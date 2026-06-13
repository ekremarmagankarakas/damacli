#pragma once

#include <memory>
#include <string>

#include "color.h"
#include "engine.h"

namespace dama {

class TuiApp {
 public:
  // Opaque pimpl; consumers cannot construct Impl without its definition (in
  // tui_app.cc) and so cannot reach engine internals through it.
  struct Impl;

  TuiApp(std::unique_ptr<Engine> engine, Color engine_side);
  ~TuiApp();
  TuiApp(const TuiApp&) = delete;
  TuiApp& operator=(const TuiApp&) = delete;

  void Run();

 private:
  std::unique_ptr<Impl> impl_;
};

// Opens a full-screen config editor, saves to config_path, and returns.
void RunConfigureScreen(const std::string& config_path);

}  // namespace dama
