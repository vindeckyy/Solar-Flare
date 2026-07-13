// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_trusted_subnet.cpp
 * @brief Tests for net::is_trusted_subnet.
 */
#include "../tests_common.h"

#include <src/network.h>

TEST(TrustedSubnetTest, EmptyInputsAlwaysReturnFalse) {
  EXPECT_FALSE(net::is_trusted_subnet("", "10.0.0.0/24"));
  EXPECT_FALSE(net::is_trusted_subnet("10.0.0.5", ""));
  EXPECT_FALSE(net::is_trusted_subnet("", ""));
}

/**
 * @brief IPv4 client inside a trusted IPv4 subnet returns true.
 */
TEST(TrustedSubnetTest, Ipv4ClientInsideTrustedSubnet) {
  EXPECT_TRUE(net::is_trusted_subnet("10.0.0.5", "10.0.0.0/24"));
  EXPECT_TRUE(net::is_trusted_subnet("192.168.1.42", "192.168.0.0/16"));
  EXPECT_TRUE(net::is_trusted_subnet("172.16.5.10", "172.16.0.0/12"));
}

/**
 * @brief IPv4 client outside any trusted subnet returns false.
 */
TEST(TrustedSubnetTest, Ipv4ClientOutsideTrustedSubnet) {
  EXPECT_FALSE(net::is_trusted_subnet("8.8.8.8", "10.0.0.0/24"));
  EXPECT_FALSE(net::is_trusted_subnet("11.0.0.1", "10.0.0.0/8,192.168.0.0/16"));
}

/**
 * @brief IPv6 client inside a trusted IPv6 subnet returns true.
 */
TEST(TrustedSubnetTest, Ipv6ClientInsideTrustedSubnet) {
  EXPECT_TRUE(net::is_trusted_subnet("fc00::1", "fc00::/7"));
  EXPECT_TRUE(net::is_trusted_subnet("fd12:3456:789a::1", "fc00::/7"));
  EXPECT_TRUE(net::is_trusted_subnet("fe80::1", "fe80::/10"));
}

/**
 * @brief IPv6 client outside any trusted subnet returns false.
 */
TEST(TrustedSubnetTest, Ipv6ClientOutsideTrustedSubnet) {
  EXPECT_FALSE(net::is_trusted_subnet("2001:db8::1", "fc00::/7"));
  EXPECT_FALSE(net::is_trusted_subnet("::1", "fc00::/7"));
}

/**
 * @brief IPv4-mapped IPv6 addresses (::ffff:10.0.0.5) are normalised
 *        and match IPv4 CIDRs.
 */
TEST(TrustedSubnetTest, Ipv4MappedIpv6MatchesIpv4Cidr) {
  EXPECT_TRUE(net::is_trusted_subnet("::ffff:10.0.0.5", "10.0.0.0/24"));
  EXPECT_FALSE(net::is_trusted_subnet("::ffff:8.8.8.8", "10.0.0.0/24"));
}

/**
 * @brief Comma-separated list with whitespace around entries is
 *        tolerated.
 */
TEST(TrustedSubnetTest, WhitespaceAroundCidrsIsTolerated) {
  EXPECT_TRUE(net::is_trusted_subnet("10.0.0.5", " 10.0.0.0/24 , 192.168.1.0/24 "));
  EXPECT_TRUE(net::is_trusted_subnet("192.168.1.42", " 10.0.0.0/24 , 192.168.1.0/24 "));
}

/**
 * @brief Malformed CIDR entries are skipped, valid ones still match.
 */
TEST(TrustedSubnetTest, MalformedCidrsAreSkipped) {
  EXPECT_TRUE(net::is_trusted_subnet("10.0.0.5", "not-a-cidr,10.0.0.0/24,999.999.999.999/24"));
  EXPECT_FALSE(net::is_trusted_subnet("10.0.0.5", "not-a-cidr,alsobad"));
}

/**
 * @brief Malformed client IP returns false (doesn't crash).
 */
TEST(TrustedSubnetTest, MalformedClientIpReturnsFalse) {
  EXPECT_FALSE(net::is_trusted_subnet("not-an-ip", "10.0.0.0/24"));
  EXPECT_FALSE(net::is_trusted_subnet("999.999.999.999", "10.0.0.0/24"));
}

/**
 * @brief Empty trailing entries between commas don't break parsing.
 */
TEST(TrustedSubnetTest, EmptyEntriesBetweenCommasAreSkipped) {
  EXPECT_TRUE(net::is_trusted_subnet("10.0.0.5", "10.0.0.0/24,,,"));
  EXPECT_FALSE(net::is_trusted_subnet("10.0.0.5", ",,,"));
}

/**
 * @brief /32 (single-host) CIDRs match only that host.
 */
TEST(TrustedSubnetTest, Slash32CidrMatchesOnlyOneHost) {
  EXPECT_TRUE(net::is_trusted_subnet("10.0.0.5", "10.0.0.5/32"));
  EXPECT_FALSE(net::is_trusted_subnet("10.0.0.6", "10.0.0.5/32"));
}
