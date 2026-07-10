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

}  // namespace platf
