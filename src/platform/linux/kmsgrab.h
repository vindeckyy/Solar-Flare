// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/platform/linux/kmsgrab.h
 * @brief Declarations for the KMS/DRM capture backend helpers.
 */
#pragma once

#include <string>

// local includes
#include "src/platform/common.h"

namespace platf {

  /**
   * @brief Resolve the desktop size from sysfs connector mode files.
   *
   * When KMS plane enumeration yields no viewport data (e.g. the Wayland
   * correlation step was skipped and no CRTC planes were active), the
   * physical connector modes exposed under @p drm_class_path (typically
   * `/sys/class/drm`) are scanned for the single largest mode by area.
   *
   * @param drm_class_path Path to the sysfs drm class directory.
   * @param out_w Output: width of the largest connector mode found.
   * @param out_h Output: height of the largest connector mode found.
   * @return `true` if at least one valid connector mode was parsed.
   */
  bool resolve_sysfs_desktop_size(const std::filesystem::path &drm_class_path, int &out_w, int &out_h);

  /**
   * @brief Merge a Wayland monitor viewport into a KMS-derived viewport.
   *
   * Wayland offsets and logical size always win because KMS cannot report them.
   * Physical width and height are only taken when the compositor reported a mode;
   * otherwise the KMS-derived size is kept so a missing @c wl_output.mode event
   * cannot zero out the capture region.
   *
   * @param dst KMS-derived viewport, updated in place.
   * @param src Viewport as reported by the Wayland compositor.
   * @return @c true if the Wayland mode disagreed with the KMS mode.
   */
  bool merge_wayland_viewport(touch_port_t &dst, const touch_port_t &src);

}  // namespace platf
