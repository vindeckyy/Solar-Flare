// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_kms_display_name.cpp
 * @brief Tests for KMS connector display-name formatting and mapping.
 */
#include "../tests_common.h"

#ifdef SUNSHINE_BUILD_DRM
  #include <src/platform/linux/kmsgrab.h>
  #include <xf86drmMode.h>
#endif

#ifdef SUNSHINE_BUILD_DRM
TEST(KmsDisplayNameTest, FormatsConnectorName) {
  EXPECT_EQ(platf::format_kms_connector_name(DRM_MODE_CONNECTOR_DisplayPort, 1), "DP-1");
  EXPECT_EQ(platf::format_kms_connector_name(DRM_MODE_CONNECTOR_HDMIA, 2), "HDMI-A-2");
  EXPECT_EQ(platf::format_kms_connector_name(DRM_MODE_CONNECTOR_eDP, 1), "eDP-1");
}

TEST(KmsDisplayNameTest, MapsLegacyNumericIndex) {
  const std::vector<platf::kms_monitor_name_entry_t> monitors = {
    {DRM_MODE_CONNECTOR_DisplayPort, 1, 0},
    {DRM_MODE_CONNECTOR_HDMIA, 1, 1},
  };

  EXPECT_EQ(platf::map_kms_display_name("0", monitors), 0);
  EXPECT_EQ(platf::map_kms_display_name("1", monitors), 1);
  EXPECT_EQ(platf::map_kms_display_name("", monitors), 0);
}

TEST(KmsDisplayNameTest, MapsConnectorNameToMonitorIndex) {
  const std::vector<platf::kms_monitor_name_entry_t> monitors = {
    {DRM_MODE_CONNECTOR_DisplayPort, 1, 0},
    {DRM_MODE_CONNECTOR_HDMIA, 1, 1},
    {DRM_MODE_CONNECTOR_eDP, 1, 2},
  };

  EXPECT_EQ(platf::map_kms_display_name("DP-1", monitors), 0);
  EXPECT_EQ(platf::map_kms_display_name("HDMI-A-1", monitors), 1);
  EXPECT_EQ(platf::map_kms_display_name("eDP-1", monitors), 2);
}

TEST(KmsDisplayNameTest, UnmatchedConnectorFallsBackToZero) {
  const std::vector<platf::kms_monitor_name_entry_t> monitors = {
    {DRM_MODE_CONNECTOR_DisplayPort, 1, 3},
  };

  EXPECT_EQ(platf::map_kms_display_name("HDMI-A-9", monitors), 0);
  EXPECT_EQ(platf::map_kms_display_name("Virtual-Virtual-1", monitors), 0);
}
#endif
