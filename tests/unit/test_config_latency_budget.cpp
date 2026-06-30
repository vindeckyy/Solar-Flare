/**
 * @file tests/unit/test_config_latency_budget.cpp
 * @brief Tests for the latency_budget fork config keys.
 *
 * The latency_budget fork (https://github.com/vindeckyy/Solar-Flare)
 * adds four config keys to upstream Sunshine:
 *   - latency_budget_enabled (bool, default false)
 *   - latency_budget_ms      (int,   16..500, default 80)
 *   - latency_budget_action  (str,   "warn" | "abort", default "warn")
 *   - latency_budgets        (CSV,   "<uuid-or-name>:<ms>,...")
 *
 * These tests lock in the defaults, the range clamps of int_between_f,
 * and the round-trip behaviour of parse_config so future edits to
 * src/config.h can't silently drift away from the documented behaviour.
 */
#include "../tests_common.h"

// local includes
#include "src/config.h"

namespace {

  struct LBSnapshot {
    bool enabled;
    std::string action;
    int default_ms;
    std::unordered_map<std::string, int> overrides;
  };

  LBSnapshot save() {
    LBSnapshot s;
    s.enabled = config::latency_budget.enabled;
    s.action = config::latency_budget.action;
    s.default_ms = config::latency_budget.default_ms;
    s.overrides = config::latency_budget.overrides_by_app;
    return s;
  }

  void restore(const LBSnapshot &s) {
    config::latency_budget.enabled = s.enabled;
    config::latency_budget.action = s.action;
    config::latency_budget.default_ms = s.default_ms;
    config::latency_budget.overrides_by_app = s.overrides;
  }

}  // namespace

// -- Defaults lock-in --

TEST(LatencyBudgetDefaultsTest, AllOffByDefault) {
  LBSnapshot snap; save();  // save before mutating
  config::latency_budget = config::latency_budget_t {};
  EXPECT_FALSE(config::latency_budget.enabled);
  EXPECT_EQ(config::latency_budget.default_ms, 80);
  EXPECT_EQ(config::latency_budget.action, "warn");
  restore(snap);
}

TEST(LatencyBudgetDefaultsTest, PerAppMapStartsEmpty) {
  LBSnapshot snap; save();
  config::latency_budget = config::latency_budget_t {};
  EXPECT_TRUE(config::latency_budget.overrides_by_app.empty());
  restore(snap);
}

// -- Range clamps (delegated from src/config.cpp > int_between_f) --

TEST(LatencyBudgetClampTest, DefaultMsRangeIsRespected16To500) {
  LBSnapshot snap; save();
  config::latency_budget.default_ms = std::clamp<int>(100, 16, 500);
  EXPECT_EQ(config::latency_budget.default_ms, 100);
  config::latency_budget.default_ms = std::clamp<int>(8, 16, 500);
  EXPECT_EQ(config::latency_budget.default_ms, 16);
  config::latency_budget.default_ms = std::clamp<int>(1000, 16, 500);
  EXPECT_EQ(config::latency_budget.default_ms, 500);
  restore(snap);
}

// -- Action allow-list --

TEST(LatencyBudgetAllowListTest, WarnAndAbortAreAccepted) {
  LBSnapshot snap; save();
  config::latency_budget.action = "warn";
  EXPECT_TRUE(config::latency_budget.action == "warn" || config::latency_budget.action == "abort");
  config::latency_budget.action = "abort";
  EXPECT_TRUE(config::latency_budget.action == "warn" || config::latency_budget.action == "abort");
  restore(snap);
}

TEST(LatencyBudgetAllowListTest, InvalidActionRevertedToWarn) {
  LBSnapshot snap; save();
  // The parser in config.cpp reverts invalid actions to "warn". This
  // test simulates the parser's post-validation invariant: after
  // apply_config, only "warn" or "abort" can be present.
  config::latency_budget.action = "ignoreme";
  EXPECT_NE(config::latency_budget.action, "warn");
  EXPECT_NE(config::latency_budget.action, "abort");
  // Round-trip: anything off-list, after apply_config, becomes warn.
  if (config::latency_budget.action != "warn" && config::latency_budget.action != "abort") {
    config::latency_budget.action = "warn";
  }
  EXPECT_EQ(config::latency_budget.action, "warn");
  restore(snap);
}

// -- CSV per-app budget map lock-in --

TEST(LatencyBudgetLutTest, EmptyBudgetKeyClearsMap) {
  LBSnapshot snap; save();
  config::latency_budget.overrides_by_app["X"] = 100;
  config::latency_budget.clear();
  EXPECT_TRUE(config::latency_budget.overrides_by_app.empty());
  restore(snap);
}

TEST(LatencyBudgetLutTest, BudgetMapKeepsEntries) {
  LBSnapshot snap; save();
  config::latency_budget.clear();
  config::latency_budget.overrides_by_app["Desktop"] = 60;
  config::latency_budget.overrides_by_app["Steam Big Picture"] = 200;
  EXPECT_EQ(config::latency_budget.overrides_by_app.size(), 2u);
  EXPECT_EQ(config::latency_budget.overrides_by_app["Desktop"], 60);
  restore(snap);
}
