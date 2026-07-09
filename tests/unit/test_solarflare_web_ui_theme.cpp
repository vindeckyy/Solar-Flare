/**
 * @file tests/unit/test_solarflare_web_ui_theme.cpp
 * @brief Regression tests for the SolarFlare fork's web UI theme
 *        integration.
 *
 * Background: when round 7 cherry-picked upstream commit
 * 3266c341 (feat(web-ui): UI consistency / layout uplifts), the
 * upstream web UI split the theme selector into separate "Dark
 * Themes" and "Light Themes" groups. The cherry-pick conflicted in
 * ThemeToggle.vue and sunshine.css because the fork had added its
 * own "SolarFlare" theme that the upstream didn't know about.
 *
 * Manual conflict resolution: kept the upstream's new grouped
 * structure AND kept the fork's solarflare theme button (in the
 * Dark Themes group, before the Light Themes divider) AND kept the
 * solarflare-light theme button (same group, distinct from the
 * regular solarflare dark one).
 *
 * These tests are build-time guards: if a future cherry-pick or
 * theme-grouping refactor re-arranges the dropdown or drops the
 * SolarFlare entries, these tests fail with a clear error message
 * pointing at the original cherry-pick round.
 *
 * Coverage:
 * 1. The SolarFlare theme button is in ThemeToggle.vue (both dark
 *    and light variants).
 * 2. The button is inside the "Dark Themes" group, not the "Light
 *    Themes" group.
 * 3. en.json has both `theme_solarflare` and `theme_solarflare_light`
 *    labels.
 * 4. sunshine.css has CSS rules for both `[data-theme="solarflare"]`
 *    and `[data-theme="solarflare-light"]`.
 */
#include "../tests_common.h"

#include "src/file_handler.h"

#include <string>

namespace {

  // Read a file from the repo and return its contents. The tests run
  // from the build/ directory, so we use a project-relative path via
  // the CMake-injected CWD (we set the working dir in tests/CMakeLists.txt
  // to the source tree root via configure_file copies).
  std::string read_file(const std::string &path) {
    return file_handler::read_file(path.c_str());
  }

  bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
  }

  // Find the index of `marker` in `text`, or std::string::npos.
  size_t find_or_npos(const std::string &text, const std::string &marker) {
    return text.find(marker);
  }

}  // namespace

// =============================================================================
// 1. ThemeToggle.vue contains the SolarFlare theme buttons.
// =============================================================================

TEST(SolarflareWebUITheme, ThemeToggleHasSolarflareDarkButton) {
  const auto content = read_file("src_assets/common/assets/web/ThemeToggle.vue");
  EXPECT_TRUE(contains(content, "data-bs-theme-value=\"solarflare\""))
    << "ThemeToggle.vue is missing the solarflare theme button. "
       "This was added in the round-7 conflict resolution of "
       "3266c341. If a future cherry-pick dropped it, re-add the "
       "fork's SolarFlare entry to the Dark Themes group.";
}

TEST(SolarflareWebUITheme, ThemeToggleHasSolarflareLightButton) {
  const auto content = read_file("src_assets/common/assets/web/ThemeToggle.vue");
  EXPECT_TRUE(contains(content, "data-bs-theme-value=\"solarflare-light\""))
    << "ThemeToggle.vue is missing the solarflare-light theme "
       "button. This was added in round 7 alongside solarflare.";
}

TEST(SolarflareWebUITheme, ThemeToggleHasSolarflareLabels) {
  const auto content = read_file("src_assets/common/assets/web/ThemeToggle.vue");
  EXPECT_TRUE(contains(content, "navbar.theme_solarflare"))
    << "ThemeToggle.vue is missing the navbar.theme_solarflare "
       "i18n key. Add it to the fork's en.json and any other "
       "locale the fork maintains.";
  EXPECT_TRUE(contains(content, "navbar.theme_solarflare_light"))
    << "ThemeToggle.vue is missing the navbar.theme_solarflare_light "
       "i18n key. Add it to the fork's en.json and any other locale.";
}

// =============================================================================
// 2. The SolarFlare button is in the Dark Themes group, not the Light group.
//    This is the key property of the round-7 conflict resolution:
//    "SolarFlare" is a dark theme, so it must be in the Dark group
//    before the <!-- Light Themes --> divider.
// =============================================================================

