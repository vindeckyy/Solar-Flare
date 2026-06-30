/**
 * @file src/spice/bitrate_ladder.cpp
 * @brief Implementation of the adaptive bitrate ladder.
 */
#include "bitrate_ladder.h"

// standard includes
#include <algorithm>
#include <atomic>
#include <mutex>

// project includes
#include "config.h"

using namespace std::string_view_literals;
using namespace std::chrono_literals;

namespace spice {

  namespace {

    std::vector<ladder_rung_t> g_ladder = default_ladder();
    std::mutex g_ladder_mu;

    struct state_t {
      std::atomic<std::size_t> index {0};
      std::atomic<int> ewma_q8 {0};  // EWMA in Q8.8 fixed point (microseconds)
      std::atomic<int> down_streak {0};
      std::atomic<int> up_hold_ticks {0};
      std::atomic<int> dropped_recent {0};
    } g_state;

    // Ladder tunables (read from config on each tick; cheap).
    int drop_pct() { return std::clamp<int>(config::bitrate_ladder.drop_pct, 1, 90); }
    int drop_window() { return std::clamp<int>(config::bitrate_ladder.drop_window, 1, 120); }
    int up_hold_s() { return std::clamp<int>(config::bitrate_ladder.up_hold_s, 1, 600); }

    // Target budget microseconds = 1_000_000 / target_fps.
    // We don't have the target FPS at this layer, so we approximate
    // with the user's configured bitrate and 60 fps: each frame is
    // 16.67 ms by default. The cap can be overridden by the caller
    // via the encoder hook.
    int budget_us() {
      // The streaming FPS is conventionally 60; we expose the same
      // tunable via config so the dashboard can hint at it.
      int fps = std::clamp<int>(config::bitrate_ladder.target_fps, 24, 144);
      return std::max(1, 1'000'000 / fps);  // microseconds per frame
    }

    void ewma_update(std::uint64_t x_us) {
      // EWMA in Q8.8 fixed point with alpha=1/8 (cheap shift-and-add).
      int q8 = static_cast<int>(x_us << 8);
      int prev = g_state.ewma_q8.load(std::memory_order_relaxed);
      // First sample: take it as-is.
      if (prev == 0) {
        g_state.ewma_q8.store(q8, std::memory_order_relaxed);
        return;
      }
      int next = prev + ((q8 - prev) >> 3);
      g_state.ewma_q8.store(next, std::memory_order_relaxed);
    }

  }  // namespace

  std::vector<ladder_rung_t> default_ladder() {
    return {
      {4000,  "4 Mbps / 480p30"},
      {6000,  "6 Mbps / 720p30"},
      {10000, "10 Mbps / 720p60"},
      {14000, "14 Mbps / 1080p60"},
      {20000, "20 Mbps / 1440p60"},
      {30000, "30 Mbps / 4K30"},
      {45000, "45 Mbps / 4K60"},
    };
  }

  void set_ladder(std::vector<ladder_rung_t> rungs) {
    std::lock_guard<std::mutex> lk(g_ladder_mu);
    if (rungs.empty()) {
      g_ladder = default_ladder();
    } else {
      std::sort(rungs.begin(), rungs.end(),
        [](const ladder_rung_t &a, const ladder_rung_t &b) { return a.kbps < b.kbps; });
      g_ladder = std::move(rungs);
    }
    g_state.index.store(0, std::memory_order_relaxed);
  }

  const std::vector<ladder_rung_t> &ladder() {
    return g_ladder;
  }

  std::size_t initial_rung(int target_kbps) {
    std::lock_guard<std::mutex> lk(g_ladder_mu);
    if (g_ladder.empty()) {
      return 0;
    }
    auto it = std::lower_bound(g_ladder.begin(), g_ladder.end(), target_kbps,
      [](const ladder_rung_t &r, int v) { return r.kbps < v; });
    if (it == g_ladder.end()) {
      return g_ladder.size() - 1;
    }
    return static_cast<std::size_t>(it - g_ladder.begin());
  }

