#pragma once

#include <memory>

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

}  // namespace dama
