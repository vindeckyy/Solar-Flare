/**
 * @file src/spice/per_client_input.h
 * @brief Per-client mouse DPI / scroll calibration.
 *
 * Upstream Sunshine treats every connected Moonlight client the same:
 * absolute pointer events from a 1600 DPI mouse and a 65 dpi touchpad
 * both look the same to the game. On a real couch setup where you have
 * a Mac trackpad client, a Windows mouse client, and a Steam Deck
 * client, that means each device's "natural" feel is wrong.
 *
 * Spice allows per-client adjustments:
 *
 *   - **Mouse DPI ratio**: a multiplier (default 1.0) applied to
 *     relative-motion events before forwarding to the game. Macs and
 *     some trackpads feel sluggish if you forward 1:1; bumping to 1.5
 *     or 2.0 gives the game the velocity it expects.
 *
 *   - **Scroll line divisor**: an integer (default 1) applied to
 *     wheel-delta. Some clients emit one wheel tick per scroll-stop
 *     (Linux X11, Wayland) and others emit fractional deltas; divide
 *     by the divisor to normalize.
 *
 *   - **Pointer inversion**: optional, useful when a flipped
 *     presentation is in use.
 *
 * The settings live in `config::spice2.per_client_calibration` (a map
 * keyed by Moonlight UUID).
 *
 * @copyright 2026 Hayden. Licensed under GPL-3.0.
 */
#pragma once

// standard includes
#include <string>
#include <unordered_map>

namespace per_client_input {

  /// Per-client calibration.
  struct input_calibration_t {
    double mouse_dpi_ratio = 1.0;        ///< 0.25 .. 4.0
    int scroll_divisor = 1;              ///< 1..16
    bool invert_pointer_x = false;
    bool invert_pointer_y = false;
    bool swap_buttons = false;           ///< right-handed vs left-handed
    int deadzone_px = 0;                 ///< ignore sub-pixel jitter
  };

  /// Apply calibration to a relative motion (dx, dy), returning adjusted (dx', dy').
  struct adjusted_motion_t {
    double dx;
    double dy;
  };
  adjusted_motion_t apply_motion(const input_calibration_t &c, double dx, double dy);

  /// Apply calibration to a wheel delta, returning an adjusted integer delta.
  int apply_wheel(const input_calibration_t &c, int wheel_delta);

  // Storage --------------------------------------------------------------
  // The map is keyed by the Moonlight UUID (lowercase, hyphenated).
  void set_calibration(const std::string &uuid, const input_calibration_t &c);
  void clear_calibration(const std::string &uuid);
  input_calibration_t get_calibration(const std::string &uuid);  // returns defaults if absent
  bool has_calibration(const std::string &uuid);
  std::unordered_map<std::string, input_calibration_t> all_calibrations();

  /// Validation helper: clamp ratios / divisors to sane ranges.
  input_calibration_t sanitize(input_calibration_t c);

  /// Free-form human-readable name for a calibration, e.g. for the dashboard.
  std::string describe(const input_calibration_t &c);

}  // namespace per_client_input
