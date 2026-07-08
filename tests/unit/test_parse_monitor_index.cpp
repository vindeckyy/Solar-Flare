/**
 * @file tests/unit/test_parse_monitor_index.cpp
 * @brief Tests for util::parse_monitor_index.
 *
 * Regression: previously util::from_view() treated every byte of its
 * input as a decimal digit, so a non-numeric monitor name like
 * "Virtual-Virtual-1" decoded to an arbitrary negative number that
 * became a corrupt out-of-bounds index in the KMS plane scan array
 * (the "-1797036149" panic seen by Reddit user braxton on CachyOS).
 */
#include "../tests_common.h"

#include <src/utility.h>

TEST(ParseMonitorIndexTest, NumericStringParsesToValue) {
  EXPECT_EQ(util::parse_monitor_index("0", 999), 0);
  EXPECT_EQ(util::parse_monitor_index("1", 999), 1);
  EXPECT_EQ(util::parse_monitor_index("42", 999), 42);
  EXPECT_EQ(util::parse_monitor_index("999999", -1), 999999);
}

TEST(ParseMonitorIndexTest, EmptyStringReturnsFallback) {
  EXPECT_EQ(util::parse_monitor_index("", 0), 0);
  EXPECT_EQ(util::parse_monitor_index("", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("", 42), 42);
}

TEST(ParseMonitorIndexTest, NonNumericReturnsFallback) {
  EXPECT_EQ(util::parse_monitor_index("Virtual-Virtual-1", 0), 0);
  EXPECT_EQ(util::parse_monitor_index("DP-1", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("HEADLESS-1", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("eDP-1", 0), 0);
}

TEST(ParseMonitorIndexTest, MixedAlphaNumericReturnsFallback) {
  EXPECT_EQ(util::parse_monitor_index("1abc", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("abc1", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("12.34", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("12-34", -1), -1);
}

TEST(ParseMonitorIndexTest, NegativeSignIsRejected) {
  // Negative indices have no meaning for monitor/plane selection; the
  // validator treats '-' as non-numeric so the fallback is used.
  EXPECT_EQ(util::parse_monitor_index("-1", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("-42", 0), 0);
}

TEST(ParseMonitorIndexTest, AllZerosReturnsZero) {
  EXPECT_EQ(util::parse_monitor_index("0", 999), 0);
  EXPECT_EQ(util::parse_monitor_index("00", 999), 0);
  EXPECT_EQ(util::parse_monitor_index("0000", 999), 0);
}
