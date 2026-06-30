/**
 * @file src/spice/bitrate_ladder.h
 * @brief Adaptive bitrate ladder for the Spice fork.
 *
 * The bitrate ladder solves a problem Sunlight's upstream defaults
 * leave to the user: "what bitrate should I encode at for *this*
 * game, on *this* network, on *this* rig?". Upstream exposes a
 * static `video.bitrate` slider and the user is on their own.
 *
 * Spice instead maintains a ladder of bitrate "rungs" and walks up
 * or down one rung at a time based on a fast feedback signal:
 * encode duration vs. target frame budget. The ladder:
 *
 *   - Starts at the rung closest to `config::video.bitrate`.
 *   - Tracks an EWMA of encode duration (microseconds per frame).
 *   - Steps DOWN a rung when the EWMA exceeds the budget by
 *     `spice2.bitrate_drop_pct` % for `spice2.bitrate_drop_window`
 *     consecutive windows.
 *   - Steps UP a rung when the EWMA is comfortably below the budget
 *     AND no frames were dropped for `spice2.bitrate_up_hold_s`
 *     seconds.
 *   - Records any ladder transition in `telemetry.h`'s ring buffer
 *     (frame_index 0, dropped=true) so the dashboard shows it.
 *
 * The implementation is pure logic; it does not touch the encoder
 * directly. The encoder calls into the ladder via:
 *
 *   auto next = spice::bitrate_ladder_tick(current_kbps, encode_us, dropped_now);
 *   if (next.changed) { encoder.set_bitrate(next.kbps); }
 *
 * The current rung is stored in a process-global so the REST
 * handler at /api/spice/bitrate can read it back at any time.
 *
 * @copyright 2026 Hayden. Licensed under GPL-3.0.
 */
#pragma once

// standard includes
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace spice {

  /**
   * @brief One rung on the ladder.
   *
   * @var kbps        Bitrate in kilobits per second.
   * @var label       Short display name (e.g. "5 Mbps / 720p60").
   */
  struct ladder_rung_t {
    int kbps = 0;
    std::string label;
  };

  /**
   * @brief Result of one tick.
   *
   * @var kbps      New bitrate after the tick (== input kbps if unchanged).
   * @var changed   True if the ladder moved this tick.
   * @var direction "up", "down", or "stay".
   * @var reason    Human-readable explanation (shown in logs + telemetry).
   * @var from_kbps Previous bitrate (== input kbps if changed is false).
   */
  struct ladder_tick_t {
    int kbps = 0;
    bool changed = false;
    std::string direction;  ///< up | down | stay
    std::string reason;
    int from_kbps = 0;
  };

  /// Built-in default ladder. Six rungs covering 4K60 down to 480p30.
  /// Pick the closest rung >= the user's chosen bitrate when starting.
  std::vector<ladder_rung_t> default_ladder();

  /// Set the ladder to a custom sequence. Empty resets to defaults.
  /// Stored by-value; safe to call from any thread (after the first).
  void set_ladder(std::vector<ladder_rung_t> rungs);

  /// Currently active ladder (read-only).
  const std::vector<ladder_rung_t> &ladder();

  /// Pick the rung >= target_kbps; if none, pick the largest.
  /// Returns 0-indexed position in the ladder.
  std::size_t initial_rung(int target_kbps);

  /**
   * @brief Advance the ladder by one tick. Idempotent and lock-free
   *        on the hot path (single std::atomic reflects the current
   *        rung index).
   *
   * @param current_kbps Bitrate the encoder is currently using.
   * @param encode_us    Microseconds the most recent frame took to
   *                     encode.
   * @param dropped      True if the most recent frame was dropped.
   * @return Tick result.
   */
  ladder_tick_t bitrate_ladder_tick(int current_kbps,
    std::uint64_t encode_us, bool dropped);

  /// Force-reset the EWMA, the rune index, and the drop/up-streak
  /// counters. Used on session start so a new stream doesn't inherit
  /// the prior ladder state.
  void bitrate_ladder_reset();

  /// Current rung index (0-based).
  std::size_t current_rung_index();

  /// Diagnostic snapshot (EWMA, up-streak ticks, drop-streak ticks).
  struct ladder_state_t {
    int current_kbps = 0;
    std::size_t index = 0;
    std::size_t rung_count = 0;
    double ewma_us = 0.0;
    int down_streak = 0;
    int up_hold_ticks = 0;
    int dropped_recent = 0;
  };
  ladder_state_t bitrate_ladder_state();

}  // namespace spice
