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

using namespace std::literals;

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

  /**
   * @brief Find a profile by client name.
   * @param client_name Client device name.
   * @return Pointer to the profile, or nullptr when not found or on empty name.
   */
  const profile_t *find(const std::string &client_name) {
    if (client_name.empty()) {
      return nullptr;
    }
    std::lock_guard<std::mutex> lock(profiles_mutex);
    auto it = profiles.find(client_name);
    return it == profiles.end() ? nullptr : &it->second;
  }

  /**
   * @brief Apply the profile for a client on top of the global config.
   * @param client_name Client device name.
   */
  void apply(const std::string &client_name) {
    if (client_name.empty()) {
      BOOST_LOG(debug) << "client_profiles: apply called with empty client_name"sv;
      return;
    }
    std::lock_guard<std::mutex> lock(profiles_mutex);

    auto it = profiles.find(client_name);
    if (it == profiles.end()) {
      BOOST_LOG(debug) << "client_profiles: no profile for '"sv << client_name << "'";
      return;
    }
    const auto &p = it->second;

    // Validate profile fields before applying. Invalid bitrate (negative or
    // > BITRATE_MAX_KBPS) is skipped with a warning rather than clobbering
    // the global with a degenerate value.
    if (p.max_bitrate < 0 || p.max_bitrate > config::BITRATE_MAX_KBPS) {
      BOOST_LOG(warning) << "client_profiles: profile '"sv << client_name << "' has invalid max_bitrate "sv << p.max_bitrate << "; skipping bitrate override"sv;
    }
    if ((p.hevc_mode < 0 || p.hevc_mode > 3) && p.hevc_mode != 0) {
      BOOST_LOG(warning) << "client_profiles: profile '"sv << client_name << "' has invalid hevc_mode "sv << p.hevc_mode;
    }
    if ((p.av1_mode < 0 || p.av1_mode > 3) && p.av1_mode != 0) {
      BOOST_LOG(warning) << "client_profiles: profile '"sv << client_name << "' has invalid av1_mode "sv << p.av1_mode;
    }

    // Snapshot current global values for reset().
    undo.active = true;
    undo.max_bitrate = config::video.max_bitrate;
    undo.hevc_mode = config::video.hevc_mode;
    undo.av1_mode = config::video.av1_mode;
    undo.latency_mode = config::solarflare.latency_mode;

    if (p.max_bitrate > 0 && p.max_bitrate <= config::BITRATE_MAX_KBPS) {
      config::video.max_bitrate = p.max_bitrate;
    }
    if (p.hevc_mode > 0 && p.hevc_mode <= 3) {
      config::video.hevc_mode = p.hevc_mode;
    }
    if (p.av1_mode > 0 && p.av1_mode <= 3) {
      config::video.av1_mode = p.av1_mode;
    }
    if (p.latency_mode == "safe" || p.latency_mode == "aggressive") {
      config::solarflare.latency_mode = p.latency_mode;
    } else if (!p.latency_mode.empty()) {
      BOOST_LOG(warning) << "client_profiles: profile '"sv << client_name << "' has invalid latency_mode '"sv << p.latency_mode << "'";
    }

    BOOST_LOG(info) << "client_profiles: applied profile '" << client_name
                    << "' (bitrate=" << p.max_bitrate << ", hevc=" << p.hevc_mode
                    << ", av1=" << p.av1_mode << ", latency=" << p.latency_mode << ')';
  }

  /**
   * @brief Restore any global config values that apply() overwrote.
   */
  void reset() {
    std::lock_guard<std::mutex> lock(profiles_mutex);
    if (!undo.active) {
      BOOST_LOG(debug) << "client_profiles: reset called with no active profile"sv;
      return;
    }
    config::video.max_bitrate = undo.max_bitrate;
    config::video.hevc_mode = undo.hevc_mode;
    config::video.av1_mode = undo.av1_mode;
    config::solarflare.latency_mode = undo.latency_mode;
    undo.active = false;
    BOOST_LOG(debug) << "client_profiles: reset to global values"sv;
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
      if (name.empty()) {
        BOOST_LOG(warning) << "client_profiles: empty profile name in key '"sv << key << "'";
        continue;
      }
      auto &p = building[name];
      p.name = name;

      try {
        if (field == "max_bitrate") {
          auto v = std::stoi(value);
          if (v < 0 || v > config::BITRATE_MAX_KBPS) {
            BOOST_LOG(warning) << "client_profiles: max_bitrate out of range for '"sv << key << "': "sv << value;
          } else {
            p.max_bitrate = v;
          }
        } else if (field == "hevc_mode") {
          auto v = std::stoi(value);
          if (v < 0 || v > 3) {
            BOOST_LOG(warning) << "client_profiles: hevc_mode out of range for '"sv << key << "': "sv << value;
          } else {
            p.hevc_mode = v;
          }
        } else if (field == "av1_mode") {
          auto v = std::stoi(value);
          if (v < 0 || v > 3) {
            BOOST_LOG(warning) << "client_profiles: av1_mode out of range for '"sv << key << "': "sv << value;
          } else {
            p.av1_mode = v;
          }
        } else if (field == "latency_mode") {
          if (value != "safe" && value != "aggressive") {
            BOOST_LOG(warning) << "client_profiles: invalid latency_mode for '"sv << key << "': "sv << value;
          } else {
            p.latency_mode = value;
          }
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
