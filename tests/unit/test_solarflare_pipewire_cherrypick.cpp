/**
 * @file tests/unit/test_solarflare_pipewire_cherrypick.cpp
 * @brief Regression guard for the round-4 cherry-pick of
 *        4e6e1377 feat(linux/pipewire): add fallback to node id.
 *
 * Upstream commit 4e6e1377 added a runtime-gated path that tries to
 * connect via pipewire object serial first, then falls back to the
 * legacy node-id path if the object-serial connect fails. The
 * cherry-pick was applied to the fork in round 4 with a manual
 * conflict resolution in src/platform/linux/pipewire.cpp.
 *
 * These tests are build-time guards that verify the cherry-pick is
 * still in place -- the SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL macro
 * must be defined and used in the connect path, and the
 * fork-specific PW_KEY_NODE_LATENCY block (added in round 1) must
 * coexist with the new fallback logic.
 *
 * If a future commit drops or inverts the fallback logic, these
 * tests fail with a clear error message pointing at the round-4
 * cherry-pick so the maintainer can re-apply the fix.
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
// 1. The SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL macro is defined.
// =============================================================================

TEST(SolarflarePipewireCherryPick, ObjectSerialMacroIsDefined) {
  const auto content = read_file("src/platform/linux/pipewire.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/pipewire.cpp. The pipewire "
       "source file is missing or the test build is misconfigured.";

  // The macro has three possible definitions depending on the
  // pipewire version: pw_check_library_version(0,3,64) (runtime),
  // constexpr true (header-only), or constexpr false (no support).
  // All three are valid. We just check that at least one of them
  // is present, which proves the round-4 cherry-pick is in place.
  const bool hasRuntimeCheck = contains(content, "pw_check_library_version(0, 3, 64)");
  const bool hasConstexprTrue = contains(content, "constexpr bool SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL = true");
  const bool hasConstexprFalse = contains(content, "constexpr bool SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL = false");
  const bool hasConstDecl = contains(content, "const bool SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL = pw_check_library_version");

  EXPECT_TRUE(hasRuntimeCheck || hasConstexprTrue || hasConstexprFalse || hasConstDecl)
    << "SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL is not defined anywhere "
       "in src/platform/linux/pipewire.cpp. The round-4 cherry-pick "
       "of 4e6e1377 (feat(linux/pipewire): add fallback to node id) "
       "has been reverted or stripped. Re-apply the upstream commit "
       "to restore the runtime-gated object-serial-vs-node-id "
       "fallback logic.";
}

// =============================================================================
// 2. The macro is actually USED in the connect path (not just defined).
// =============================================================================

TEST(SolarflarePipewireCherryPick, ObjectSerialMacroIsUsedInConnect) {
  const auto content = read_file("src/platform/linux/pipewire.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/pipewire.cpp.";

  // The connect path that USES the macro is the one inside
  // ensure_stream that decides whether to call pw_stream_connect
  // with PW_ID_ANY (object-serial path) vs node (legacy path).
  // The conditional looks like:
  //   if (SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL && (object_serial & ...
  EXPECT_TRUE(contains(content, "if (SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL"))
    << "SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL is defined but never "
       "used in the connect path. The 4e6e1377 cherry-pick is "
       "incomplete -- the macro exists but the conditional that "
       "decides between object-serial and node-id is missing. "
       "Re-apply the connect-path logic from 4e6e1377.";
}

// =============================================================================
// 3. The fork-specific PW_KEY_NODE_LATENCY block coexists with the new
//    fallback logic. This is the key invariant of the round-4
//    conflict resolution: BOTH the old fork block (which sets the
//    node latency) AND the new upstream fallback (which tries
//    object serial first) must be present.
// =============================================================================

TEST(SolarflarePipewireCherryPick, ForkLatencyBlockCoexists) {
  const auto content = read_file("src/platform/linux/pipewire.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/pipewire.cpp.";

  // The fork-specific block sets PW_KEY_NODE_LATENCY from
  // config::solarflare.pipewire_latency_ms. It must still be in
  // the file alongside the new upstream fallback logic.
  EXPECT_TRUE(contains(content, "config::solarflare.pipewire_latency_ms"))
    << "src/platform/linux/pipewire.cpp no longer references "
       "config::solarflare.pipewire_latency_ms. The fork's "
       "PW_KEY_NODE_LATENCY hint (added in round 1) has been "
       "stripped -- probably by an over-eager rebase. Re-apply the "
       "fork's block in ensure_stream (see docs/CONFIGURATION.md "
       "for the ranges).";

  // And it must use the right format (fraction string in nanoseconds).
  EXPECT_TRUE(contains(content, "ms * 1000000) + \"/1000\""))
    << "The fork's PW_KEY_NODE_LATENCY format string changed "
       "unexpectedly. Should be '<ms * 1_000_000>/1000' which gives "
       "a fraction string like '8192/1000' for 8 ms. Revert to the "
       "round-1 implementation.";
}

// =============================================================================
// 4. The PW_KEY_TARGET_OBJECT old block is NOT in the file (it was
//    removed by 4e6e1377 and moved to the runtime-gated macro).
//    This is a build-time guard against accidentally reintroducing
//    the old unconditional PW_KEY_TARGET_OBJECT handling.
// =============================================================================

TEST(SolarflarePipewireCherryPick, OldObjectSerialBlockRemoved) {
  const auto content = read_file("src/platform/linux/pipewire.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/pipewire.cpp.";

  // The old (pre-4e6e1377) code path was an unconditional:
  //   #ifdef PW_KEY_TARGET_OBJECT
  //   if ((object_serial & SPA_ID_INVALID) != SPA_ID_INVALID) {
  //     pw_properties_setf(props, PW_KEY_TARGET_OBJECT, ...);
  //     node = PW_ID_ANY;
  //   }
  //   #endif
  // The new (post-4e6e1377) code wraps this in the
  // SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL runtime check. If the old
  // unconditional block was accidentally reintroduced (e.g. by a
  // revert of the cherry-pick), this test catches it.
  EXPECT_FALSE(contains(content,
    "#ifdef PW_KEY_TARGET_OBJECT\n        if ((object_serial & SPA_ID_INVALID)"))
    << "src/platform/linux/pipewire.cpp contains the OLD "
       "unconditional PW_KEY_TARGET_OBJECT block (pre-4e6e1377). "
       "The cherry-pick moved this logic inside a "
       "SUNSHINE_USE_PIPEWIRE_OBJECT_SERIAL runtime check so it "
       "can fall back to the legacy node-id path on older pipewire "
       "versions. The old block bypasses that fallback; remove it "
       "and use the runtime-gated form instead.";
}
