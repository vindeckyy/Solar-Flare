// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/platform/linux/kmsgrab.h
 * @brief Declarations for the KMS/DRM capture backend helpers.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// local includes
#include "src/platform/common.h"

namespace platf {

  /**
   * @brief One KMS monitor entry used when resolving connector display names.
   */
  struct kms_monitor_name_entry_t {
    std::uint32_t type;  ///< DRM connector type (for example DRM_MODE_CONNECTOR_DisplayPort).
    std::uint32_t index;  ///< Connector index within that type (1-based DRM naming).
    std::uint32_t monitor_index;  ///< Position in the SolarFlare/KMS monitor list.
  };

  /**
   * @brief Format a DRM connector type and index as a display name.
   *
   * Matches the string produced by @c drmModeGetConnectorTypeName plus a
   * hyphen and the connector index (for example @c DP-1 or @c HDMI-A-1).
   *
   * @param connector_type DRM connector type constant.
   * @param connector_index Connector index within that type.
   * @return Connector display name used in the KMS output list.
   */
  std::string format_kms_connector_name(std::uint32_t connector_type, std::uint32_t connector_index);

  /**
   * @brief Map a display name to a KMS monitor index.
   *
   * Accepts a legacy numeric monitor index string, or a connector name that
   * matches @ref format_kms_connector_name for an entry in @p monitors.
   *
   * @param display_name Numeric index or connector name (for example @c DP-1).
   * @param monitors Monitor descriptors to search.
   * @return Matched monitor index, the parsed numeric index, or @c 0 when an
   *         unmatched connector name is supplied.
   */
  std::int64_t map_kms_display_name(std::string_view display_name, const std::vector<kms_monitor_name_entry_t> &monitors);

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