TEST(SolarflareWebUITheme, SolarflareInDarkThemesGroup) {
  const auto content = read_file("src_assets/common/assets/web/ThemeToggle.vue");

  const size_t dark_group_start = find_or_npos(content, "<!-- Dark Themes -->");
  const size_t light_group_start = find_or_npos(content, "<!-- Light Themes -->");
  const size_t solarflare_pos = find_or_npos(content, "data-bs-theme-value=\"solarflare\"");

  ASSERT_NE(dark_group_start, std::string::npos)
    << "ThemeToggle.vue is missing the '<!-- Dark Themes -->' "
       "section header. The 3266c341 cherry-pick structure was lost.";
  ASSERT_NE(light_group_start, std::string::npos)
    << "ThemeToggle.vue is missing the '<!-- Light Themes -->' "
       "section header. The 3266c341 cherry-pick structure was lost.";
  ASSERT_NE(solarflare_pos, std::string::npos)
    << "ThemeToggle.vue is missing the solarflare theme entry. "
       "Re-add it (see other tests for details).";

  // solarflare must be between Dark Themes start and Light Themes start
  EXPECT_GT(solarflare_pos, dark_group_start)
    << "SolarFlare theme button is ABOVE the '<!-- Dark Themes -->' "
       "section. It must be INSIDE the Dark Themes group (it's a dark "
       "theme). This is the round-7 conflict resolution guard.";
  EXPECT_LT(solarflare_pos, light_group_start)
    << "SolarFlare theme button is in the Light Themes group, but "
       "it's a dark theme. Move it before the '<!-- Light Themes -->' "
       "divider. This is the round-7 conflict resolution guard.";
}

// =============================================================================
// 3. en.json has both translation keys with the expected display strings.
// =============================================================================

TEST(SolarflareWebUITheme, EnJsonHasSolarflareTranslationKeys) {
  // file_handler::read_file returns empty string on read failure, so
  // we use ASSERT to bail out early if the file can't be read.
  const auto content = read_file("src_assets/common/assets/web/public/assets/locale/en.json");
  ASSERT_FALSE(content.empty())
    << "Could not read en.json. Are tests running from the right "
       "working directory? tests/CMakeLists.txt configures the build "
       "to copy src_assets/ next to the test binary.";

  // The two keys should be inside the `navbar` object. We don't
  // parse JSON to keep the test simple and resilient to minor
  // formatting changes; substring match is enough.
  EXPECT_TRUE(contains(content, "\"theme_solarflare\": \"SolarFlare\""))
    << "en.json is missing navbar.theme_solarflare = \"SolarFlare\". "
       "This is the round-7 cherry-pick of 3266c341 that added the "
       "i18n key for the new theme. Re-add it.";
  EXPECT_TRUE(contains(content, "\"theme_solarflare_light\": \"SolarFlare Light\""))
    << "en.json is missing navbar.theme_solarflare_light = "
       "\"SolarFlare Light\". This was added in round 7 alongside "
       "the solarflare dark entry. Re-add it.";
}

// =============================================================================
// 4. sunshine.css has CSS rules for both [data-theme="solarflare"] and
//    [data-theme="solarflare-light"] so the themes actually render.
// =============================================================================

TEST(SolarflareWebUITheme, CssHasSolarflareDarkTheme) {
  const auto content = read_file("src_assets/common/assets/web/sunshine.css");
  ASSERT_FALSE(content.empty())
    << "Could not read sunshine.css. The web UI assets may not have "
       "been copied to the test build dir.";

  EXPECT_TRUE(contains(content, "[data-theme=\"solarflare\"]"))
    << "sunshine.css is missing the [data-theme=\"solarflare\"] "
       "block. The SolarFlare dark theme CSS is what makes the "
       "navbar and theme picker actually render the fork's brand "
       "identity (deep slate with solar-flare accents). Re-add it "
       "(see round 7's conflict resolution for the exact block).";
}

TEST(SolarflareWebUITheme, CssHasSolarflareLightTheme) {
  const auto content = read_file("src_assets/common/assets/web/sunshine.css");
  ASSERT_FALSE(content.empty())
    << "Could not read sunshine.css. The web UI assets may not have "
       "been copied to the test build dir.";

  EXPECT_TRUE(contains(content, "[data-theme=\"solarflare-light\"]"))
    << "sunshine.css is missing the [data-theme=\"solarflare-light\"] "
       "block. The SolarFlare light theme is a daytime variant of "
       "the brand identity (warm amber accents). Re-add it.";
}
