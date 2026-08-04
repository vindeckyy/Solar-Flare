// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_latency_stats.cpp
 * @brief Tests for the SolarFlare fork's host-side latency statistics
 *        (src/latency_stats.h / src/latency_stats.cpp).
 *
 * The metric accumulators feed the GET /api/stream/latency endpoint
 * with a per-frame min/max/avg snapshot. These tests lock in the
 * accumulator math (min/max/avg/sample counting), the reset behaviour,
 * the clamping of negative samples, and the effective-settings
 * snapshot get/set round trip.
 */
#include "../tests_common.h"

// standard includes
#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

// local includes
#include "src/latency_stats.h"

namespace {

  class LatencyStatsTest: public ::testing::Test {
  protected:
    void TearDown() override {
      // Reset the singleton so tests don't leak samples into each other
      // or into later test binaries in the same process.
      sunshine::latency_stats().reset();
      sunshine::latency_stats().set_effective_settings({});
    }
  };

  // ---------------------------------------------------------------------
  // metric_accumulator_t
  // ---------------------------------------------------------------------

  TEST_F(LatencyStatsTest, EmptyAccumulatorSnapshotIsZero) {
    sunshine::metric_accumulator_t acc;
    auto s = acc.snapshot();
    EXPECT_DOUBLE_EQ(s.min, 0.0);
    EXPECT_DOUBLE_EQ(s.max, 0.0);
    EXPECT_DOUBLE_EQ(s.avg, 0.0);
    EXPECT_EQ(s.samples, 0u);
  }

  TEST_F(LatencyStatsTest, SingleSampleHasMinMaxAvgEqual) {
    sunshine::metric_accumulator_t acc;
    acc.collect(2.5);

    auto s = acc.snapshot();
    EXPECT_DOUBLE_EQ(s.min, 2.5);
    EXPECT_DOUBLE_EQ(s.max, 2.5);
    EXPECT_DOUBLE_EQ(s.avg, 2.5);
    EXPECT_EQ(s.samples, 1u);
  }

  TEST_F(LatencyStatsTest, AccumulatorTracksMinMaxAvg) {
    sunshine::metric_accumulator_t acc;
    acc.collect(1.0);
    acc.collect(5.0);
    acc.collect(3.0);

    auto s = acc.snapshot();
    EXPECT_DOUBLE_EQ(s.min, 1.0);
    EXPECT_DOUBLE_EQ(s.max, 5.0);
    EXPECT_DOUBLE_EQ(s.avg, 3.0);
    EXPECT_EQ(s.samples, 3u);
  }

  TEST_F(LatencyStatsTest, NegativeSamplesAreClampedToZero) {
    sunshine::metric_accumulator_t acc;
    acc.collect(-4.0);

    auto s = acc.snapshot();
    EXPECT_DOUBLE_EQ(s.min, 0.0);
    EXPECT_DOUBLE_EQ(s.max, 0.0);
    EXPECT_DOUBLE_EQ(s.avg, 0.0);
    EXPECT_EQ(s.samples, 1u);
  }

  TEST_F(LatencyStatsTest, ResetClearsAccumulatedState) {
    sunshine::metric_accumulator_t acc;
    acc.collect(1.0);
    acc.collect(9.0);
    acc.reset();

    auto s = acc.snapshot();
    EXPECT_DOUBLE_EQ(s.min, 0.0);
    EXPECT_DOUBLE_EQ(s.max, 0.0);
    EXPECT_DOUBLE_EQ(s.avg, 0.0);
    EXPECT_EQ(s.samples, 0u);
  }

  TEST_F(LatencyStatsTest, ResetClearsEverySingletonMetric) {
    auto &stats = sunshine::latency_stats();
    stats.capture_ms.collect(1.0);
    stats.convert_ms.collect(2.0);
    stats.encode_ms.collect(3.0);
    stats.network_total_ms.collect(4.0);
    stats.network_queue_dwell_ms.collect(5.0);
    stats.network_fec_ms.collect(6.0);
    stats.network_send_ms.collect(7.0);
    stats.rtt_ms.collect(8.0);

    stats.reset();

    EXPECT_EQ(stats.capture_ms.snapshot().samples, 0u);
    EXPECT_EQ(stats.convert_ms.snapshot().samples, 0u);
    EXPECT_EQ(stats.encode_ms.snapshot().samples, 0u);
    EXPECT_EQ(stats.network_total_ms.snapshot().samples, 0u);
    EXPECT_EQ(stats.network_queue_dwell_ms.snapshot().samples, 0u);
    EXPECT_EQ(stats.network_fec_ms.snapshot().samples, 0u);
    EXPECT_EQ(stats.network_send_ms.snapshot().samples, 0u);
    EXPECT_EQ(stats.rtt_ms.snapshot().samples, 0u);
  }

