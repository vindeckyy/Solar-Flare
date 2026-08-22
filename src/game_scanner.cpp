// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file game_scanner.cpp
 * @brief Implementation of game scanners for Steam, Lutris, and Heroic.
 */
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

// lib includes
#include <nlohmann/json.hpp>

// local includes
#include "game_scanner.h"
#include "logging.h"

using namespace std::literals;

namespace game_scanner {

  namespace fs = std::filesystem;

  /**
   * @brief Parse a VDF key-value line, extracting the key and value as quoted strings.
   * @param line A line from a VDF file.
   * @return A pair of {key, value}. Both empty if the line has fewer than two quoted strings.
   */
  static std::pair<std::string, std::string> parse_vdf_kv_line(const std::string &line) {
    auto q1 = line.find('"');
    if (q1 == std::string::npos) return {};
    auto q2 = line.find('"', q1 + 1);
    if (q2 == std::string::npos) return {};

    std::string key = line.substr(q1 + 1, q2 - q1 - 1);

    auto q3 = line.find('"', q2 + 1);
    if (q3 == std::string::npos) return {};
    auto q4 = line.find('"', q3 + 1);
    if (q4 == std::string::npos) return {};

    std::string value = line.substr(q3 + 1, q4 - q3 - 1);
    return {key, value};
  }

  /**
   * @brief Get the user's home directory path.
   * @return The home directory path, or an empty path on failure.
   */
  static fs::path get_home() {
#ifdef _WIN32
    const char *home = std::getenv("USERPROFILE");
#else
    const char *home = std::getenv("HOME");
#endif
    if (!home) return {};
    return fs::path(home);
  }

  std::vector<GameEntry> scan_steam() {
    std::vector<GameEntry> games;

    auto home = get_home();
    if (home.empty()) return games;

    fs::path steam_data;
    const std::vector<fs::path> candidate_dirs = {
      home / ".steam" / "steam",
      home / ".local" / "share" / "Steam",
      // Flatpak Steam installs to ~/.var/app/com.valvesoftware.Steam and
      // symlinks its data tree under data/Steam. Most Linux gamers in
      // 2026 install via Flatpak (CachyOS, Bazzite, SteamOS, etc.) so
      // omitting this path silently hides their entire library.
      home / ".var" / "app" / "com.valvesoftware.Steam" / "data" / "Steam",
      // Snap Steam lives under ~/snap/steam/common/.steam/steam.
      home / "snap" / "steam" / "common" / ".steam" / "steam",
#ifdef _WIN32
      fs::path("C:\\Program Files (x86)\\Steam")
#endif
    };

    for (const auto &dir : candidate_dirs) {
      if (fs::exists(dir / "steamapps" / "libraryfolders.vdf")) {
        steam_data = dir;
        break;
      }
    }
    if (steam_data.empty()) return games;

    auto vdf_path = steam_data / "steamapps" / "libraryfolders.vdf";

    std::vector<fs::path> libraries;
    {
      std::ifstream vdf(vdf_path);
      if (vdf.is_open()) {
        std::string line;
        while (std::getline(vdf, line)) {
          auto [key, value] = parse_vdf_kv_line(line);
          if (key == "path" && !value.empty()) {
            libraries.push_back(fs::path(value) / "steamapps");
          }
        }
      }
    }

    // Always include the default steamapps directory
    libraries.push_back(steam_data / "steamapps");

    for (const auto &lib : libraries) {
      if (!fs::exists(lib)) continue;

      std::error_code ec;
      for (const auto &entry : fs::directory_iterator(lib, ec)) {
        if (ec) break;
        if (entry.path().extension() != ".acf") continue;

        std::string name, appid;
        {
          std::ifstream acf(entry.path());
          if (!acf.is_open()) continue;

          std::string acf_line;
          while (std::getline(acf, acf_line)) {
            auto [k, v] = parse_vdf_kv_line(acf_line);
            if (k == "name" && !v.empty()) name = v;
            else if (k == "appid" && !v.empty()) appid = v;

            if (!name.empty() && !appid.empty()) break;
          }
        }

        if (!name.empty()) {
          GameEntry g;
          g.name = name;
          g.path = lib.string();
          g.launcher = "steam";
          if (!appid.empty()) {
            g.cover_url = std::format("https://steamcdn-a.akamaihd.net/steam/apps/{}/header.jpg", appid);
          }
          games.push_back(std::move(g));
        }
      }
    }

    return games;
  }

