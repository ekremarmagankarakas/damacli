#pragma once

#include "board_state.h"
#include "move.h"

struct HistoryEntry {
  BoardState pre_state;
  Move move;
};
