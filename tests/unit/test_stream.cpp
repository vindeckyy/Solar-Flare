// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_stream.cpp
 * @brief Test src/stream.*
 */

#include <cstdint>
#include <functional>
#include <src/stream.h>
#include <string>
#include <vector>

extern "C" {
#include <moonlight-common-c/src/Limelight-internal.h>
}

namespace stream {
  std::vector<uint8_t> concat_and_insert(uint64_t insert_size, uint64_t slice_size, const std::string_view &data1, const std::string_view &data2);
}

#include "../tests_common.h"

TEST(ConcatAndInsertTests, ConcatNoInsertionTest) {
  char b1[] = {'a', 'b'};
  char b2[] = {'c', 'd', 'e'};
  auto res = stream::concat_and_insert(0, 2, std::string_view {b1, sizeof(b1)}, std::string_view {b2, sizeof(b2)});
  auto expected = std::vector<uint8_t> {'a', 'b', 'c', 'd', 'e'};
  ASSERT_EQ(res, expected);
}

TEST(ConcatAndInsertTests, ConcatLargeStrideTest) {
  char b1[] = {'a', 'b'};
  char b2[] = {'c', 'd', 'e'};
  auto res = stream::concat_and_insert(1, sizeof(b1) + sizeof(b2) + 1, std::string_view {b1, sizeof(b1)}, std::string_view {b2, sizeof(b2)});
  auto expected = std::vector<uint8_t> {0, 'a', 'b', 'c', 'd', 'e'};
  ASSERT_EQ(res, expected);
}

TEST(ConcatAndInsertTests, ConcatSmallStrideTest) {
  char b1[] = {'a', 'b'};
  char b2[] = {'c', 'd', 'e'};
  auto res = stream::concat_and_insert(1, 1, std::string_view {b1, sizeof(b1)}, std::string_view {b2, sizeof(b2)});
  auto expected = std::vector<uint8_t> {0, 'a', 0, 'b', 0, 'c', 0, 'd', 0, 'e'};
  ASSERT_EQ(res, expected);
}

TEST(StreamAddressTests, IPv4AddressMatchesNetworkBytes) {
  const boost::asio::ip::address_v4::bytes_type interface_address {{192, 168, 12, 34}};
  const auto local_address = boost::asio::ip::make_address_v4("192.168.12.34");

  EXPECT_TRUE(stream::detail::ipv4_address_matches(interface_address, local_address));
}

TEST(StreamAddressTests, IPv4AddressRejectsDifferentNetworkBytes) {
  const boost::asio::ip::address_v4::bytes_type interface_address {{192, 168, 12, 35}};
  const auto local_address = boost::asio::ip::make_address_v4("192.168.12.34");

  EXPECT_FALSE(stream::detail::ipv4_address_matches(interface_address, local_address));
}

TEST(StreamPacingTests, BatchIsLimitedByRemainingIntervalAllowance) {
  EXPECT_EQ(stream::detail::next_pacing_batch_size(64, 64, 88, 90), 2);
}

TEST(StreamPacingTests, BatchIsLimitedByPlatformMaximum) {
  EXPECT_EQ(stream::detail::next_pacing_batch_size(100, 44, 0, 90), 44);
}

TEST(StreamPacingTests, BatchUsesAllPendingPacketsWhenBelowLimits) {
  EXPECT_EQ(stream::detail::next_pacing_batch_size(7, 44, 10, 90), 7);
}

TEST(StreamPacingTests, ExhaustedOrInvalidIntervalProducesNoBatch) {
  EXPECT_EQ(stream::detail::next_pacing_batch_size(20, 44, 90, 90), 0);
  EXPECT_EQ(stream::detail::next_pacing_batch_size(20, 44, 0, 0), 0);
}

TEST(StreamPacingTests, VideoQueueAgeBudgetTracksSafeAndAggressiveFrameIntervals) {
  EXPECT_EQ(stream::detail::video_queue_age_budget(60, 0, false), 50ms);
  EXPECT_EQ(stream::detail::video_queue_age_budget(60, 0, true), 25ms);
  EXPECT_EQ(stream::detail::video_queue_age_budget(60, 5994, false), std::chrono::nanoseconds(50'050'050));
}

TEST(StreamAdaptiveStatsTests, ParsesFrameFecStatusWithBigEndianFields) {
  SS_FRAME_FEC_STATUS status {};
  status.missingPacketsBeforeHighestReceived = util::endian::big<std::uint16_t>(5);
  status.totalDataPackets = util::endian::big<std::uint16_t>(100);

  auto stats = stream::detail::parse_frame_fec_status(
    std::string_view {reinterpret_cast<const char *>(&status), sizeof(status)},
    17
  );

  ASSERT_TRUE(stats);
  EXPECT_FLOAT_EQ(stats->first, 5.0f);
  EXPECT_FLOAT_EQ(stats->second, 17.0f);
}

TEST(StreamAdaptiveStatsTests, RejectsMalformedOrImpossibleFrameFecStatus) {
  EXPECT_FALSE(stream::detail::parse_frame_fec_status("short", 10));

  SS_FRAME_FEC_STATUS status {};
  status.missingPacketsBeforeHighestReceived = util::endian::big<std::uint16_t>(2);
  status.totalDataPackets = util::endian::big<std::uint16_t>(1);
  EXPECT_FALSE(stream::detail::parse_frame_fec_status(
    std::string_view {reinterpret_cast<const char *>(&status), sizeof(status)},
    10
  ));
}

// ----------------------------------------------------------------------------
// Regression: the audio stream port (AUDIO_STREAM_PORT) must not collide with
// the ENet control port (CONTROL_PORT). Both are offset from
// config::sunshine.port by `map_port(.)`, so a single host that binds them
// sequentially will fail the second bind with EADDRINUSE if the two offsets
// resolve to the same absolute port. The d80b3f9 fix bumped AUDIO_STREAM_PORT
// from 26 to 27 to break the collision; these tests lock the values in so a
// future "tidy up" can't silently re-introduce it.
// ----------------------------------------------------------------------------
TEST(StreamPortConstants, VideoPortIsDistinctFromControlAndAudio) {
  EXPECT_NE(stream::VIDEO_STREAM_PORT, stream::CONTROL_PORT);
  EXPECT_NE(stream::VIDEO_STREAM_PORT, stream::AUDIO_STREAM_PORT);
}

TEST(StreamPortConstants, AudioPortIsDistinctFromControl) {
  // The actual bug fixed in d80b3f9: both CONTROL_PORT and AUDIO_STREAM_PORT
  // were 26, mapping both UDP sockets to the same port. Keep them distinct.
  EXPECT_NE(stream::AUDIO_STREAM_PORT, stream::CONTROL_PORT);
}

TEST(StreamPortConstants, AllPortsInValidRange) {
  // The ports are offsets from config::sunshine.port (default 47989). The
  // range check in map_port() warns if the resulting port is < 1024 or
  // > 65535; with the default base these offsets stay well within bounds.
  EXPECT_GE(stream::VIDEO_STREAM_PORT, 0);
  EXPECT_LE(stream::VIDEO_STREAM_PORT, 65535);
  EXPECT_GE(stream::CONTROL_PORT, 0);
  EXPECT_LE(stream::CONTROL_PORT, 65535);
  EXPECT_GE(stream::AUDIO_STREAM_PORT, 0);
  EXPECT_LE(stream::AUDIO_STREAM_PORT, 65535);
}
