#include "board.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <optional>
#include <vector>

#include "color.h"
#include "move.h"
#include "piece_kind.h"
#include "square.h"
#include "test_helpers.h"

namespace dama {

TEST_CASE("Initial position: White to move and no captures are available") {
  Board board;
  CHECK(board.SideToMove() == Color::kWhite);
  for (const Move& m : board.LegalMoves()) {
    CHECK(m.captures.empty());
  }
}

TEST_CASE("White man captures forward (north)") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::WMan},
                                          {Sq("a4"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  CHECK(moves[0].from == Sq("a3"));
  CHECK(moves[0].to == Sq("a5"));
  REQUIRE(moves[0].captures.size() == 1);
  CHECK(moves[0].captures[0] == Sq("a4"));
}

TEST_CASE("White man captures sideways east") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("d3"), PieceKind::WMan},
                                          {Sq("e3"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  CHECK(moves[0].to == Sq("f3"));
  REQUIRE(moves[0].captures.size() == 1);
  CHECK(moves[0].captures[0] == Sq("e3"));
}

TEST_CASE("White man captures sideways west") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("d3"), PieceKind::WMan},
                                          {Sq("c3"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  CHECK(moves[0].to == Sq("b3"));
  REQUIRE(moves[0].captures.size() == 1);
  CHECK(moves[0].captures[0] == Sq("c3"));
}

TEST_CASE("White man does not capture backward") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("d4"), PieceKind::WMan},
                                          {Sq("d3"), PieceKind::BMan}}));
  for (const Move& m : board.LegalMoves()) {
    CHECK(m.captures.empty());
  }
}

TEST_CASE("Black man captures south") {
  Board board;
  board.Restore(MakeState(Color::kBlack, {{Sq("a6"), PieceKind::BMan},
                                          {Sq("a5"), PieceKind::WMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  CHECK(moves[0].to == Sq("a4"));
  REQUIRE(moves[0].captures.size() == 1);
  CHECK(moves[0].captures[0] == Sq("a5"));
}

TEST_CASE("Mandatory capture filters out quiet moves") {
  Board board;
  // a3 has a capture; h3 has only quiet moves available. Mandatory capture
  // should suppress h3's quiet moves entirely.
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::WMan},
                                          {Sq("a4"), PieceKind::BMan},
                                          {Sq("h3"), PieceKind::WMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  CHECK(moves[0].from == Sq("a3"));
  CHECK_FALSE(moves[0].captures.empty());
}

TEST_CASE("Mandatory longest capture filters out shorter chains") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::WMan},
                                          {Sq("a4"), PieceKind::BMan},
                                          {Sq("b5"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  CHECK(moves[0].captures.size() == 2);
  CHECK(moves[0].to == Sq("c5"));
}

TEST_CASE("Apply removes captured piece and flips side to move") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::WMan},
                                          {Sq("a4"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  board.Apply(moves[0]);
  CHECK(PieceAt(board, "a3") == PieceKind::Empty);
  CHECK(PieceAt(board, "a4") == PieceKind::Empty);
  CHECK(PieceAt(board, "a5") == PieceKind::WMan);
  CHECK(board.SideToMove() == Color::kBlack);
}

TEST_CASE("Apply on a multi-capture removes every captured piece") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::WMan},
                                          {Sq("a4"), PieceKind::BMan},
                                          {Sq("b5"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  board.Apply(moves[0]);
  CHECK(PieceAt(board, "a3") == PieceKind::Empty);
  CHECK(PieceAt(board, "a4") == PieceKind::Empty);
  CHECK(PieceAt(board, "a5") == PieceKind::Empty);
  CHECK(PieceAt(board, "b5") == PieceKind::Empty);
  CHECK(PieceAt(board, "c5") == PieceKind::WMan);
}

TEST_CASE("Man promotes to king when a capture ends on the last rank") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a6"), PieceKind::WMan},
                                          {Sq("a7"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  REQUIRE(moves.size() == 1);
  CHECK(moves[0].promotes);
  board.Apply(moves[0]);
  CHECK(PieceAt(board, "a8") == PieceKind::WKing);
}

TEST_CASE("King captures by flying along a file") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a1"), PieceKind::WKing},
                                          {Sq("a5"), PieceKind::BMan}}));
  auto moves = board.LegalMoves();
  // King can land on any empty square past the captured piece: a6, a7, a8.
  REQUIRE(moves.size() == 3);
  std::vector<Square> landings;
  for (const Move& m : moves) {
    CHECK(m.from == Sq("a1"));
    REQUIRE(m.captures.size() == 1);
    CHECK(m.captures[0] == Sq("a5"));
    landings.push_back(m.to);
  }
  CHECK(std::find(landings.begin(), landings.end(), Sq("a6")) !=
        landings.end());
  CHECK(std::find(landings.begin(), landings.end(), Sq("a7")) !=
        landings.end());
  CHECK(std::find(landings.begin(), landings.end(), Sq("a8")) !=
        landings.end());
}

TEST_CASE("Undo restores the prior position and side to move") {
  Board board;
  auto moves = board.LegalMoves();
  REQUIRE_FALSE(moves.empty());
  auto before = board.Snapshot();
  board.Apply(moves[0]);
  CHECK_FALSE(board.Snapshot() == before);
  board.Undo();
  CHECK(board.Snapshot() == before);
}

TEST_CASE(
    "Capture moves copied out of LegalMoves() temporary keep via/captures") {
  // Regression for the dangling-pointer bug in TuiApp::AdvancePath: holding
  // a pointer to a Move inside the temporary returned by LegalMoves() leaves
  // a dangle once the for-range ends. Copying the Move out by value (as the
  // fix does) must produce a Move whose via/captures arrays survive the
  // temporary's destruction. Under ASan/UBSan this test fails if the pattern
  // ever regresses.
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::WMan},
                                          {Sq("a4"), PieceKind::BMan},
                                          {Sq("b5"), PieceKind::BMan}}));
  std::optional<Move> picked;
  for (const Move& m : board.LegalMoves()) {
    if (m.from == Sq("a3") && m.to == Sq("c5")) {
      picked = m;
    }
  }
  REQUIRE(picked.has_value());
  REQUIRE(picked->via.size() == 1);
  CHECK(picked->via[0] == Sq("a5"));
  REQUIRE(picked->captures.size() == 2);
  CHECK(picked->captures[0] == Sq("a4"));
  CHECK(picked->captures[1] == Sq("b5"));
  board.Apply(*picked);
  CHECK(PieceAt(board, "c5") == PieceKind::WMan);
  CHECK(PieceAt(board, "a4") == PieceKind::Empty);
  CHECK(PieceAt(board, "b5") == PieceKind::Empty);
}

}  // namespace dama
