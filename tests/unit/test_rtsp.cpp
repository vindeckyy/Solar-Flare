// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_rtsp.cpp
 * @brief Tests RTSP ANNOUNCE SDP parsing.
 */
#include "../tests_common.h"

#include <src/rtsp.h>

TEST(RtspAnnouncePayloadTest, AcceptsEmptyAttributeValues) {
  const auto parsed = rtsp_stream::parse_announce_payload(
    "s=Moonlight\r\n"
    "a=x-nv-video[0].packetSize:\r\n"
    "a=x-nv-video[0].maxFPS:60 \r\n"
  );

  ASSERT_TRUE(parsed);
  EXPECT_EQ(parsed->client, "Moonlight");
  EXPECT_EQ(parsed->attributes.at("x-nv-video[0].packetSize"), "");
  EXPECT_EQ(parsed->attributes.at("x-nv-video[0].maxFPS"), "60");
}

TEST(RtspAnnouncePayloadTest, RejectsAttributesWithoutASeparator) {
  EXPECT_FALSE(rtsp_stream::parse_announce_payload("a=x-nv-video[0].packetSize\r\n"));
}
