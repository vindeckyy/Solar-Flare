// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_utility_parsing.cpp
 * @brief Tests strict integer and hexadecimal parsing helpers.
 */
#include "../tests_common.h"

#include <array>
#include <cstdint>
#include <limits>
#include <src/utility.h>

TEST(UtilityIntegerParsingTest, RejectsMalformedAndOutOfRangeInput) {
  EXPECT_EQ(util::parse_integer<std::int64_t>("-42"), -42);
  EXPECT_EQ(util::parse_integer<std::int64_t>("9223372036854775807"), std::numeric_limits<std::int64_t>::max());
  EXPECT_FALSE(util::parse_integer<std::int64_t>("9223372036854775808"));
  EXPECT_FALSE(util::parse_integer<std::int64_t>("12x"));
  EXPECT_FALSE(util::parse_integer<std::int64_t>(""));
}

TEST(UtilityIntegerParsingTest, MonitorIndexFallsBackForOverflow) {
  EXPECT_EQ(util::parse_monitor_index("999999999999999999999999", -1), -1);
  EXPECT_EQ(util::parse_monitor_index("DP-1", 7), 7);
}

TEST(UtilityHexParsingTest, RejectsMalformedAndOversizeInput) {
  EXPECT_EQ(util::from_hex<std::uint32_t>("1234"), 0x1234U);
  EXPECT_FALSE(util::from_hex<std::uint32_t>("123"));
  EXPECT_FALSE(util::from_hex<std::uint32_t>("GG"));
  EXPECT_FALSE(util::from_hex<std::uint32_t>("0011223344"));
  EXPECT_EQ(util::from_hex_vec("", true), "");
  EXPECT_EQ(util::from_hex_vec("0", true), "");
  EXPECT_EQ(util::from_hex_vec("ZZ", true), "");
}

TEST(UtilityHexParsingTest, PreservesLegacyByteOrder) {
  const auto bytes = util::from_hex<std::array<std::uint8_t, 2>>("0102", true);
  ASSERT_TRUE(bytes);
  EXPECT_EQ(*bytes, (std::array<std::uint8_t, 2> {1, 2}));
  EXPECT_EQ(util::from_hex_vec("0102", true), std::string("\x01\x02", 2));
}
