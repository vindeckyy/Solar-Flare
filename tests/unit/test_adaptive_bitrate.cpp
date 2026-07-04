/**
 * @file tests/unit/test_adaptive_bitrate.cpp
 * @brief Tests for the EWMA-based adaptive bitrate controller.
 */
#include "../tests_common.h"

#include <src/adaptive_bitrate.h>

namespace {
  constexpr int kMinBitrate = 2000;
  constexpr int kMaxBitrate = 100000;

  video::AdaptiveBitrate::config_t default_cfg() {
    video::AdaptiveBitrate::config_t cfg;
    cfg.enabled = true;
    cfg.min_bitrate = kMinBitrate;
    cfg.max_bitrate = kMaxBitrate;
    return cfg;
  }
}  // namespace

/**
 * @brief First network sample must NOT be flagged as congestion.
 *
 * Regression test: before the priming fix, _ewma_rtt was 0 on the first
 * call, so the rtt_spike check `rtt_ms > 2 * 0.3 * rtt_ms` always
 * evaluated true and the controller immediately throttled the stream
 * on startup before the EWMA had any data to compare against.
 */
TEST(AdaptiveBitrateTest, FirstSampleIsNotCongestion) {
  video::AdaptiveBitrate ab(default_cfg());

  // A typical healthy RTT of 20ms with 0% loss should not trip congestion.
  ab.update_network_stats(0.0f, 20.0f);

  // After the priming sample the scale must still be at 1.0.
  EXPECT_FLOAT_EQ(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief EWMA only kicks in on the second sample; the second sample's
 *        RTT is now compared against the first.
 */
TEST(AdaptiveBitrateTest, EwmaKicksInOnSecondSample) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime: _ewma_rtt = 20

  // Second sample: 25ms, which is 25% above the primed EWMA but well
  // below 2x (40ms). Should NOT be a spike.
  ab.update_network_stats(0.0f, 25.0f);
  EXPECT_FLOAT_EQ(ab.get_target_bitrate(20000), 20000);

  // Third sample: 100ms, which is 5x the primed EWMA. Should be a spike.
  ab.update_network_stats(0.0f, 100.0f);
  // Scale drops by the rtt_penalty of 0.7. We just assert it dropped.
  EXPECT_LT(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief Packet loss above 1% reduces the bitrate proportionally.
 */
TEST(AdaptiveBitrateTest, PacketLossTriggersScaleDown) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime

  // 2% sustained packet loss. EWMA converges on this value, triggering
  // the "loss > 1%" branch: loss_penalty = 1 - 2/100 = 0.98.
  for (int i = 0; i < 20; ++i) {
    ab.update_network_stats(2.0f, 20.0f);
  }
  // After many ticks the scale should have moved below 1.0.
  EXPECT_LT(ab.get_target_bitrate(20000), 20000);
  EXPECT_GE(ab.get_target_bitrate(20000), kMinBitrate);
}

/**
 * @brief High packet loss (>5%) hits the hard 0.50 penalty cap.
 */
TEST(AdaptiveBitrateTest, SeverePacketLossCapsAtHalfScale) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime

  for (int i = 0; i < 20; ++i) {
    ab.update_network_stats(10.0f, 20.0f);
  }
  int target = ab.get_target_bitrate(20000);
  EXPECT_GE(target, kMinBitrate);
  EXPECT_LE(target, 11000);  // 20000 * 0.5 + a little jitter, but floor enforced
}

/**
 * @brief Reset() restores scale to 1.0 and re-arms the EWMA priming.
 */
TEST(AdaptiveBitrateTest, ResetRestoresFullScaleAndRePrimes) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime
  ab.update_network_stats(10.0f, 20.0f);  // drive scale down
  for (int i = 0; i < 5; ++i) {
    ab.update_network_stats(10.0f, 20.0f);
  }
  ASSERT_LT(ab.get_target_bitrate(20000), 20000);

  ab.reset();
  // After reset the EWMA is un-primed again: the first post-reset call
  // is the new prime and must leave scale at 1.0.
  ab.update_network_stats(0.0f, 20.0f);
  EXPECT_FLOAT_EQ(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief Healthy stream + no network issues recovers toward full scale.
 *
 * The recovery loop requires (a) no congestion on the network side and
 * (b) the stream-health sample to come in healthy. We can't easily
 * advance the steady_clock in a test, so we verify the precondition:
 * after priming and a single healthy stream-health sample, the scale
 * is still at 1.0 (no premature bump).
 */
TEST(AdaptiveBitrateTest, HealthyStreamKeepsScaleFullBeforeTimeout) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime
  ab.update_stream_health(1.0f, 5.0f, 0.0f);
  EXPECT_FLOAT_EQ(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief Unhealthy stream-health sample drops scale and aborts recovery.
 */
TEST(AdaptiveBitrateTest, UnhealthyStreamHealthDropsScale) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime

  // 12ms encode time exceeds the 11ms threshold: scale *= 0.88.
  ab.update_stream_health(1.0f, 12.0f, 0.0f);
  EXPECT_LT(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief Dropped-frame ratio above 5% drops scale.
 */
TEST(AdaptiveBitrateTest, HighDroppedFrameRatioDropsScale) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime
  ab.update_stream_health(1.0f, 5.0f, 0.10f);  // 10% dropped
  EXPECT_LT(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief get_target_bitrate never returns below the configured floor.
 *
 * Regression test for the clamp-order bug: previously
 * `result = max(result, min); result = min(result, base)` could return
 * a value below min_bitrate when base_bitrate < min_bitrate.
 */
TEST(AdaptiveBitrateTest, FloorIsRespectedEvenWhenBaseIsBelowFloor) {
  video::AdaptiveBitrate ab(default_cfg());

  // Base bitrate below the floor. The output must still be >= min.
  int target = ab.get_target_bitrate(500);  // < 2000
  EXPECT_GE(target, kMinBitrate);
  EXPECT_LE(target, kMaxBitrate);
}

/**
 * @brief get_target_bitrate never returns above base_bitrate when scale is full.
 */
TEST(AdaptiveBitrateTest, CeilingRespectsBaseBitrate) {
  video::AdaptiveBitrate ab(default_cfg());
  EXPECT_EQ(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief get_target_bitrate never returns above max_bitrate.
 */
TEST(AdaptiveBitrateTest, CeilingRespectsMaxBitrate) {
  video::AdaptiveBitrate ab(default_cfg());
  // base > max: must clamp to max.
  EXPECT_EQ(ab.get_target_bitrate(500000), kMaxBitrate);
}
