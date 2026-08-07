// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/client_profiles.h
 * @brief Per-client streaming profile overrides.
 *
 * Users with mixed clients (TV, phone, PC) often want different streaming
 * settings per device. SolarFlare supports optional per-client profiles
 * keyed by the client device name (the `uniqueid` Moonlight sends on
 * launch). When a profile matches, its overrides are applied on top of the
 * global config for that session only.
 *
 * Profiles are configured via flat keys in sunshine.conf:
 *   client_profile_<name>_max_bitrate = 30000
 *   client_profile_<name>_hevc_mode = 2
 *   client_profile_<name>_av1_mode = 0
 *   client_profile_<name>_latency_mode = safe
 *
 * Only settings that are safe to change per-session are supported: bitrate
 * ceiling and codec preferences. Resolution/framerate are negotiated by the
 * client over RTSP per session and are intentionally not overridden here.
 */
#pragma once

// standard includes
#include <string>
#include <unordered_map>

namespace sunshine::client_profiles {

  /**
   * @brief One per-client profile.
   *
   * All fields are optional overrides: 0 / empty means "use the global
   * config value".
   */
  struct profile_t {
    std::string name;  ///< Client device name this profile applies to.
    int max_bitrate {0};  ///< Override video max_bitrate in kbps (0 = global).
    int hevc_mode {0};  ///< Override video hevc_mode (0 = global).
    int av1_mode {0};  ///< Override video av1_mode (0 = global).
    std::string latency_mode;  ///< Override solarflare.latency_mode (empty = global).
  };

  /**
   * @brief Apply the profile for @p client_name on top of the global config.
   *
   * If a profile with that exact name exists, its non-zero fields are
   * written into config::video / config::solarflare. This mutates the
   * process-global config, so it must be called before the session is
   * allocated and before probe_encoders(); the caller is responsible for
   * restoring global values with reset().
   *
   * @param client_name The client device name.
   */
  void apply(const std::string &client_name);

  /**
   * @brief Restore any global config values that apply() overwrote.
   *
   * Undoes the most recent apply() call so one client's profile does not
   * leak into the next session.
   */
  void reset();

  /**
   * @brief Return the profile for @p client_name, or nullptr when none.
   *
   * @param client_name The client device name.
   * @return Pointer to the stored profile, or nullptr.
   */
  const profile_t *find(const std::string &client_name);

  /**
   * @brief Populate the profiles map from parsed config vars.
   *
   * Called by config.cpp's apply_config() after the fork keys are parsed.
   * Reads every `client_profile_*_<field>` key.
   *
   * @param vars The parsed config variable map.
   */
  void load_from_config(const std::unordered_map<std::string, std::string> &vars);

}  // namespace sunshine::client_profiles
