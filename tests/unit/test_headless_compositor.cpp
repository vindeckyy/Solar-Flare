// SPDX-License-Identifier: GPL-3.0-only

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
  // ponytail: with no env-var signal, resolve_backend must pick labwc
  // deterministically. The old "labwc-or-gamescope" assertion came from the
  // gamescope-on-PATH false-positive branch that we removed in this sweep.
  unsetenv("XDG_CURRENT_DESKTOP");
  unsetenv("XDG_SESSION_DESKTOP");
  unsetenv("WAYLAND_DISPLAY");

  platf::headless::compositor_t c;
  c.set_backend(platf::headless::backend_e::auto_detect);
  auto resolved = c.resolve_backend();
  EXPECT_EQ(resolved, platf::headless::backend_e::labwc)
    << "auto-detect with no env-var signal must fall back to labwc, got "
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

// niri detection: XDG_CURRENT_DESKTOP=niri is the strongest signal.
// niri also sets XDG_SESSION_DESKTOP=niri in some setups.
TEST(HeadlessCompositorTest, IsNiriRunningDetectsXdgCurrentDesktop) {
  unsetenv("XDG_SESSION_DESKTOP");
  setenv("XDG_CURRENT_DESKTOP", "niri", 1);
  EXPECT_TRUE(platf::headless::is_niri_running());
  unsetenv("XDG_CURRENT_DESKTOP");
}

// niri detection: env-var-only contract. The old binary-on-PATH branch was
// removed because a CachyOS box with niri INSTALLED but not running would
// false-positive, leading auto-detect to bind to a non-existent session.
TEST(HeadlessCompositorTest, IsNiriRunningIgnoresBinaryOnPath) {
  unsetenv("XDG_CURRENT_DESKTOP");
  unsetenv("XDG_SESSION_DESKTOP");
  EXPECT_FALSE(platf::headless::is_niri_running());
}

// niri detection: XDG_SESSION_DESKTOP fallback (covers gdm/wayland-session
// environments where the session id is set but the desktop id is not).
TEST(HeadlessCompositorTest, IsNiriRunningDetectsSessionDesktop) {
  unsetenv("XDG_CURRENT_DESKTOP");
  setenv("XDG_SESSION_DESKTOP", "niri", 1);
  EXPECT_TRUE(platf::headless::is_niri_running());
  unsetenv("XDG_SESSION_DESKTOP");
}

// niri detection: env-var contract holds regardless of PATH state.
TEST(HeadlessCompositorTest, IsNiriRunningReturnsFalseWhenAbsent) {
  unsetenv("XDG_CURRENT_DESKTOP");
  unsetenv("XDG_SESSION_DESKTOP");
  EXPECT_FALSE(platf::headless::is_niri_running());
}

// resolve_backend() picks niri when is_niri_running() returns true.
TEST(HeadlessCompositorTest, AutoDetectPicksNiri) {
  unsetenv("WAYLAND_DISPLAY");
  unsetenv("XDG_SESSION_DESKTOP");
  setenv("XDG_CURRENT_DESKTOP", "niri", 1);
  platf::headless::compositor_t c;
  c.set_backend(platf::headless::backend_e::auto_detect);
  EXPECT_EQ(c.resolve_backend(), platf::headless::backend_e::niri);
  unsetenv("XDG_CURRENT_DESKTOP");
}

// SetBackendRoundTrips extension: niri override is honored.
TEST(HeadlessCompositorTest, SetBackendNiriRoundTrips) {
  platf::headless::compositor_t c;
  c.set_backend(platf::headless::backend_e::niri);
  EXPECT_EQ(c.resolve_backend(), platf::headless::backend_e::niri);
}
