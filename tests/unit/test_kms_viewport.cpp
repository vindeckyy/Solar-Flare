// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_kms_viewport.cpp
 * @brief Tests for Wayland/KMS viewport correlation helpers.
 */
#include "../tests_common.h"

#ifdef SUNSHINE_BUILD_DRM
  #include <src/platform/linux/kmsgrab.h>
#endif

#ifdef SUNSHINE_BUILD_DRM
namespace {
  platf::touch_port_t kms_viewport(int width, int height) {
    return {
      .offset_x = 0,
      .offset_y = 0,
      .width = width,
      .height = height,
      .logical_width = 0,
      .logical_height = 0,
    };
  }

  platf::touch_port_t wayland_viewport(
    int width,
    int height,
    int offset_x,
    int offset_y,
    int logical_width,
    int logical_height
  ) {
    return {
      .offset_x = offset_x,
      .offset_y = offset_y,
      .width = width,
      .height = height,
      .logical_width = logical_width,
      .logical_height = logical_height,
    };
  }
}  // namespace

// Issue #19: a missing wl_output.mode must not clobber the KMS-derived size.
TEST(KmsViewportMergeTest, KeepsKmsSizeWhenWaylandModeMissing) {
  auto dst = kms_viewport(2560, 1600);
  const auto src = wayland_viewport(0, 0, 0, 0, 1463, 914);

  EXPECT_FALSE(platf::merge_wayland_viewport(dst, src));
  EXPECT_EQ(dst.width, 2560);
  EXPECT_EQ(dst.height, 1600);
  EXPECT_EQ(dst.logical_width, 1463);
  EXPECT_EQ(dst.logical_height, 914);
  EXPECT_GT(dst.offset_x + dst.width, 0);
}

TEST(KmsViewportMergeTest, AdoptsWaylandModeWhenReported) {
  auto dst = kms_viewport(2560, 1600);
  const auto src = wayland_viewport(2560, 1600, 0, 0, 1463, 914);

  EXPECT_FALSE(platf::merge_wayland_viewport(dst, src));
  EXPECT_EQ(dst.width, 2560);
  EXPECT_EQ(dst.height, 1600);
}

TEST(KmsViewportMergeTest, ReportsMismatchWhenModesDisagree) {
  auto dst = kms_viewport(1920, 1080);
  const auto src = wayland_viewport(2560, 1600, 0, 0, 1463, 914);

  EXPECT_TRUE(platf::merge_wayland_viewport(dst, src));
  EXPECT_EQ(dst.width, 2560);
  EXPECT_EQ(dst.height, 1600);
}

TEST(KmsViewportMergeTest, AlwaysAdoptsOffsets) {
  auto dst = kms_viewport(2560, 1600);
  const auto src = wayland_viewport(0, 0, 1280, 720, 1463, 914);

  EXPECT_FALSE(platf::merge_wayland_viewport(dst, src));
  EXPECT_EQ(dst.offset_x, 1280);
  EXPECT_EQ(dst.offset_y, 720);
  EXPECT_EQ(dst.width, 2560);
  EXPECT_EQ(dst.height, 1600);
}

TEST(KmsViewportMergeTest, CaptureRegionRemainsNonZeroAfterMissingMode) {
  auto dst = kms_viewport(2560, 1600);
  const auto src = wayland_viewport(0, 0, 0, 0, 1463, 914);

  platf::merge_wayland_viewport(dst, src);
  EXPECT_GT(dst.width, 0);
  EXPECT_GT(dst.height, 0);
}
#endif
