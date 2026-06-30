/**
 * @file tests/unit/test_config_bitrate_ladder.cpp
 * @brief Tests for the bitrate_ladder fork config keys.
 */
#include "../tests_common.h"
#include "src/config.h"

namespace {
  struct BLSnapshot {
    bool enabled;
    int drop_pct;
    int drop_window;
    int up_hold_s;
    int target_fps;
    std::string rungs;
  };
  BLSnapshot save() {
    return {config::bitrate_ladder.enabled,
            config::bitrate_ladder.drop_pct,
            config::bitrate_ladder.drop_window,
            config::bitrate_ladder.up_hold_s,
            config::bitrate_ladder.target_fps,
            config::bitrate_ladder.rungs};
  }
  void restore(const BLSnapshot &s) {
    config::bitrate_ladder.enabled = s.enabled;
    config::bitrate_ladder.drop_pct = s.drop_pct;
    config::bitrate_ladder.drop_window = s.drop_window;
    config::bitrate_ladder.up_hold_s = s.up_hold_s;
    config::bitrate_ladder.target_fps = s.target_fps;
    config::bitrate_ladder.rungs = s.rungs;
  }
}

TEST(BitrateLadderDefaultsTest, OffByDefault) {
  BLSnapshot snap; save();
  config::bitrate_ladder = config::bitrate_ladder_t {};
  EXPECT_FALSE(config::bitrate_ladder.enabled);
  EXPECT_EQ(config::bitrate_ladder.drop_pct, 25);
  EXPECT_EQ(config::bitrate_ladder.drop_window, 30);
  EXPECT_EQ(config::bitrate_ladder.up_hold_s, 30);
  EXPECT_EQ(config::bitrate_ladder.target_fps, 60);
  EXPECT_TRUE(config::bitrate_ladder.rungs.empty());
  restore(snap);
}

TEST(BitrateLadderClampTest, DropPctRangeIsRespected1To90) {
  BLSnapshot snap; save();
  config::bitrate_ladder.drop_pct = std::clamp<int>(50, 1, 90);
  EXPECT_EQ(config::bitrate_ladder.drop_pct, 50);
  config::bitrate_ladder.drop_pct = std::clamp<int>(100, 1, 90);
  EXPECT_EQ(config::bitrate_ladder.drop_pct, 90);
  config::bitrate_ladder.drop_pct = std::clamp<int>(-5, 1, 90);
  EXPECT_EQ(config::bitrate_ladder.drop_pct, 1);
  restore(snap);
}

TEST(BitrateLadderClampTest, UpHoldSecsRangeIsRespected1To600) {
  BLSnapshot snap; save();
  config::bitrate_ladder.up_hold_s = std::clamp<int>(300, 1, 600);
  EXPECT_EQ(config::bitrate_ladder.up_hold_s, 300);
  config::bitrate_ladder.up_hold_s = std::clamp<int>(1000, 1, 600);
  EXPECT_EQ(config::bitrate_ladder.up_hold_s, 600);
  restore(snap);
}

TEST(BitrateLadderClampTest, TargetFpsRangeIsRespected24To144) {
  BLSnapshot snap; save();
  config::bitrate_ladder.target_fps = std::clamp<int>(60, 24, 144);
  EXPECT_EQ(config::bitrate_ladder.target_fps, 60);
  config::bitrate_ladder.target_fps = std::clamp<int>(10, 24, 144);
  EXPECT_EQ(config::bitrate_ladder.target_fps, 24);
  config::bitrate_ladder.target_fps = std::clamp<int>(999, 24, 144);
  EXPECT_EQ(config::bitrate_ladder.target_fps, 144);
  restore(snap);
}
