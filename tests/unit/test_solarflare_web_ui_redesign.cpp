// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_solarflare_web_ui_redesign.cpp
 * @brief Regression tests for the SolarFlare observatory web interface.
 *
 * These source-level guards protect the shared navigation rail, dashboard
 * command deck, non-looping motion policy, responsive breakpoint, and global
 * theme initialization. They intentionally test stable design contracts
 * rather than generated Vite asset names.
 */

#include "../tests_common.h"

#include <string>

namespace {

  /**
   * @brief Determine whether a source file contains a design contract marker.
   *
   * @param source Source file contents.
   * @param marker Marker expected in the source file.
   * @return `true` when @p marker occurs in @p source; otherwise `false`.
   */
  bool contains_marker(const std::string &source, const std::string &marker) {
    return source.find(marker) != std::string::npos;
  }

}  // namespace

TEST(SolarflareWebUIRedesign, NavbarUsesSemanticObservatoryRail) {
  const auto navbar = test_utils::read_repo_file("src_assets/common/assets/web/Navbar.vue");

  ASSERT_FALSE(navbar.empty());
  EXPECT_TRUE(contains_marker(navbar, "aria-label=\"Primary navigation\""));
  EXPECT_TRUE(contains_marker(navbar, "class=\"collapse navbar-collapse sf-nav-content\""));
  EXPECT_TRUE(contains_marker(navbar, "class=\"sf-node-readout\""));
  EXPECT_TRUE(contains_marker(navbar, "data-nav-code=\"06\""));
}

TEST(SolarflareWebUIRedesign, DashboardUsesCommandDeckAndRealHostTelemetry) {
  const auto dashboard = test_utils::read_repo_file("src_assets/common/assets/web/index.html");

  ASSERT_FALSE(dashboard.empty());
  EXPECT_TRUE(contains_marker(dashboard, "class=\"sf-command-deck my-4\""));
  EXPECT_TRUE(contains_marker(dashboard, "aria-labelledby=\"sf-dashboard-title\""));
  EXPECT_TRUE(contains_marker(dashboard, "{{ solarflareStatusText }}"));
  EXPECT_TRUE(contains_marker(dashboard, "{{ platform || 'Detecting' }}"));
  EXPECT_TRUE(contains_marker(dashboard, "href=\"./pin\""));
  EXPECT_TRUE(contains_marker(dashboard, "href=\"./apps\""));
}

TEST(SolarflareWebUIRedesign, DesignSystemHasRailResponsiveAndMotionContracts) {
  const auto css = test_utils::read_repo_file("src_assets/common/assets/web/sunshine.css");

  ASSERT_FALSE(css.empty());
  EXPECT_TRUE(contains_marker(css, "SolarFlare Observatory Interface"));
  EXPECT_TRUE(contains_marker(css, "--sf-rail-width: 17.25rem"));
  EXPECT_TRUE(contains_marker(css, "@media (max-width: 991.98px)"));
  EXPECT_TRUE(contains_marker(css, ".sf-command-deck"));
  EXPECT_TRUE(contains_marker(css, "animation: none !important"));
}

TEST(SolarflareWebUIRedesign, EveryEntryPointInitializesTheSharedTheme) {
  const auto init = test_utils::read_repo_file("src_assets/common/assets/web/init.js");
  const auto theme = test_utils::read_repo_file("src_assets/common/assets/web/theme.js");
  const auto welcome = test_utils::read_repo_file("src_assets/common/assets/web/welcome.html");

  ASSERT_FALSE(init.empty());
  ASSERT_FALSE(theme.empty());
  ASSERT_FALSE(welcome.empty());
  EXPECT_TRUE(contains_marker(init, "import { loadAutoTheme } from './theme'"));
  EXPECT_TRUE(contains_marker(init, "loadAutoTheme()"));
  EXPECT_TRUE(contains_marker(theme, "? 'solarflare' : 'solarflare-light'"));
  EXPECT_TRUE(contains_marker(welcome, "class=\"sf-auth-stage\""));
}

TEST(SolarflareWebUIRedesign, PresentationCopyNormalizesUpstreamBranding) {
  const auto locale = test_utils::read_repo_file("src_assets/common/assets/web/locale.js");
  const auto palette = test_utils::read_repo_file("src_assets/common/assets/web/CommandPalette.vue");
  const auto general = test_utils::read_repo_file("src_assets/common/assets/web/configs/tabs/General.vue");
  const auto welcome = test_utils::read_repo_file("src_assets/common/assets/web/welcome.html");

  ASSERT_FALSE(locale.empty());
  EXPECT_TRUE(contains_marker(locale, "function brandMessageTree(value)"));
  EXPECT_TRUE(contains_marker(locale, ".replaceAll(/\\bSunshine\\b/g, 'SolarFlare')"));
  EXPECT_TRUE(contains_marker(locale, "key === 'theme_sunshine' ? 'SolarFlare Daylight'"));
  EXPECT_FALSE(contains_marker(palette, "restart Sunshine?"));
  EXPECT_FALSE(contains_marker(palette, "quit Sunshine?"));
  EXPECT_TRUE(contains_marker(general, "placeholder=\"SolarFlare\""));
  EXPECT_TRUE(contains_marker(welcome, "newUsername: \"solarflare\""));
}

TEST(SolarflareWebUIRedesign, FeaturedCatalogIsLocalAndDoesNotFetchThirdPartyData) {
  const auto featured = test_utils::read_repo_file("src_assets/common/assets/web/featured.html");

  ASSERT_FALSE(featured.empty());
  EXPECT_TRUE(contains_marker(featured, "const FEATURED_APPS = Object.freeze"));
  EXPECT_TRUE(contains_marker(featured, "id: 'moonlight-pc'"));
  EXPECT_TRUE(contains_marker(featured, "this.apps = FEATURED_APPS.map"));
  EXPECT_FALSE(contains_marker(featured, "fetch(indexUrl)"));
  EXPECT_FALSE(contains_marker(featured, "app.lizardbyte.dev"));
}
