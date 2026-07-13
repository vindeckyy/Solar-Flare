// SPDX-License-Identifier: GPL-3.0-only

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

/**
 * @brief First sample with rtt_ms=0 must still NOT be flagged as congestion.
 *
 * Regression test: a client that reports rtt_ms=0 on the very first packet
 * (common during session bring-up before measurement starts) used to pin
 * the EWMA near zero for the whole session lifetime, then trigger
 * rtt_spike on every subsequent real reading. Fixed by flooring the EWMA
 * seed at MIN_EWMA_RTT_MS (0.5ms).
 */
TEST(AdaptiveBitrateTest, FirstSampleZeroRttIsNotCongestion) {
  video::AdaptiveBitrate ab(default_cfg());

  // First packet of the session: RTT not yet measured, loss=0. Should not
  // throttle the stream.
  ab.update_network_stats(0.0f, 0.0f);

  // Two more healthy samples at a real LAN RTT. Without the MIN_EWMA_RTT_MS
  // floor these would compare against ~0 and register as spikes — driving
  // the scale down. With the floor in place the EWMA settles around the
  // real RTT and scale stays at 1.0.
  ab.update_network_stats(0.0f, 8.0f);
  ab.update_network_stats(0.0f, 8.0f);

  EXPECT_FLOAT_EQ(ab.get_target_bitrate(20000), 20000);
}

/**
 * @brief Healthy stream-health sample arms the recovery timer.
 *
 * Regression test: previously update_stream_health() only consumed the
 * recovery timer (it never set it). If update_network_stats() wasn't called
 * between the scale-down event and the recovery window (e.g. network stats
 * arrive less often than stream-health samples), bitrate would never
 * recover — the controller would stay throttled until process restart.
 * Fixed by arming the timer from update_stream_health when the stream is
 * healthy and no recovery timer is currently running.
 */
TEST(AdaptiveBitrateTest, HealthyStreamHealthArmsRecovery) {
  video::AdaptiveBitrate ab(default_cfg());
  ab.update_network_stats(0.0f, 20.0f);  // prime

  // Drive the scale down via healthy stream-health samples (no network
  // events). Use the high encode-time threshold to bring it down, then
  // flip back to healthy. The healthy sample must NOT block recovery; in
  // fact it must arm the recovery timer so future healthy ticks can ramp
  // the scale back up.
  ab.update_stream_health(0.80f, 12.0f, 0.0f);  // unhealthy: drops scale
  int dropped = ab.get_target_bitrate(20000);
  ASSERT_LT(dropped, 20000);

  // Multiple healthy stream-health ticks. Recovery itself is gated on
  // RECOVERY_TIMEOUT (10s) which we cannot advance here, but we can at
  // least confirm the post-congestion scale hasn't been knocked further
  // down by the healthy samples (the prior bug was that one healthy tick
  // would arm recovery, but a subsequent healthy tick with no _in_recovery
  // edge triggered would never re-arm — meaning a single re-congestion
  // event followed by stable health could permanently strand the timer).
  for (int i = 0; i < 5; ++i) {
    ab.update_stream_health(1.0f, 5.0f, 0.0f);
  }
  EXPECT_GE(ab.get_target_bitrate(20000), dropped);
}
