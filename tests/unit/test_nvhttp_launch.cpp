// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_nvhttp_launch.cpp
 * @brief Tests Moonlight-compatible /launch query parsing in nvhttp.
 */
#include "../tests_common.h"

#include <Simple-Web-Server/server_http.hpp>
#include <src/nvhttp.h>
#include <src/rtsp.h>

namespace {

/**
 * @brief Build query arguments matching a typical Moonlight /launch request.
 *
 * @param local_audio_play_mode Value for localAudioPlayMode (0 = stream audio to client).
 * @return Parsed query-string map.
 */
SimpleWeb::CaseInsensitiveMultimap moonlight_launch_args(int local_audio_play_mode) {
  SimpleWeb::CaseInsensitiveMultimap args;
  args.emplace("uniqueid", "0123456789ABCDEF");
  args.emplace("appid", "247389957");
  args.emplace("mode", "2560x1600x60");
  args.emplace("additionalStates", "1");
  args.emplace("sops", "1");
  args.emplace("rikey", "0123456789ABCDEF0123456789ABCDEF");
  args.emplace("rikeyid", "1");
  args.emplace("localAudioPlayMode", std::to_string(local_audio_play_mode));
  args.emplace("surroundAudioInfo", "196610");
  args.emplace("gcmap", "0");
  args.emplace("gcpersist", "0");
  args.emplace("corever", "1");
  args.emplace("hdrMode", "1");
  args.emplace(
    "clientHdrCapDisplayData",
    "0x0x0x0x0x0x0x0x0x0x0"
  );
  return args;
}

}  // namespace

/**
 * @brief Regression for issue #22: localAudioPlayMode=0 must not be rejected.
 */
TEST(NvhttpLaunchTest, AcceptsLocalAudioPlayModeZero) {
  const auto session = nvhttp::test_access::make_launch_session(false, moonlight_launch_args(0));
  ASSERT_TRUE(session);
  EXPECT_EQ(session->width, 2560);
  EXPECT_EQ(session->height, 1600);
  EXPECT_EQ(session->fps, 60);
  EXPECT_EQ(session->appid, 247389957);
  EXPECT_FALSE(session->host_audio);
  EXPECT_TRUE(session->rtsp_cipher.has_value());
}

/**
 * @brief Verify host-audio mode and lenient mode parsing for fractional refresh rates.
 */
TEST(NvhttpLaunchTest, ParsesLenientModeSegments) {
  auto args = moonlight_launch_args(1);
  args.erase("mode");
  args.emplace("mode", "1920x1080x59.94");

  const auto session = nvhttp::test_access::make_launch_session(true, args);
  ASSERT_TRUE(session);
  EXPECT_EQ(session->width, 1920);
  EXPECT_EQ(session->height, 1080);
  EXPECT_EQ(session->fps, 59);
  EXPECT_TRUE(session->host_audio);
}

/**
 * @brief Malformed rikey input should not abort session construction (legacy behavior).
 */
TEST(NvhttpLaunchTest, ToleratesMalformedRikey) {
  auto args = moonlight_launch_args(0);
  args.erase("rikey");
  args.emplace("rikey", "ZZ");

  const auto session = nvhttp::test_access::make_launch_session(false, args);
  ASSERT_TRUE(session);
  EXPECT_TRUE(session->gcm_key.empty());
}
