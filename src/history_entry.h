#pragma once

#include "board_state.h"
#include "move.h"

namespace dama {

struct HistoryEntry {
  BoardState pre_state;
  Move move;
};

}  // namespace dama
