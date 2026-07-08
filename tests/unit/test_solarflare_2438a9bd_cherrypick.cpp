/**
 * @file tests/unit/test_solarflare_2438a9bd_cherrypick.cpp
 * @brief Regression guard for the round-3 cherry-pick of
 *        2438a9bd feat(linux/xdgportal): Add support for pipewire-serial.
 *
 * Upstream commit 2438a9bd added pipewire-serial support to the
 * XDG Desktop Portal capture path (src/platform/linux/portalgrab.cpp).
 * Before the fix, the portal capture only knew about pipewire NODE
 * IDs; the new code reads the optional "pipewire-serial" key from
 * the portal's D-Bus reply and threads it through to the
 * pipewire_streaminfo_t struct, where it eventually gets passed to
 * pw_stream_connect for the object-serial connect path.
 *
 * The cherry-pick was applied in round 3 with auto-merge (no
 * conflict, the fork hadn't touched portalgrab.cpp).
 *
 * These tests are build-time guards: if a future commit drops the
 * pipewire-serial plumbing (e.g. by reverting the cherry-pick), these
 * tests fail with a clear error message pointing at the round-3
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
// 1. The pipewire_streaminfo_t struct has the object_serial field.
//    The pre-2438a9bd struct only had pipewire_node. The cherry-pick
//    added a uint64_t pipewire_object_serial field (initialised to
//    SPA_ID_INVALID).
// =============================================================================

TEST(SolarflarePortalPipewireCherryPick, StreaminfoHasObjectSerialField) {
  const auto content = read_file("src/platform/linux/portalgrab.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/portalgrab.cpp.";

  EXPECT_TRUE(contains(content, "uint64_t pipewire_object_serial = SPA_ID_INVALID"))
    << "src/platform/linux/portalgrab.cpp is missing the "
       "uint64_t pipewire_object_serial field on the "
       "pipewire_streaminfo_t struct. The 2438a9bd cherry-pick "
       "added this so the portal capture can pass an object serial "
       "(not just a node id) to pw_stream_connect. Re-apply the "
       "cherry-pick.";
}

// =============================================================================
// 2. The portal D-Bus reply parses the "pipewire-serial" key.
//    This is the core of 2438a9bd: the new g_variant_lookup call
//    that reads the optional "pipewire-serial" key.
// =============================================================================

TEST(SolarflarePortalPipewireCherryPick, PortalParsesPipewireSerialKey) {
  const auto content = read_file("src/platform/linux/portalgrab.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/portalgrab.cpp.";

  // The new code path:
  //   uint64_t out_pipewire_object_serial;
  //   result = g_variant_lookup(value, "pipewire-serial", "t", &out_pipewire_object_serial);
  EXPECT_TRUE(contains(content, "g_variant_lookup(value, \"pipewire-serial\""))
    << "src/platform/linux/portalgrab.cpp is missing the "
       "g_variant_lookup(value, \"pipewire-serial\", ...) call. "
       "This is the core of the 2438a9bd cherry-pick: the portal "
       "capture now reads the pipewire-serial key from the D-Bus "
       "reply. Re-apply the cherry-pick.";

  // And it falls back to SPA_ID_INVALID when the key is missing.
  EXPECT_TRUE(contains(content, "out_pipewire_object_serial = SPA_ID_INVALID"))
    << "src/platform/linux/portalgrab.cpp is missing the "
       "SPA_ID_INVALID fallback when the pipewire-serial key is "
       "not in the portal reply. Re-apply the 2438a9bd cherry-pick.";
}

// =============================================================================
// 3. The configure_stream API carries the object_serial through to
//    the call site. The pre-2438a9bd signature had 3 output params;
//    the post-2438a9bd signature has 4 (added uint64_t& object_serial).
// =============================================================================

TEST(SolarflarePortalPipewireCherryPick, ConfigureStreamCarriesObjectSerial) {
  const auto content = read_file("src/platform/linux/portalgrab.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/portalgrab.cpp.";

  // The new signature has uint64_t &out_pipewire_object_serial as a
  // [[maybe_unused]]-annotated output param.
  EXPECT_TRUE(contains(content, "uint64_t &out_pipewire_object_serial [[maybe_unused]]"))
    << "src/platform/linux/portalgrab.cpp's configure_stream "
       "signature is missing the uint64_t &out_pipewire_object_serial "
       "output parameter. The 2438a9bd cherry-pick added this so "
       "the caller (src/platform/linux/portalgrab.cpp's stream "
       "method) can thread the object_serial through to "
       "pw_stream_connect. Re-apply the cherry-pick.";

  // And the implementation assigns it from stream.pipewire_object_serial.
  EXPECT_TRUE(contains(content, "out_pipewire_object_serial = stream.pipewire_object_serial"))
    << "configure_stream is not assigning the object_serial from "
       "the stream struct. The 2438a9bd cherry-pick added this "
       "assignment. Re-apply the cherry-pick.";
}
