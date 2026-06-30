/**
 * @file src/spice/latency_budget.h
 * @brief Per-app latency budget enforcer for the Spice fork.
 *
 * A latency budget is the maximum rolling-median input-to-photon
 * latency the host will tolerate for a given app. When the rolling
 * median (last 30 samples) exceeds the budget, the configured action
 * (warn|abort) is taken. The bulk of the heuristics live in
 * latency_budget.cpp; this header is the public API.
 *
 * Design notes:
 *
 *  - Samples come from the telemetry pipeline (see telemetry.h).
 *  - Budget lookup is O(1) by exact app name match, with fallback to
 *    the global spice.latency_budget_ms.
 *  - The "warn" action is non-destructive: a single log line per
 *    breach spaced at >= 5 s. The "abort" action posts a non-blocking
 *    session-tear-down signal that the host session loop drains on
 *    its next tick.
 *
 * @copyright 2026 Hayden. Licensed under GPL-3.0.
 */
#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace spice {

  /**
   * @brief Outcome returned by @c evaluate().
   *
   * @var within_budget  True if median <= budget (after evaluation).
   * @var median_us      Median of the last 30 samples, in microseconds.
   * @var budget_us      The budget used (resolved per-app or global).
   * @var action_taken   "none", "warn", or "abort".
   */
  struct budget_eval_t {
    bool within_budget = true;
    std::uint64_t median_us = 0;
    std::uint64_t budget_us = 0;
    std::string action_taken;  ///< one of "none", "warn", "abort"
  };

  /**
   * @brief Resolve the effective budget for an app.
   *
   * @param app_name_or_uuid App name (e.g. "Desktop") or Moonlight
   *                         UUID. Empty string means "global only".
   * @return Budget in microseconds. Always >= 16000 (16 ms floor);
   *         clamped to config::spice.latency_budget_ms when no
   *         per-app override exists.
   */
  std::uint64_t resolve_budget_us(const std::string &app_name_or_uuid);

  /**
   * @brief Evaluate the most recent N samples vs the app budget.
   *
   * @param app_name_or_uuid App to evaluate against.
   * @param recent_encode_us Recent encode durations, microseconds.
   *                         The rolling median is taken over the
   *                         last @c 30 entries (or @c recent_encode_us.size()
   *                         if smaller).
   * @return Compute-and-act result.
   */
  budget_eval_t evaluate(const std::string &app_name_or_uuid,
    const std::vector<std::uint64_t> &recent_encode_us);

  /**
   * @brief Clear the rolling window for an app (called on session end).
   */
  void clear_window(const std::string &app_name_or_uuid);

  /// Currently configured global budget (ms, post-validation).
  int global_budget_ms();

  /// Currently configured action ("warn" or "abort").
  const std::string &action();

}  // namespace spice
