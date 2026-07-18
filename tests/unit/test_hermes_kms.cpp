// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_hermes_kms.cpp
 * @brief Tests for the Hermes-KMS probe that don't require the kernel module.
 *
 * The full capture loop needs a real /dev/dri/card* + loaded hermes_kms
 * module to exercise. These tests cover the public probe contract so that
 * regressions in the failure-propagation paths (module not loaded,
 * UAPI too old, caps missing) are caught without hardware.
 */
#include "../tests_common.h"

#include <src/platform/linux/hermes_kms.h>

#ifdef SUNSHINE_BUILD_DRM
  #include <src/platform/linux/kmsgrab.h>
#endif

#include <filesystem>
#include <fstream>

// On a CI host without the hermes_kms kernel module loaded, probe_hermes_kms()
// must report module_loaded=false with a non-empty last_error and the device
// must be unavailable.
TEST(HermesKmsTest, ProbeReportsModuleNotLoaded) {
  auto status = platf::probe_hermes_kms();
  // Don't assert == false because a developer's box could have it loaded.
  if (!status.module_loaded) {
    EXPECT_FALSE(status.last_error.empty());
    EXPECT_EQ(status.card_index, -1);
    EXPECT_EQ(status.uapi_version, 0u);
  }
}

// verify_hermes_kms() must agree with probe_hermes_kms()'s view of the world.
TEST(HermesKmsTest, VerifyAgreesWithProbe) {
  auto status = platf::probe_hermes_kms();
  EXPECT_EQ(platf::verify_hermes_kms(), status.module_loaded && status.card_index >= 0);
}

// hermes_kms_display_names() must return an empty list when the module
// isn't usable, and otherwise return exactly one HERMES-1 entry.
TEST(HermesKmsTest, DisplayNamesContract) {
  auto names = platf::hermes_kms_display_names(platf::mem_type_e::system);
  auto status = platf::probe_hermes_kms();
  if (status.module_loaded && status.card_index >= 0) {
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "HERMES-1");
  } else {
    EXPECT_TRUE(names.empty());
  }
}

// ---------------------------------------------------------------------------
// Sysfs connector-mode resolution (used by the KMS fallback when the Wayland
// correlation step is skipped and no CRTC planes are active).
// ---------------------------------------------------------------------------

#ifdef SUNSHINE_BUILD_DRM
namespace {
  // Build a fake /sys/class/drm tree: one cardN directory plus a few
  // cardN-CONNECTOR subdirs each containing a `modes` file with a single
  // "<W>x<H>" first line.
  void write_fake_connector(const std::filesystem::path &class_path, const std::string &name, const std::string &mode) {
    auto conn = class_path / name;
    std::filesystem::create_directories(conn);
    std::ofstream(conn / "modes") << mode << "\n";
  }
}  // namespace

// The largest single connector mode (by area) must be returned, never a
// width/height pair mixed across two different connectors.
TEST(KmsgrabSysfsTest, PicksLargestConnectorMode) {
  auto tmp = std::filesystem::temp_directory_path() / "sf_kms_sysfs_largest";
  std::filesystem::remove_all(tmp);
  std::filesystem::create_directories(tmp / "card0");  // card dir with no connector, must be ignored

  write_fake_connector(tmp, "card0-HDMI-A-1", "1920x1080");
  write_fake_connector(tmp, "card0-DP-1", "1280x1024");
  write_fake_connector(tmp, "card0-eDP-1", "3840x2160");

  int w = 0, h = 0;
  EXPECT_TRUE(platf::resolve_sysfs_desktop_size(tmp, w, h));
  EXPECT_EQ(w, 3840);
  EXPECT_EQ(h, 2160);

  std::filesystem::remove_all(tmp);
}

// Connectors whose mode line cannot be parsed, plus renderD* / cardN dirs
// without a dash, must be ignored and the function must report no mode.
TEST(KmsgrabSysfsTest, IgnoresUnparseableAndNonConnectorEntries) {
  auto tmp = std::filesystem::temp_directory_path() / "sf_kms_sysfs_ignore";
  std::filesystem::remove_all(tmp);
  std::filesystem::create_directories(tmp);

  write_fake_connector(tmp, "card0-HDMI-A-1", "garbage");
  // renderD128 and card0 must not be treated as connectors (no '-').
  std::filesystem::create_directories(tmp / "renderD128");
  std::filesystem::create_directories(tmp / "card0");

  int w = 0, h = 0;
  EXPECT_FALSE(platf::resolve_sysfs_desktop_size(tmp, w, h));
  EXPECT_EQ(w, 0);
  EXPECT_EQ(h, 0);

  std::filesystem::remove_all(tmp);
}

// A missing sysfs class directory must report no mode rather than throw.
TEST(KmsgrabSysfsTest, MissingDirectoryReportsNoMode) {
  int w = 0, h = 0;
  EXPECT_FALSE(platf::resolve_sysfs_desktop_size("/nonexistent/sys/class/drm_xyz", w, h));
  EXPECT_EQ(w, 0);
  EXPECT_EQ(h, 0);
}
#endif
