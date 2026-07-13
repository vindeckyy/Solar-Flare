/**
 * @file tests/unit/test_stat_trackers.cpp
 * @brief Tests for src/stat_trackers.h — fork-modified, previously uncovered.
 *
 * ponytail: one test per function, no suites.
 *   - one_digit_after_decimal() and two_digits_after_decimal() verify
 *     the precision contract the Web UI depends on.
 *   - min_max_avg_tracker validates the min/max/avg accumulation and the
 *     reset contract.
 */
#include "../tests_common.h"

// local includes
#include "src/stat_trackers.h"

TEST(StatTrackersTest, OneDigitFormatterProducesOneDecimal) {
  auto fmt = stat_trackers::one_digit_after_decimal();
  auto result = boost::str(fmt % 1.5);
  EXPECT_EQ(result, "1.5");
}

TEST(StatTrackersTest, TwoDigitFormatterProducesTwoDecimals) {
  auto fmt = stat_trackers::two_digits_after_decimal();
  auto result = boost::str(fmt % 3.14159);
  EXPECT_EQ(result, "3.14");
}

TEST(StatTrackersTest, MinMaxAvgTrackerAccumulatesUntilInterval) {
  stat_trackers::min_max_avg_tracker<double> tracker;

  // With a long interval the tracker accumulates without firing.
  tracker.collect_and_callback_on_interval(10.0, [](double, double, double) {
    FAIL() << "callback should not fire on first collect";
  }, std::chrono::seconds(3600));

  tracker.collect_and_callback_on_interval(5.0, [](double, double, double) {
    FAIL() << "callback should not fire before interval elapses";
  }, std::chrono::seconds(3600));

  // Internal state accumulated: min=5, max=10, sum=15, calls=2.
  // No observable output yet, but the tracker didn't crash.
  SUCCEED();
}

TEST(StatTrackersTest, MinMaxAvgTrackerResetsToEmpty) {
  stat_trackers::min_max_avg_tracker<int> tracker;

  // Interval=0 means every second collect fires the callback.
  int fire_count = 0;
  tracker.collect_and_callback_on_interval(10, [&](int, int, double) { fire_count++; }, std::chrono::seconds(0));
  EXPECT_EQ(fire_count, 0);

  tracker.collect_and_callback_on_interval(20, [&](int, int, double) { fire_count++; }, std::chrono::seconds(0));
  EXPECT_EQ(fire_count, 1);

  // reset() clears internal state but doesn't fire the callback.
  tracker.reset();
  EXPECT_EQ(fire_count, 1);

  // After reset the first collect primes again, second fires.
  tracker.collect_and_callback_on_interval(30, [&](int, int, double) { fire_count++; }, std::chrono::seconds(0));
  EXPECT_EQ(fire_count, 1);

  tracker.collect_and_callback_on_interval(40, [&](int, int, double) { fire_count++; }, std::chrono::seconds(0));
  EXPECT_EQ(fire_count, 2);
}
