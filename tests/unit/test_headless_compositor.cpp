/**
 * @file tests/unit/test_headless_compositor.cpp
 * @brief Tests for platf::headless::compositor_t.
 *
 * Only the parts of the compositor that don't require launching a real
 * subprocess (labwc / gamescope / krfb-virtualmonitor) are tested here.
 * The full start()/stop() lifecycle is exercised by integration tests
 * where those binaries are present.
 */
#include "../tests_common.h"

#include <cstdlib>
#include <src/platform/linux/headless_compositor.h>

namespace platf::headless {
  // Forward-declare the testable portions. The header only exposes the
  // public API; we test resolve_backend via set_backend round-trips and
  // direct construction.
}

// Test that set_backend + resolve_backend agree on the user's explicit
// override.
TEST(HeadlessCompositorTest, SetBackendRoundTrips) {
  platf::headless::compositor_t c;
  c.set_backend(platf::headless::backend_e::labwc);
  // resolve_backend honors an explicit override before falling through
  // to environment detection.
  EXPECT_EQ(c.resolve_backend(), platf::headless::backend_e::labwc);

  c.set_backend(platf::headless::backend_e::gamescope);
  EXPECT_EQ(c.resolve_backend(), platf::headless::backend_e::gamescope);

  c.set_backend(platf::headless::backend_e::krfb);
  EXPECT_EQ(c.resolve_backend(), platf::headless::backend_e::krfb);
}

// Default constructor auto-detects. On a CI host without gamescope on
// PATH, no gamescope session, and no KWin session, resolve_backend()
// should pick labwc (the documented default fallback).
TEST(HeadlessCompositorTest, AutoDetectFallsBackToLabwc) {
  // Unset any desktop-detection env vars so is_kwin_running()/is_gamescope_running()
  // both return false on a headless test host.
  unsetenv("XDG_CURRENT_DESKTOP");
  unsetenv("WAYLAND_DISPLAY");

  platf::headless::compositor_t c;
  c.set_backend(platf::headless::backend_e::auto_detect);
  // We don't know for certain whether gamescope / kwin binaries are on
  // PATH in CI; only assert labwc-or-gamescope (the documented fallback
  // order) and never krfb (which requires KWin).
  auto resolved = c.resolve_backend();
  EXPECT_TRUE(resolved == platf::headless::backend_e::labwc ||
              resolved == platf::headless::backend_e::gamescope)
    << "auto-detect should fall back to labwc or gamescope, got "
    << static_cast<int>(resolved);
}

// is_gamescope_running() detects Gamescope via XDG_CURRENT_DESKTOP.
TEST(HeadlessCompositorTest, IsGamescopeRunningDetectsDesktop) {
  unsetenv("WAYLAND_DISPLAY");
  setenv("XDG_CURRENT_DESKTOP", "gamescope", 1);
  EXPECT_TRUE(platf::headless::is_gamescope_running());
  unsetenv("XDG_CURRENT_DESKTOP");
}

// is_gamescope_running() detects Gamescope via WAYLAND_DISPLAY name.
TEST(HeadlessCompositorTest, IsGamescopeRunningDetectsWaylandDisplay) {
  setenv("WAYLAND_DISPLAY", "gamescope-0", 1);
  EXPECT_TRUE(platf::headless::is_gamescope_running());
  unsetenv("WAYLAND_DISPLAY");
}

// output_name() falls back to "HEADLESS-1" when no backend has populated
// _output_name. This is the contract callers rely on.
TEST(HeadlessCompositorTest, OutputNameDefaultsToHeadless1) {
  platf::headless::compositor_t c;
  EXPECT_EQ(c.output_name(), "HEADLESS-1");
}

// pid() returns -1 when no compositor has been started.
TEST(HeadlessCompositorTest, PidDefaultsToMinusOne) {
  platf::headless::compositor_t c;
  EXPECT_EQ(c.pid(), -1);
}

// wayland_socket() and x11_display() return empty strings when no
// compositor has been started.
TEST(HeadlessCompositorTest, SocketsDefaultToEmpty) {
  platf::headless::compositor_t c;
  EXPECT_TRUE(c.wayland_socket().empty());
  EXPECT_TRUE(c.x11_display().empty());
}