  std::vector<GameEntry> scan_lutris() {
    std::vector<GameEntry> games;

    auto home = get_home();
    if (home.empty()) return games;

    std::vector<fs::path> lutris_dirs = {
      home / ".local" / "share" / "lutris" / "games",
      home / ".config" / "lutris" / "games",
    };

    for (const auto &dir : lutris_dirs) {
      if (!fs::exists(dir)) continue;

      std::error_code ec;
      for (const auto &entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (entry.path().extension() != ".yml") continue;

        std::string name, slug;
        {
          std::ifstream yml(entry.path());
          if (!yml.is_open()) continue;

          std::string line;
          while (std::getline(yml, line)) {
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;

            std::string trimmed = line.substr(start);
            auto colon = trimmed.find(": ");
            if (colon == std::string::npos) continue;

            std::string key = trimmed.substr(0, colon);
            std::string value = trimmed.substr(colon + 2);

            if (key == "name" && !value.empty()) name = value;
            else if (key == "slug" && !value.empty()) slug = value;

            if (!name.empty() && !slug.empty()) break;
          }
        }

        if (!name.empty()) {
          GameEntry g;
          g.name = name;
          // Lutris games are launched via `lutris lutris:<slug>` rather
          // than by exec'ing the .yml config file. Store the launcher
          // invocation in `path` so UI consumers (which typically expect
          // an executable command) can paste it directly into a prep cmd
          // or use it as the apps.cmd field without further translation.
          g.path = !slug.empty() ? std::format("lutris lutris:{}", slug)
                                 : entry.path().string();
          g.launcher = "lutris";
          if (!slug.empty()) {
            g.cover_url = std::format("https://lutris.net/games/banner/{}.jpg", slug);
          }
          games.push_back(std::move(g));
        }
      }
    }

    return games;
  }

  std::vector<GameEntry> scan_heroic() {
    std::vector<GameEntry> games;

    auto home = get_home();
    if (home.empty()) return games;

    auto heroic_dir = home / ".config" / "heroic";
    if (!fs::exists(heroic_dir)) return games;

    const std::vector<fs::path> json_candidates = {
      heroic_dir / "gog_store" / "installed.json",
      heroic_dir / "legendaryConfig" / "installed.json",
      heroic_dir / "gog_store" / "library.json",
      heroic_dir / "legendaryConfig" / "library.json",
      heroic_dir / "sideload_apps.json",
    };

    for (const auto &json_path : json_candidates) {
      if (!fs::exists(json_path)) continue;

      try {
        std::ifstream f(json_path);
        if (!f.is_open()) continue;

        nlohmann::json data = nlohmann::json::parse(f);

        auto process_entry = [&](const nlohmann::json &entry) {
          std::string name;
          if (entry.contains("title")) name = entry["title"].get<std::string>();
          else if (entry.contains("app_title")) name = entry["app_title"].get<std::string>();
          else if (entry.contains("name")) name = entry["name"].get<std::string>();
          if (name.empty()) return;

          GameEntry g;
          g.name = name;
          g.launcher = "heroic";

          if (entry.contains("install_path")) {
            g.path = entry["install_path"].get<std::string>();
          } else if (entry.contains("install_folder")) {
            g.path = entry["install_folder"].get<std::string>();
          }

          if (entry.contains("art_cover")) g.cover_url = entry["art_cover"].get<std::string>();
          else if (entry.contains("art_square")) g.cover_url = entry["art_square"].get<std::string>();

          games.push_back(std::move(g));
        };

        if (data.is_array()) {
          for (const auto &entry : data) process_entry(entry);
        } else if (data.is_object()) {
          // Some Heroic files have a "games" or "library" key containing the array
          bool found = false;
          for (const auto &k : {"library", "games"}) {
            if (data.contains(k) && data[k].is_array()) {
              for (const auto &entry : data[k]) process_entry(entry);
              found = true;
            }
          }
          if (!found) {
            // Try iterating the object values as game entries
            for (const auto &[key, entry] : data.items()) {
              if (entry.is_object()) process_entry(entry);
            }
          }
        }
      } catch (const std::exception &e) {
        BOOST_LOG(warning) << "game_scanner: failed to parse Heroic file "sv << json_path.string() << ": "sv << e.what();
      }
    }

    return games;
  }

  /**
   * @brief Scan all supported launchers and deduplicate by name.
   * @return Merged vector of discovered games from all launchers.
   */
  std::vector<GameEntry> scan_all() {
    std::vector<GameEntry> games;

    try {
      auto steam = scan_steam();
      games.insert(games.end(),
                   std::make_move_iterator(steam.begin()),
                   std::make_move_iterator(steam.end()));
    } catch (const std::exception &e) {
      BOOST_LOG(warning) << "game_scanner: scan_steam failed: "sv << e.what();
    }
    try {
      auto lutris = scan_lutris();
      games.insert(games.end(),
                   std::make_move_iterator(lutris.begin()),
                   std::make_move_iterator(lutris.end()));
    } catch (const std::exception &e) {
      BOOST_LOG(warning) << "game_scanner: scan_lutris failed: "sv << e.what();
    }
    try {
      auto heroic = scan_heroic();
      games.insert(games.end(),
                   std::make_move_iterator(heroic.begin()),
                   std::make_move_iterator(heroic.end()));
    } catch (const std::exception &e) {
      BOOST_LOG(warning) << "game_scanner: scan_heroic failed: "sv << e.what();
    }

    if (games.empty()) {
      BOOST_LOG(debug) << "game_scanner: no games found"sv;
    }

    // Deduplicate by case-sensitive name+launcher, keeping first occurrence and preserving insertion order (Steam, Lutris, Heroic).
    std::unordered_set<std::string> seen;
    std::vector<GameEntry> deduped;
    deduped.reserve(games.size());
    for (auto &g : games) {
      std::string key = g.name + '\0' + g.launcher;
      if (seen.insert(key).second) {
        deduped.push_back(std::move(g));
      }
    }
    games = std::move(deduped);

    return games;
  }

}  // namespace game_scanner
