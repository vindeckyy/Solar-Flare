/**
 * @file tests/unit/test_solarflare_a84735d1_cherrypick.cpp
 * @brief Regression guard for the round-7 cherry-pick of
 *        a84735d1 fix(web-ui): don't open ui automatically on app start.
 *
 * Upstream commit a84735d1 removed an unconditional launch_ui() call
 * from src/config.cpp's Windows shortcut_admin path. Before the fix,
 * the shortcut_admin path (which restarts Sunshine as admin to start
 * the service) ALSO opened the web UI in the user's default browser.
 * This was a UX surprise: the user just wanted the service started,
 * not the UI opened. The fix removed the launch_ui() call.
 *
 * The cherry-pick was applied in round 7 to src/config.cpp. The
 * surrounding code on both sides of the removed lines is the
 * `service_ctrl::is_service_running()` branch inside the
 * `service_admin_launch` block of the Windows-only (#ifdef _WIN32)
 * code path.
 *
 * These tests are build-time guards: if a future commit reintroduces
 * the launch_ui() call (e.g. by reverting the cherry-pick), these
 * tests fail with a clear error message pointing at the round-7
 * cherry-pick so the maintainer can re-apply the fix.
 *
 * The cherry-pick also deleted packaging/linux/flatpak/scripts/sunshine.sh
 * (the Flatpak wrapper that opened the UI). That file is gone from
 * the fork entirely, so we can't test for it; we just verify the
 * src/config.cpp half.
 */
#include "../tests_common.h"

#include "src/file_handler.h"

#include <string>

namespace {

  std::string read_file(const std::string &path) {
    return file_handler::read_file(path.c_str());
  }

  bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
  }

}  // namespace

// =============================================================================
// 1. The launch_ui() call is GONE from the service_admin_launch branch.
//    This is the key invariant of a84735d1: the old line
//    '// Launch the web UI\n      launch_ui();' should NOT exist in
//    src/config.cpp anymore.
// =============================================================================

TEST(SolarflareWebUIFixCherryPick, LaunchUiCallRemovedFromShortcutAdmin) {
  const auto content = read_file("src/config.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/config.cpp. The test build is "
       "misconfigured.";

  // The pre-a84735d1 lines were:
  //   // Launch the web UI
  //   launch_ui();
  // Both must be gone.
  EXPECT_FALSE(contains(content, "// Launch the web UI"))
    << "src/config.cpp contains the old '// Launch the web UI' "
       "comment that a84735d1 removed. The launch_ui() call is "
       "still being triggered when Sunshine starts as a "
       "Windows shortcut. Re-apply the round-7 cherry-pick of "
       "a84735d1 to remove the launch_ui() call from the "
       "service_admin_launch branch.";
  EXPECT_FALSE(contains(content, "launch_ui();"))
    << "src/config.cpp contains a bare 'launch_ui();' call. "
       "This was removed by the a84735d1 cherry-pick. Re-apply "
       "the cherry-pick.";
}

// =============================================================================
// 2. The 'service_ctrl::wait_for_ui_ready()' call IS still present
//    (the comment '// Wait for the UI to be ready for connections'
//    precedes it). This is the replacement behaviour: the code now
//    waits for the UI to be ready but does NOT open the UI itself.
//    If both are present, the cherry-pick is intact.
// =============================================================================

TEST(SolarflareWebUIFixCherryPick, WaitForUiReadyStillPresent) {
  const auto content = read_file("src/config.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/config.cpp.";

  EXPECT_TRUE(contains(content, "service_ctrl::wait_for_ui_ready()"))
    << "src/config.cpp no longer calls service_ctrl::wait_for_ui_ready()."
       " This is the replacement for the old launch_ui() call -- the "
       "code should wait for the UI to be ready but NOT open it. "
       "Re-apply the a84735d1 cherry-pick.";
  EXPECT_TRUE(contains(content, "Wait for the UI to be ready for connections"))
    << "src/config.cpp is missing the 'Wait for the UI to be ready' "
       "comment. This comment was retained by the a84735d1 "
       "cherry-pick; if it's been lost, the surrounding code "
       "structure has changed too. Investigate.";
}

// =============================================================================
// 3. The 'Always return 1 to ensure Sunshine doesn't start normally'
//    return statement is still in place. The a84735d1 cherry-pick
//    REPLACED the launch_ui() with 'return 1' (the shortcut path
//    exits without starting the main loop), so the return 1 must
//    still be there.
// =============================================================================

TEST(SolarflareWebUIFixCherryPick, ShortcutAdminReturn1StillPresent) {
  const auto content = read_file("src/config.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/config.cpp.";

  // The comment 'Always return 1 to ensure Sunshine doesn't start normally'
  // should be present near the service_admin_launch block.
  EXPECT_TRUE(contains(content, "Always return 1 to ensure Sunshine doesn't start normally"))
    << "src/config.cpp is missing the 'Always return 1 to ensure "
       "Sunshine doesn't start normally' comment. This is the "
       "return-1 that the a84735d1 cherry-pick left in place after "
       "removing launch_ui(). If it's been lost, the surrounding "
       "shortcut_admin path may have been refactored to start the "
       "main loop, which would re-introduce the bug. Investigate.";
}
