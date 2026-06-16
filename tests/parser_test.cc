#include "parser.h"

#include <doctest/doctest.h>

#include <string_view>
#include <variant>

#include "board.h"
#include "command.h"
#include "move.h"
#include "parse_error.h"
#include "piece_kind.h"
#include "square.h"
#include "test_helpers.h"

namespace dama {

namespace {

Command ParseInitial(std::string_view input) {
  Board board;
  return Parse(input, board);
}

ParseError ErrorOf(const Command& cmd) {
  REQUIRE(std::holds_alternative<ParseError>(cmd));
  return std::get<ParseError>(cmd);
}

}  // namespace

TEST_CASE("Parser maps reserved words to commands") {
  CHECK(std::holds_alternative<QuitCmd>(ParseInitial("quit")));
  CHECK(std::holds_alternative<QuitCmd>(ParseInitial("exit")));
  CHECK(std::holds_alternative<ResetCmd>(ParseInitial("reset")));
  CHECK(std::holds_alternative<UndoCmd>(ParseInitial("undo")));
  CHECK(std::holds_alternative<ResignCmd>(ParseInitial("resign")));
  CHECK(std::holds_alternative<HistoryCmd>(ParseInitial("history")));
  CHECK(std::holds_alternative<HelpCmd>(ParseInitial("help")));
}

TEST_CASE("Parser flags empty input") {
  CHECK(ErrorOf(ParseInitial("")) == ParseError::kEmpty);
}

TEST_CASE("Parser rejects malformed move text as bad syntax") {
  CHECK(ErrorOf(ParseInitial("a3")) == ParseError::kBadSyntax);
  CHECK(ErrorOf(ParseInitial("a3a")) == ParseError::kBadSyntax);
  CHECK(ErrorOf(ParseInitial("a3a4a5")) == ParseError::kBadSyntax);
  CHECK(ErrorOf(ParseInitial("z9z9")) == ParseError::kBadSyntax);
  CHECK(ErrorOf(ParseInitial("xxxx")) == ParseError::kBadSyntax);
}

TEST_CASE("Parser accepts a legal quiet move on the initial board") {
  Command cmd = ParseInitial("a3a4");
  REQUIRE(std::holds_alternative<Move>(cmd));
  const Move& move = std::get<Move>(cmd);
  CHECK(move.from == Sq("a3"));
  CHECK(move.to == Sq("a4"));
  CHECK(move.via.empty());
  CHECK(move.captures.empty());
}

TEST_CASE("Parser rejects a quiet move that has no matching legal move") {
  CHECK(ErrorOf(ParseInitial("a3a5")) == ParseError::kIllegal);
}

TEST_CASE("Parser accepts a single capture") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::kWMan},
                                          {Sq("a4"), PieceKind::kBMan}}));
  Command cmd = Parse("a3xa5", board);
  REQUIRE(std::holds_alternative<Move>(cmd));
  const Move& move = std::get<Move>(cmd);
  CHECK(move.from == Sq("a3"));
  CHECK(move.to == Sq("a5"));
  CHECK(move.via.empty());
  REQUIRE(move.captures.size() == 1);
  CHECK(move.captures[0] == Sq("a4"));
}

TEST_CASE("Parser accepts a multi-capture chain") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::kWMan},
                                          {Sq("a4"), PieceKind::kBMan},
                                          {Sq("b5"), PieceKind::kBMan}}));
  Command cmd = Parse("a3xa5xc5", board);
  REQUIRE(std::holds_alternative<Move>(cmd));
  const Move& move = std::get<Move>(cmd);
  CHECK(move.from == Sq("a3"));
  CHECK(move.to == Sq("c5"));
  REQUIRE(move.via.size() == 1);
  CHECK(move.via[0] == Sq("a5"));
  REQUIRE(move.captures.size() == 2);
  CHECK(move.captures[0] == Sq("a4"));
  CHECK(move.captures[1] == Sq("b5"));
}

TEST_CASE("Parser rejects a shorter capture when a longer one is mandatory") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::kWMan},
                                          {Sq("a4"), PieceKind::kBMan},
                                          {Sq("b5"), PieceKind::kBMan}}));
  CHECK(ErrorOf(Parse("a3xa5", board)) == ParseError::kIllegal);
}

TEST_CASE("Parser rejects capture syntax when no capture is available") {
  CHECK(ErrorOf(ParseInitial("a3xa5")) == ParseError::kIllegal);
}

TEST_CASE("Parser rejects quiet syntax when a capture is mandatory") {
  Board board;
  board.Restore(MakeState(Color::kWhite, {{Sq("a3"), PieceKind::kWMan},
                                          {Sq("a4"), PieceKind::kBMan}}));
  CHECK(ErrorOf(Parse("a3a4", board)) == ParseError::kIllegal);
}

}  // namespace dama
