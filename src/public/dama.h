#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace dama {

enum class Side { White, Black };
enum class ViewMode { Unicode, Ascii };
enum class Result { Ongoing, WhiteWins, BlackWins, Draw };

struct Config {
  ViewMode view = ViewMode::Unicode;
  int engine_depth = 3;            // 0 disables engine (two-player)
  Side engine_side = Side::Black;
};

struct CommandResult {
  bool ok = true;
  std::string message;             // human feedback, may be empty
};

class Session {
 public:
  explicit Session(Config cfg);
  ~Session();
  Session(Session&&) noexcept;
  Session& operator=(Session&&) noexcept;
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;

  // Board as a string the caller can print or stuff into a TUI widget.
  // Includes the "<color> to move" line at the bottom.
  std::string Render() const;

  // Apply a user command exactly as the existing CLI parser accepts:
  //   moves: source/destination square notation per the existing parser
  //   meta:  "undo", "reset", "history", "resign"
  // If an engine is configured and the move leaves the engine to move,
  // the engine plays automatically inside this call.
  CommandResult Apply(std::string_view command);

  bool IsOver() const;
  Result Outcome() const;
  Side ToMove() const;
  std::vector<std::string> History() const;     // newest last

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace dama
