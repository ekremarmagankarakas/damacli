#pragma once

#include <optional>
#include <string>
#include <string_view>

struct Square {
  int row;  // rank 0 to 7
  int col;  // file 0 to 7 (a to h)

  bool operator==(const Square& other) const {
    return row == other.row && col == other.col;
  }

  std::string ToAlgebraic() const {
    return {static_cast<char>('a' + col), static_cast<char>('1' + row)};
  }

  // Parses "e4" style. Returns nullopt on bad input.
  static std::optional<Square> FromAlgebraic(std::string_view s) {
    if (s.size() != 2) {
      return std::nullopt;
    }
    int col = s[0] - 'a';
    int row = s[1] - '1';
    if (col < 0 || col > 7 || row < 0 || row > 7) {
      return std::nullopt;
    }
    return Square{row, col};
  }
};
