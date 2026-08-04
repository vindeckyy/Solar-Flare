// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_update.cpp
 * @brief Unit tests for SolarFlare SHA256SUMS parsing, version compare, and cancel.
 */
#include "../tests_common.h"

#include <src/update.h>
#include <system_error>

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

TEST(UpdateTest, FormatInstallErrorIncludesRollbackDetail) {
  const std::error_code primary = std::make_error_code(std::errc::permission_denied);
  const std::error_code rollback = std::make_error_code(std::errc::no_such_file_or_directory);

  const auto single = update::format_install_error("Failed to set permissions on staged binary: ", primary);
  EXPECT_NE(single.find("Failed to set permissions on staged binary: "), std::string::npos);
  EXPECT_NE(single.find(primary.message()), std::string::npos);
  EXPECT_EQ(single.find("rollback also failed"), std::string::npos);

  const auto both = update::format_install_error("Failed to install new binary: ", primary, &rollback);
  EXPECT_NE(both.find("Failed to install new binary: "), std::string::npos);
  EXPECT_NE(both.find(primary.message()), std::string::npos);
  EXPECT_NE(both.find("rollback also failed: "), std::string::npos);
  EXPECT_NE(both.find(rollback.message()), std::string::npos);

  const std::error_code ok;
  const auto ignored_ok_rollback = update::format_install_error("Failed to install assets: ", primary, &ok);
  EXPECT_EQ(ignored_ok_rollback.find("rollback also failed"), std::string::npos);
}

TEST(UpdateTest, CancelRejectsIdleAndBusyPhases) {
#ifdef __linux__
  update::test_access::force_phase(update::phase_e::idle, "");
  update::test_access::force_apply_when_idle(false);
  update::test_access::force_worker_running(false);

  {
    const auto err = update::cancel();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, "No update operation is in progress");
  }

  update::test_access::force_phase(update::phase_e::downloading, "Downloading");
  {
    const auto err = update::cancel();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, "No update operation is in progress");
  }

  update::test_access::force_phase(update::phase_e::applying, "Installing");
  {
    const auto err = update::cancel();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, "No update operation is in progress");
  }

  update::test_access::force_phase(update::phase_e::idle, "");
#else
  const auto err = update::cancel();
  ASSERT_TRUE(err.has_value());
  EXPECT_NE(err->find("Linux"), std::string::npos);
#endif
}

TEST(UpdateTest, CancelClearsApplyWhenIdleWhileWaiting) {
#ifndef __linux__
  GTEST_SKIP() << "Linux updater cancel path only";
#else
  update::test_access::force_phase(update::phase_e::waiting_idle, "Waiting for the stream to end");
  update::test_access::force_worker_running(true);
  update::test_access::force_apply_when_idle(true);

  const auto err = update::cancel();
  EXPECT_FALSE(err.has_value()) << (err ? *err : "");
  EXPECT_FALSE(update::test_access::apply_when_idle());

  // Simulate the wait-idle worker observing the cleared flag.
  update::test_access::complete_idle_cancel();
  const auto st = update::status();
  EXPECT_EQ(st.phase, update::phase_e::ready);
  EXPECT_NE(st.message.find("cancelled"), std::string::npos);
  EXPECT_TRUE(st.can_apply);
  EXPECT_FALSE(st.busy);

  update::test_access::force_phase(update::phase_e::idle, "");
  update::test_access::force_worker_running(false);
  update::test_access::force_apply_when_idle(false);
#endif
}

TEST(UpdateTest, CancelCompletesOrphanWaitingIdleWithoutWorker) {
#ifndef __linux__
  GTEST_SKIP() << "Linux updater cancel path only";
#else
  update::test_access::force_phase(update::phase_e::waiting_idle, "Waiting for the stream to end");
  update::test_access::force_worker_running(false);
  update::test_access::force_apply_when_idle(true);

  const auto err = update::cancel();
  EXPECT_FALSE(err.has_value()) << (err ? *err : "");
  EXPECT_FALSE(update::test_access::apply_when_idle());

  const auto st = update::status();
  EXPECT_EQ(st.phase, update::phase_e::ready);
  EXPECT_NE(st.message.find("cancelled"), std::string::npos);
  EXPECT_TRUE(st.can_apply);
  EXPECT_FALSE(st.busy);

  update::test_access::force_phase(update::phase_e::idle, "");
  update::test_access::force_apply_when_idle(false);
#endif
}

TEST(UpdateTest, IdleApplyHonorsCancelBeforeClaiming) {
#ifndef __linux__
  GTEST_SKIP() << "Linux updater cancel path only";
#else
  update::test_access::force_phase(update::phase_e::waiting_idle, "Waiting for the stream to end");
  update::test_access::force_worker_running(true);
  update::test_access::force_apply_when_idle(false);

  update::test_access::apply_now_from_idle();

  const auto st = update::status();
  EXPECT_EQ(st.phase, update::phase_e::ready);
  EXPECT_NE(st.message.find("cancelled"), std::string::npos);
  EXPECT_TRUE(st.can_apply);
  EXPECT_FALSE(st.busy);

  update::test_access::force_phase(update::phase_e::idle, "");
  update::test_access::force_worker_running(false);
  update::test_access::force_apply_when_idle(false);
#endif
}

TEST(UpdateTest, CancelAcceptsReadyPhase) {
#ifndef __linux__
  GTEST_SKIP() << "Linux updater cancel path only";
#else
  update::test_access::force_phase(update::phase_e::ready, "Update staged and verified");
  update::test_access::force_worker_running(false);
  update::test_access::force_apply_when_idle(false);

  const auto err = update::cancel();
  EXPECT_FALSE(err.has_value()) << (err ? *err : "");
  EXPECT_EQ(update::status().phase, update::phase_e::ready);

  update::test_access::force_phase(update::phase_e::idle, "");
#endif
}
