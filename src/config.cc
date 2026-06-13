#include "config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace dama {

namespace {

std::string Trim(const std::string& s) {
  const auto* ws = " \t\r\n";
  auto b = s.find_first_not_of(ws);
  if (b == std::string::npos) {
    return {};
  }
  auto e = s.find_last_not_of(ws);
  return s.substr(b, e - b + 1);
}

}  // namespace

std::filesystem::path DefaultConfigPath() {
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  std::filesystem::path base;
  if (xdg && xdg[0] != '\0') {
    base = xdg;
  } else {
    const char* home = std::getenv("HOME");
    base = home ? std::filesystem::path(home) / ".config"
                : std::filesystem::current_path();
  }
  return base / "damacli" / "config.ini";
}

Config LoadConfig(const std::filesystem::path& path) {
  Config cfg;
  std::ifstream f(path);
  if (!f.is_open()) {
    return cfg;
  }
  std::string line;
  while (std::getline(f, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
      continue;
    }
    auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    std::string key = Trim(line.substr(0, eq));
    std::string val = Trim(line.substr(eq + 1));
    if (key == "tui") {
      cfg.tui = (val == "true" || val == "1");
    } else if (key == "unicode") {
      cfg.unicode = (val == "true" || val == "1");
    } else if (key == "engine_depth") {
      try {
        int d = std::stoi(val);
        if (d >= 0) {
          cfg.engine_depth = d;
        }
      } catch (const std::exception&) {
      }
    } else if (key == "play_black") {
      cfg.play_black = (val == "true" || val == "1");
    }
  }
  return cfg;
}

void SaveConfig(const Config& cfg, const std::filesystem::path& path) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream f(path);
  f << "# damacli configuration\n"
    << "tui=" << (cfg.tui ? "true" : "false") << "\n"
    << "unicode=" << (cfg.unicode ? "true" : "false") << "\n"
    << "engine_depth=" << cfg.engine_depth << "\n"
    << "play_black=" << (cfg.play_black ? "true" : "false") << "\n";
}

}  // namespace dama