  ladder_tick_t bitrate_ladder_tick(int current_kbps,
    std::uint64_t encode_us, bool dropped) {
    ladder_tick_t r;
    r.kbps = current_kbps;
    r.from_kbps = current_kbps;
    if (current_kbps <= 0 || g_ladder.empty()) {
      r.direction = "stay";
      r.reason = "no bitrate configured";
      return r;
    }

    ewma_update(encode_us);
    int q8 = g_state.ewma_q8.load(std::memory_order_relaxed);
    double ewma_us = q8 / 256.0;

    int budget = budget_us();
    int pct = static_cast<int>((encode_us * 100) / std::max(1, budget));

    auto cur = g_state.index.load(std::memory_order_relaxed);
    auto bump = [&](std::size_t next, const std::string &dir, const std::string &why) {
      g_state.index.store(next, std::memory_order_relaxed);
      r.changed = true;
      r.direction = dir;
      r.kbps = g_ladder[next].kbps;
      r.reason = why;
    };

    // Drop decision: encode is consistently > (100 + drop_pct)% of budget.
    if (pct >= 100 + drop_pct()) {
      int streak = g_state.down_streak.fetch_add(1, std::memory_order_relaxed) + 1;
      if (streak >= drop_window() && cur + 1 < g_ladder.size()) {
        bump(cur + 1, "down",
          "encode EWMA " + std::to_string(static_cast<int>(ewma_us)) +
          " us exceeded budget by " + std::to_string(pct - 100) + "%");
        g_state.down_streak.store(0, std::memory_order_relaxed);
      } else {
        r.direction = "stay";
        r.reason = "drop streak " + std::to_string(streak) + "/" +
                   std::to_string(drop_window());
      }
    } else if (dropped) {
      int n = g_state.dropped_recent.fetch_add(1, std::memory_order_relaxed) + 1;
      // Two drops in a row => step down.
      if (n >= 2 && cur + 1 < g_ladder.size()) {
        bump(cur + 1, "down", "dropped-frame streak of " + std::to_string(n));
        g_state.dropped_recent.store(0, std::memory_order_relaxed);
        g_state.down_streak.store(0, std::memory_order_relaxed);
      } else {
        r.direction = "stay";
        r.reason = "dropped frame " + std::to_string(n) + "/2";
      }
    } else {
      // No pressure => consider stepping UP.
      g_state.down_streak.store(0, std::memory_order_relaxed);
      g_state.dropped_recent.store(0, std::memory_order_relaxed);
      int hold = g_state.up_hold_ticks.fetch_add(1, std::memory_order_relaxed) + 1;
      // Approximately: up_hold_ticks counts frames; ~60 ticks/sec.
      int ticks_needed = up_hold_s() * 60;
      if (cur > 0 && pct < 80 && hold >= ticks_needed) {
        bump(cur - 1, "up",
          "encode healthy for " + std::to_string(up_hold_s()) +
          " s, headroom " + std::to_string(100 - pct) + "%");
        g_state.up_hold_ticks.store(0, std::memory_order_relaxed);
      } else {
        r.direction = "stay";
        r.reason = "healthy " + std::to_string(hold) + "/" +
                   std::to_string(ticks_needed) + " ticks";
      }
    }
    return r;
  }

  void bitrate_ladder_reset() {
    g_state.index.store(0, std::memory_order_relaxed);
    g_state.ewma_q8.store(0, std::memory_order_relaxed);
    g_state.down_streak.store(0, std::memory_order_relaxed);
    g_state.up_hold_ticks.store(0, std::memory_order_relaxed);
    g_state.dropped_recent.store(0, std::memory_order_relaxed);
  }

  std::size_t current_rung_index() {
    return g_state.index.load(std::memory_order_relaxed);
  }

  ladder_state_t bitrate_ladder_state() {
    ladder_state_t s;
    s.index = current_rung_index();
    s.rung_count = g_ladder.size();
    s.current_kbps = (s.index < g_ladder.size()) ? g_ladder[s.index].kbps : 0;
    s.ewma_us = g_state.ewma_q8.load(std::memory_order_relaxed) / 256.0;
    s.down_streak = g_state.down_streak.load(std::memory_order_relaxed);
    s.up_hold_ticks = g_state.up_hold_ticks.load(std::memory_order_relaxed);
    s.dropped_recent = g_state.dropped_recent.load(std::memory_order_relaxed);
    return s;
  }

}  // namespace spice
