#pragma once

#include <string>
#include <vector>

#include "square.h"

namespace dama {

struct Move {
  Square from;
  Square to;
  // Intermediate landings between from and to. Empty for quiet moves and for
  // single-jump captures.
  std::vector<Square> via;
  // Squares of pieces removed by this move (jumped over). Empty for quiet.
  std::vector<Square> captures;
  bool promotes = false;
};

inline std::string MoveToUCI(const Move& m) {
  std::string s = m.from.ToAlgebraic();
  if (m.captures.empty()) {
    return s + m.to.ToAlgebraic();
  }
  for (const auto& v : m.via) {
    s += 'x';
    s += v.ToAlgebraic();
  }
  s += 'x';
  s += m.to.ToAlgebraic();
  return s;
}

}  // namespace dama
