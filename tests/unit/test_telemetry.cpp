// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_telemetry.cpp
 * @brief Tests for the host resource telemetry store (src/telemetry.*).
 *
 * The store feeds the GET /api/stream/telemetry endpoint. These tests
 * lock in the ring-buffer behaviour (capacity, wrap, order), the
 * negative-clamp, the JSON snapshot shape, and the reset semantics.
 */
#include "../tests_common.h"

// standard includes
#include <cstddef>

// third-party includes
#include <nlohmann/json.hpp>

// local includes
#include "src/telemetry.h"

namespace {

  class TelemetryTest: public ::testing::Test {
  protected:
    void TearDown() override {
      // Reset the process-wide store so tests don't leak samples into
      // each other or into later test binaries in the same process.
      sunshine::telemetry::reset();
    }
  };

  TEST_F(TelemetryTest, RecordAppendsToSeries) {
    sunshine::telemetry::record("test_ms", 1.0);
    sunshine::telemetry::record("test_ms", 2.5);

    auto snap = sunshine::telemetry::snapshot();
    ASSERT_TRUE(snap.contains("test_ms"));
    auto series = snap["test_ms"];
    ASSERT_EQ(series.size(), 2);
    EXPECT_DOUBLE_EQ(static_cast<double>(series[0]), 1.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(series[1]), 2.5);
  }

  TEST_F(TelemetryTest, NegativeValuesAreClamped) {
    sunshine::telemetry::record("test_ms", -5.0);

    auto snap = sunshine::telemetry::snapshot();
    auto series = snap["test_ms"];
    ASSERT_EQ(series.size(), 1);
    EXPECT_DOUBLE_EQ(static_cast<double>(series[0]), 0.0);
  }

  TEST_F(TelemetryTest, RingBufferWrapsAtCapacity) {
    for (std::size_t i = 0; i < sunshine::telemetry::kSeriesCapacity + 10; ++i) {
      sunshine::telemetry::record("test_ms", static_cast<double>(i));
    }

    auto snap = sunshine::telemetry::snapshot();
    auto series = snap["test_ms"];
    ASSERT_EQ(series.size(), sunshine::telemetry::kSeriesCapacity);
    // The oldest 10 samples were overwritten; the first retained is 10.
    EXPECT_DOUBLE_EQ(static_cast<double>(series[0]), 10.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(series[series.size() - 1]),
      static_cast<double>(sunshine::telemetry::kSeriesCapacity + 9));
  }

  TEST_F(TelemetryTest, EmptyStoreReturnsOnlyWindow) {
    auto snap = sunshine::telemetry::snapshot();
    EXPECT_EQ(snap.size(), 1);
    EXPECT_TRUE(snap.contains("window_s"));
  }

  TEST_F(TelemetryTest, ResetClearsAllSeries) {
    sunshine::telemetry::record("test_ms", 1.0);
    sunshine::telemetry::reset();

    auto snap = sunshine::telemetry::snapshot();
    EXPECT_FALSE(snap.contains("test_ms"));
  }

  TEST_F(TelemetryTest, MultipleSeriesAreIndependent) {
    sunshine::telemetry::record("cpu", 10.0);
    sunshine::telemetry::record("ram", 20.0);

    auto snap = sunshine::telemetry::snapshot();
    ASSERT_TRUE(snap.contains("cpu"));
    ASSERT_TRUE(snap.contains("ram"));
    EXPECT_DOUBLE_EQ(static_cast<double>(snap["cpu"][0]), 10.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(snap["ram"][0]), 20.0);
  }

}  // namespace
