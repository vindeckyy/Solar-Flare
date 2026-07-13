// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file game_scanner.h
 * @brief Declarations for scanning installed games from Steam, Lutris, and Heroic.
 */
#pragma once

#include <string>
#include <vector>

namespace game_scanner {
  /**
   * @brief Represents a discovered game entry.
   */
  struct GameEntry {
    std::string name;  ///< The display name of the game.
    std::string path;  ///< The filesystem path to the game or its launcher entry.
    std::string launcher;  ///< The source launcher: "steam", "lutris", or "heroic".
    std::string cover_url;  ///< URL for the game's cover art image.
  };

  /**
   * @brief Scan Steam for installed games.
   * @return Vector of discovered games.
   */
  std::vector<GameEntry> scan_steam();

  /**
   * @brief Scan Lutris for installed games.
   * @return Vector of discovered games.
   */
  std::vector<GameEntry> scan_lutris();

  /**
   * @brief Scan Heroic Games Launcher for installed games.
   * @return Vector of discovered games.
   */
  std::vector<GameEntry> scan_heroic();

  /**
   * @brief Scan all supported launchers for installed games.
   * @return Merged vector of discovered games from all launchers.
   */
  std::vector<GameEntry> scan_all();
}  // namespace game_scanner
