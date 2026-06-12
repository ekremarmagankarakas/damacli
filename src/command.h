#pragma once

#include <variant>

#include "move.h"
#include "parse_error.h"

struct QuitCmd {};
struct UndoCmd {};
struct ResetCmd {};
struct HistoryCmd {};
struct HelpCmd {};
struct ResignCmd {};

using Command = std::variant<Move, QuitCmd, UndoCmd, ResetCmd, HistoryCmd,
                             HelpCmd, ResignCmd, ParseError>;
