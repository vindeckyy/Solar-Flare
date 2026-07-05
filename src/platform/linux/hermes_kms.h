/**
 * @file src/platform/linux/hermes_kms.h
 * @brief Hermes-KMS virtual display consumer for Sunshine.
 *
 * Hermes-KMS (github.com/MrOz59/Hermes-KMS) is a Linux DRM/KMS driver that
 * exposes a render node + a DRM_RENDER_ALLOW ioctl UAPI for zero-copy
 * scanout capture. We use it as another capture backend alongside KMS,
 * Wayland, X11, Portal, and KWin.
 *
 * The UAPI is documented in the upstream repo under include/uapi/drm/.
 * We bundle the header here so we don't depend on the kernel module being
 * installed at build time.
 *
 * The capture loop is intentionally minimal: open the render node,
 * probe with GET_VERSION/GET_CAPS, run WAIT_FRAME -> ACQUIRE_FRAME to pull
 * DMA-BUFs. The DMA-BUF is then handed off to the existing encoder path
 * (VAAPI / NVENC / AMF) — Hermes-KMS is a capture backend only.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "src/video.h"

namespace platf {

  // Hermes-KMS UAPI version we require. Bump when upstream adds fields.
  constexpr std::uint32_t HERMES_KMS_REQUIRED_UAPI = 7;

  struct hermes_kms_caps_t {
    std::uint64_t flags = 0;
    std::uint32_t min_width = 0;
    std::uint32_t min_height = 0;
    std::uint32_t max_width = 0;
    std::uint32_t max_height = 0;
    std::uint32_t preferred_width = 0;
    std::uint32_t preferred_height = 0;
    std::uint32_t max_refresh_hz = 0;
  };

  struct hermes_kms_status_t {
    bool module_loaded = false;       ///< True if /sys/module/hermes_kms exists.
    int card_index = -1;              ///< /dev/dri/cardN, -1 if not found.
    std::uint32_t uapi_version = 0;    ///< 0 if GET_VERSION ioctl failed.
    hermes_kms_caps_t caps;
    std::string driver_version;       ///< "M.m.p" from GET_VERSION.
    std::string last_error;           ///< Human-readable error if probing failed.
  };

  /**
   * @brief Probe the kernel module + device. Safe to call repeatedly.
   *        Does not open the device for capture — only checks presence and
   *        reads capabilities.
   */
  hermes_kms_status_t probe_hermes_kms();

  /**
   * @brief Capture backend entry points. Mirror the kms_display() / wl_display()
   *        signatures so they slot into the existing source selector.
   *
   *  - hermes_kms_display_names(): list outputs (always 1 if the device exists).
   *  - hermes_kms_display(): build a display_t bound to the device.
   *  - verify_hermes_kms(): true if a working device is present.
   */
  std::vector<std::string> hermes_kms_display_names(mem_type_e hwdevice_type);
  std::shared_ptr<display_t> hermes_kms_display(mem_type_e hwdevice_type,
                                                const std::string &display_name,
                                                const video::config_t &config);
  bool verify_hermes_kms();

}  // namespace platf