  // Two collectors racing on the very first sample used to hit the old
  // `if (s == 1) min.store/max.store` fast path: the second collector
  // could read the sentinel-initialized min (0.0) and skip storing its
  // (larger) value. With the infinity sentinels + CAS-only loops, every
  // sample is guaranteed to be reflected in the final min/max regardless
  // of interleaving.
  TEST_F(LatencyStatsTest, ConcurrentCollectMinMaxIsExact) {
    sunshine::metric_accumulator_t acc;

    constexpr int kThreads = 8;
    constexpr int kSamplesPerThread = 2000;
    double expected_min = std::numeric_limits<double>::infinity();
    double expected_max = -std::numeric_limits<double>::infinity();

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
      threads.emplace_back([&acc, t] {
        // Give each thread a distinct value range so the global min/max
        // are known up front, and have every thread hammer the first
        // sample simultaneously to stress the first-sample race.
        const double base = 1.0 + t;
        for (int i = 0; i < kSamplesPerThread; ++i) {
          acc.collect(base + i * 0.001);
        }
      });
    }
    for (auto &th : threads) {
      th.join();
    }

    // Global min/max over all threads' ranges.
    for (int t = 0; t < kThreads; ++t) {
      expected_min = std::min(expected_min, 1.0 + t);
      expected_max = std::max(expected_max, 1.0 + t + (kSamplesPerThread - 1) * 0.001);
    }

    const auto s = acc.snapshot();
    EXPECT_EQ(s.samples, static_cast<std::uint32_t>(kThreads * kSamplesPerThread));
    EXPECT_DOUBLE_EQ(s.min, expected_min);
    EXPECT_DOUBLE_EQ(s.max, expected_max);
  }

  // reset() is not synchronized with collect(): a collector that started
  // before the reset may land samples after it. The telemetry consumer
  // only needs approximate results, so this test documents the tolerance:
  // the post-reset snapshot must never expose the +/-infinity sentinels
  // (that would leak into the UI as "inf"), and a sample that lands
  // after the reset is allowed to count — it belongs to the new window.
  TEST_F(LatencyStatsTest, ResetMidCollectIsApproximate) {
    sunshine::metric_accumulator_t acc;
    std::atomic<bool> stop {false};
    std::atomic<bool> started {false};

    std::thread collector {[&] {
      started.store(true, std::memory_order_release);
      while (!stop.load(std::memory_order_acquire)) {
        acc.collect(5.0);
      }
    }};

    while (!started.load(std::memory_order_acquire)) {
    }

    // Let a few samples land, then reset while the collector keeps going.
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    acc.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    // Must not leak the infinity sentinels, and all observed values must
    // belong to the post-reset window (all 5.0 samples).
    const auto s = acc.snapshot();
    EXPECT_TRUE(std::isfinite(s.min));
    EXPECT_TRUE(std::isfinite(s.max));
    EXPECT_TRUE(std::isfinite(s.avg));
    if (s.samples > 0) {
      EXPECT_DOUBLE_EQ(s.min, 5.0);
      EXPECT_DOUBLE_EQ(s.max, 5.0);
      EXPECT_NEAR(s.avg, 5.0, 0.001);  // accumulated float error over many samples
    }

    stop.store(true, std::memory_order_release);
    collector.join();
  }

  // ---------------------------------------------------------------------
  // effective_settings_t
  // ---------------------------------------------------------------------

  TEST_F(LatencyStatsTest, EffectiveSettingsRoundTrip) {
    sunshine::effective_settings_t settings;
    settings.codec = "hevc_vaapi";
    settings.hwdevice = "vaapi";
    settings.vendor = "Mesa Gallium";
    settings.va_entrypoint = "EncSlice";
    settings.rc_mode = "VBR";
    settings.quality = 2;
    settings.slices = 1;
    settings.async_depth = 4;
    settings.qmin = 10;
    settings.qmax = 40;
    settings.rc_buffer_size = 123456;
    settings.bit_rate = 60000000;
    settings.framerate = 60;

    auto &stats = sunshine::latency_stats();
    stats.set_effective_settings(settings);

    auto got = stats.effective_settings();
    EXPECT_EQ(got.codec, "hevc_vaapi");
    EXPECT_EQ(got.hwdevice, "vaapi");
    EXPECT_EQ(got.vendor, "Mesa Gallium");
    EXPECT_EQ(got.va_entrypoint, "EncSlice");
    EXPECT_EQ(got.rc_mode, "VBR");
    EXPECT_EQ(got.quality, 2);
    EXPECT_EQ(got.slices, 1);
    EXPECT_EQ(got.async_depth, 4);
    EXPECT_EQ(got.qmin, 10);
    EXPECT_EQ(got.qmax, 40);
    EXPECT_EQ(got.rc_buffer_size, 123456);
    EXPECT_EQ(got.bit_rate, 60000000);
    EXPECT_EQ(got.framerate, 60);
  }

  TEST_F(LatencyStatsTest, EffectiveSettingsDefaultWhenUnset) {
    auto got = sunshine::latency_stats().effective_settings();
    EXPECT_TRUE(got.codec.empty());
    EXPECT_TRUE(got.hwdevice.empty());
    EXPECT_EQ(got.quality, 0);
    EXPECT_EQ(got.slices, 0);
    EXPECT_EQ(got.async_depth, 0);
    EXPECT_EQ(got.qmin, 0);
    EXPECT_EQ(got.qmax, 0);
    EXPECT_EQ(got.rc_buffer_size, 0);
    EXPECT_EQ(got.bit_rate, 0);
    EXPECT_EQ(got.framerate, 0);
  }

  TEST_F(LatencyStatsTest, SingletonIsStable) {
    // The process-wide singleton must return the same object every time.
    EXPECT_EQ(&sunshine::latency_stats(), &sunshine::latency_stats());
  }

}  // namespace
