// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_update.cpp
 * @brief Unit tests for SolarFlare SHA256SUMS parsing and version compare.
 */
#include "../tests_common.h"

#include <src/update.h>

TEST(UpdateTest, ParsesSha256SumsBasenames) {
  const auto map = update::parse_sha256sums(
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  solarflare-linux-x86_64.tar.gz\n"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb *SHA256SUMS\n"
    "# comment\n"
    "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc  path/to/sunshine-x86_64\n"
  );

  ASSERT_EQ(map.size(), 3u);
  EXPECT_EQ(map.at("solarflare-linux-x86_64.tar.gz"), "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  EXPECT_EQ(map.at("SHA256SUMS"), "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  EXPECT_EQ(map.at("sunshine-x86_64"), "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");
}

TEST(UpdateTest, CompareVersionsOrdersBuildTags) {
  EXPECT_LT(update::compare_versions("v2026.726.1-solarflare", "v2026.728.1-solarflare"), 0);
  EXPECT_GT(update::compare_versions("2026.729.1", "2026.728.1"), 0);
  EXPECT_EQ(update::compare_versions("v2026.728.1-solarflare", "2026.728.1"), 0);
}

TEST(UpdateTest, ApplyHelperPathIsStable) {
#ifdef SUNSHINE_UPDATE_HELPER_PATH
  EXPECT_EQ(update::apply_helper_path(), SUNSHINE_UPDATE_HELPER_PATH);
#else
  EXPECT_EQ(update::apply_helper_path(), "/usr/local/libexec/solarflare-update-apply");
#endif
}

TEST(UpdateTest, PhaseTokensAreStable) {
  EXPECT_EQ(update::to_string(update::phase_e::downloading), "downloading");
  EXPECT_EQ(update::to_string(update::phase_e::waiting_idle), "waiting_idle");
  const auto json = update::to_json(update::status());
  ASSERT_TRUE(json.contains("phase"));
  ASSERT_TRUE(json.contains("log"));
  ASSERT_TRUE(json.contains("helper_path"));
}
