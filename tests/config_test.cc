#include "config.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace dama {

namespace {

// Each test gets its own scratch dir under the OS temp directory. Avoids
// stomping on the user's real ~/.config/damacli.
std::filesystem::path FreshTempDir(const std::string& tag) {
  auto dir =
      std::filesystem::temp_directory_path() / ("damacli_config_test_" + tag);
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

void WriteFile(const std::filesystem::path& path, const std::string& body) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream f(path);
  f << body;
}

}  // namespace

TEST_CASE("LoadConfig returns defaults when the file is missing") {
  auto path = FreshTempDir("missing") / "nope.ini";
  Config cfg = LoadConfig(path);
  CHECK(cfg.tui == true);
  CHECK(cfg.unicode == true);
  CHECK(cfg.engine_depth == 3);
  CHECK(cfg.play_black == false);
}

TEST_CASE(
    "LoadConfig parses values and tolerates comments/sections/whitespace") {
  auto path = FreshTempDir("parse") / "config.ini";
  WriteFile(path,
            "# leading comment\n"
            "; semicolon comment\n"
            "[ignored-section]\n"
            "  tui = false  \n"
            "unicode=0\n"
            "engine_depth =  5\n"
            "play_black=true\n"
            "garbage-line-with-no-equals\n");
  Config cfg = LoadConfig(path);
  CHECK(cfg.tui == false);
  CHECK(cfg.unicode == false);
  CHECK(cfg.engine_depth == 5);
  CHECK(cfg.play_black == true);
}

TEST_CASE("LoadConfig keeps defaults for unrecognized or unparsable values") {
  auto path = FreshTempDir("bad") / "config.ini";
  WriteFile(path,
            "engine_depth=not-an-int\n"
            "unknown_key=42\n");
  Config cfg = LoadConfig(path);
  CHECK(cfg.engine_depth == 3);
}

TEST_CASE("LoadConfig accepts engine_depth=0 (engine disabled)") {
  auto path = FreshTempDir("disabled") / "config.ini";
  WriteFile(path, "engine_depth=0\n");
  Config cfg = LoadConfig(path);
  CHECK(cfg.engine_depth == 0);
}

TEST_CASE("SaveConfig + LoadConfig is a round trip") {
  auto path = FreshTempDir("roundtrip") / "nested" / "config.ini";
  Config in;
  in.tui = false;
  in.unicode = false;
  in.engine_depth = 7;
  in.play_black = true;
  SaveConfig(in, path);

  REQUIRE(std::filesystem::exists(path));
  Config out = LoadConfig(path);
  CHECK(out.tui == in.tui);
  CHECK(out.unicode == in.unicode);
  CHECK(out.engine_depth == in.engine_depth);
  CHECK(out.play_black == in.play_black);
}

TEST_CASE("DefaultConfigPath honors XDG_CONFIG_HOME when set") {
  const char* old_xdg = std::getenv("XDG_CONFIG_HOME");
  ::setenv("XDG_CONFIG_HOME", "/tmp/damacli-xdg-test", /*overwrite=*/1);
  auto path = DefaultConfigPath();
  CHECK(path == std::filesystem::path("/tmp/damacli-xdg-test/damacli/"
                                      "config.ini"));
  if (old_xdg) {
    ::setenv("XDG_CONFIG_HOME", old_xdg, /*overwrite=*/1);
  } else {
    ::unsetenv("XDG_CONFIG_HOME");
  }
}

}  // namespace dama
