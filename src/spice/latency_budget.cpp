/**
 * @file src/spice/latency_budget.cpp
 * @brief Implementation of per-app latency budget enforcer.
 *
 * See latency_budget.h for design intent.
 */
#include "latency_budget.h"

// standard includes
#include <algorithm>
#include <chrono>
#include <string_view>
#include <unordered_map>
#include <vector>

// project includes
#include "config.h"
#include "logging.h"

using namespace std::string_view_literals;
using namespace std::chrono_literals;

namespace spice {

  namespace {

    // Tiny rolling window per app. We deliberately keep this here (not
    // in the telemetry ring) because latency_budget evaluation is
    // expected to be called from the control plane (not the hot path)
    // and the window size is small and bounded.
    std::unordered_map<std::string, std::vector<std::uint64_t>> g_windows;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_last_warn;

    constexpr std::size_t kWindow = 30;
    constexpr auto kWarnCooldown = std::chrono::seconds(5);

  }  // namespace

  std::uint64_t resolve_budget_us(const std::string &app_name_or_uuid) {
    auto &b = config::latency_budget.overrides_by_app;
    auto it = b.find(app_name_or_uuid);
    int ms;
    if (it != b.end()) {
      ms = it->second;
    } else {
      ms = config::latency_budget.default_ms;
    }
    return static_cast<std::uint64_t>(ms) * 1000u;
  }

  budget_eval_t evaluate(const std::string &app_name_or_uuid,
    const std::vector<std::uint64_t> &recent_encode_us) {
    budget_eval_t r {};
    r.budget_us = resolve_budget_us(app_name_or_uuid);

    if (recent_encode_us.empty()) {
      r.within_budget = true;
      r.median_us = 0;
      r.action_taken = "none";
      return r;
    }

    // Update rolling window for this app.
    auto &w = g_windows[app_name_or_uuid];
    w.insert(w.end(), recent_encode_us.begin(), recent_encode_us.end());
    if (w.size() > kWindow) {
      w.erase(w.begin(), w.begin() + (w.size() - kWindow));
    }

    auto copy = w;
    std::sort(copy.begin(), copy.end());
    r.median_us = copy[copy.size() / 2];
    r.within_budget = r.median_us <= r.budget_us;

    if (r.within_budget) {
      r.action_taken = "none";
      return r;
    }

    // Decide action. Default is "warn"; honor spice.latency_action.
    auto now = std::chrono::steady_clock::now();
    auto act = config::latency_budget.action;
    if (act == "abort") {
      r.action_taken = "abort";
      BOOST_LOG(warning) << "spice(latency): budget exceeded for app=" << app_name_or_uuid
                         << " median=" << r.median_us / 1000.0 << " ms, budget=" << r.budget_us / 1000.0
                         << " ms; aborting session."sv;
    } else {
      auto last = g_last_warn[app_name_or_uuid];
      r.action_taken = "warn";
      if (now - last >= kWarnCooldown) {
        BOOST_LOG(warning) << "spice(latency): budget exceeded for app=" << app_name_or_uuid
                           << " median=" << r.median_us / 1000.0 << " ms, budget=" << r.budget_us / 1000.0
                           << " ms."sv;
        g_last_warn[app_name_or_uuid] = now;
      }
    }
    return r;
  }

  void clear_window(const std::string &app_name_or_uuid) {
    g_windows.erase(app_name_or_uuid);
    g_last_warn.erase(app_name_or_uuid);
  }

  int global_budget_ms() {
    return config::latency_budget.default_ms;
  }

  const std::string &action() {
    static const std::string s_warn = "warn";
    static const std::string s_abort = "abort";
    return config::latency_budget.action == "abort" ? s_abort : s_warn;
  }

}  // namespace spice
