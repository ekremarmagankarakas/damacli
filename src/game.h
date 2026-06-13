#pragma once

#include <memory>
#include <string_view>

#include "board.h"
#include "engine.h"
#include "input_source.h"
#include "view.h"

namespace dama {

struct Move;

class Game {
 public:
  explicit Game(std::unique_ptr<View> view,
                std::unique_ptr<InputSource> input_source,
                std::unique_ptr<Engine> engine = nullptr,
                Color engine_side = Color::kBlack);
  void Play();

 private:
  void HandleMove(const Move& move, std::string_view raw);

  Board board_;
  std::unique_ptr<View> view_;
  std::unique_ptr<InputSource> input_source_;
  std::unique_ptr<Engine> engine_;
  Color engine_side_;
  bool is_game_over_ = false;
  bool quit_ = false;
};

}  // namespace dama
