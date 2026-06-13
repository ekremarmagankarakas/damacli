#pragma once

#include <string_view>

#include "command.h"

namespace dama {

class Board;

// Dispatch a full input line: commands (quit/undo/reset/...) or a move.
// Move syntax:
//   a3a4         quiet move (from + to, 4 chars, no 'x')
//   a3xa5        single capture
//   a3xa5xc5...  capture chain
// Move strings are matched against Board::LegalMoves(); if no match,
// ParseError::kIllegal is returned.
Command Parse(std::string_view input, const Board& board);

}  // namespace dama
