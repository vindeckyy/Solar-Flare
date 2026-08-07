// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/client_profiles.cpp
 * @brief Implementation of per-client streaming profile overrides.
 */
#include "client_profiles.h"

// standard includes
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// local includes
#include "config.h"
#include "logging.h"

namespace sunshine::client_profiles {

  namespace {

    std::mutex profiles_mutex;  ///< Guards profiles and the undo state.
    std::unordered_map<std::string, profile_t> profiles;  ///< name -> profile.

    // Snapshot of the global config values the last apply() overwrote, so
    // reset() can restore them.
    struct undo_t {
      bool active {false};  ///< True between apply() and reset().
      int max_bitrate {0};
      int hevc_mode {0};
      int av1_mode {0};
      std::string latency_mode;
    } undo;

  }  // namespace

  const profile_t *find(const std::string &client_name) {
    std::lock_guard<std::mutex> lock(profiles_mutex);
    auto it = profiles.find(client_name);
    return it == profiles.end() ? nullptr : &it->second;
  }

  void apply(const std::string &client_name) {
    std::lock_guard<std::mutex> lock(profiles_mutex);

    auto it = profiles.find(client_name);
    if (it == profiles.end()) {
      return;
    }
    const auto &p = it->second;

    // Snapshot current global values for reset().
    undo.active = true;
    undo.max_bitrate = config::video.max_bitrate;
    undo.hevc_mode = config::video.hevc_mode;
    undo.av1_mode = config::video.av1_mode;
    undo.latency_mode = config::solarflare.latency_mode;

    if (p.max_bitrate > 0) {
      config::video.max_bitrate = p.max_bitrate;
    }
    if (p.hevc_mode > 0) {
      config::video.hevc_mode = p.hevc_mode;
    }
    if (p.av1_mode > 0) {
      config::video.av1_mode = p.av1_mode;
    }
    if (p.latency_mode == "safe" || p.latency_mode == "aggressive") {
      config::solarflare.latency_mode = p.latency_mode;
    }

    BOOST_LOG(info) << "client_profiles: applied profile '" << client_name
                    << "' (bitrate=" << p.max_bitrate << ", hevc=" << p.hevc_mode
                    << ", av1=" << p.av1_mode << ", latency=" << p.latency_mode << ')';
  }

  void reset() {
    std::lock_guard<std::mutex> lock(profiles_mutex);
    if (!undo.active) {
      return;
    }
    config::video.max_bitrate = undo.max_bitrate;
    config::video.hevc_mode = undo.hevc_mode;
    config::video.av1_mode = undo.av1_mode;
    config::solarflare.latency_mode = undo.latency_mode;
    undo.active = false;
  }

  /**
   * @brief Populate the profiles map from parsed config vars.
   *
   * Called by config.cpp's apply_config() after the solarflare keys are
   * parsed. Reads every `client_profile_*_<field>` key.
   *
   * @param vars The parsed config variable map.
   */
  void load_from_config(const std::unordered_map<std::string, std::string> &vars) {
    std::lock_guard<std::mutex> lock(profiles_mutex);
    profiles.clear();

    // Index all keys by `client_profile_<name>_<field>`. Field names are
    // known suffixes ("max_bitrate", "hevc_mode", "av1_mode",
    // "latency_mode"); the client name is everything between the prefix
    // and the matched suffix, so names may themselves contain underscores.
    static const std::vector<std::string_view> field_names {
      "max_bitrate",
      "hevc_mode",
      "av1_mode",
      "latency_mode",
    };

    std::unordered_map<std::string, profile_t> building;
    for (auto &[key, value] : vars) {
      constexpr std::string_view prefix = "client_profile_";
      if (!key.starts_with(prefix)) {
        continue;
      }
      auto rest = std::string_view(key).substr(prefix.size());

      std::string_view field;
      for (auto candidate : field_names) {
        if (rest.size() > candidate.size() + 1 &&
            rest.substr(rest.size() - candidate.size()) == candidate &&
            rest[rest.size() - candidate.size() - 1] == '_') {
          field = candidate;
          break;
        }
      }
      if (field.empty()) {
        continue;
      }

      auto name = std::string(rest.substr(0, rest.size() - field.size() - 1));
      auto &p = building[name];
      p.name = name;

      try {
        if (field == "max_bitrate") {
          p.max_bitrate = std::stoi(value);
        }
        else if (field == "hevc_mode") {
          p.hevc_mode = std::stoi(value);
        }
        else if (field == "av1_mode") {
          p.av1_mode = std::stoi(value);
        }
        else if (field == "latency_mode") {
          p.latency_mode = value;
        }
      } catch (const std::exception &) {
        BOOST_LOG(warning) << "client_profiles: invalid value for '" << key << "': " << value;
      }
    }

    profiles = std::move(building);
    if (!profiles.empty()) {
      BOOST_LOG(info) << "client_profiles: loaded " << profiles.size() << " profile(s)";
    }
  }

}  // namespace sunshine::client_profiles
