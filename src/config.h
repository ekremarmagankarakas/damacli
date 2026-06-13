#pragma once
#include <filesystem>

namespace dama {

struct Config {
  bool tui = true;
  bool unicode = true;
  int engine_depth = 3;
  bool play_black = false;
};

// Returns $XDG_CONFIG_HOME/damacli/config.ini (or ~/.config fallback).
std::filesystem::path DefaultConfigPath();

Config LoadConfig(const std::filesystem::path& path);
void SaveConfig(const Config& cfg, const std::filesystem::path& path);

}  // namespace dama
