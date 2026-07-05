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
 * The capture loop is intentionally minimal: open the card node,
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

  /**
   * @brief Hermes-KMS UAPI version we require.
   * @details Bump this constant when upstream adds fields. The consumer
   *          checks GET_VERSION on probe and refuses to load if the
   *          kernel module reports an older version.
   */
  constexpr std::uint32_t HERMES_KMS_REQUIRED_UAPI = 7;

  /**
   * @brief Reported capabilities of a Hermes-KMS device.
   * @details Populated from the GET_CAPS ioctl. flags is a bitmask of
   *          HERMES_KMS_CAP_* values defined in the upstream UAPI header.
   */
  struct hermes_kms_caps_t {
    std::uint64_t flags = 0;            ///< Capability bitmask (HERMES_KMS_CAP_*).
    std::uint32_t min_width = 0;        ///< Smallest scanout width the device accepts.
    std::uint32_t min_height = 0;       ///< Smallest scanout height.
    std::uint32_t max_width = 0;        ///< Largest scanout width.
    std::uint32_t max_height = 0;       ///< Largest scanout height.
    std::uint32_t preferred_width = 0;  ///< Compositor's preferred width.
    std::uint32_t preferred_height = 0; ///< Compositor's preferred height.
    std::uint32_t max_refresh_hz = 0;   ///< Maximum supported refresh rate in Hz.
  };

  /**
   * @brief Result of probe_hermes_kms(). Describes the local environment for
   *        the Hermes-KMS driver and the device (if any).
   */
  struct hermes_kms_status_t {
    bool module_loaded = false;       ///< True if /sys/module/hermes_kms exists.
    int card_index = -1;              ///< /dev/dri/card* index, -1 if not found.
    std::uint32_t uapi_version = 0;    ///< 0 if GET_VERSION ioctl failed or not run yet.
    hermes_kms_caps_t caps;           ///< Capabilities (only valid if card_index >= 0).
    std::string driver_version;       ///< "M.m.p" from GET_VERSION.
    std::string last_error;           ///< Human-readable error if probing failed.
  };

  /**
   * @brief Probe the kernel module + device. Safe to call repeatedly.
   *        Does not open the device for capture — only checks presence and
   *        reads capabilities.
   * @return hermes_kms_status_t describing the local environment. On a
   *         clean system without the kernel module, returns
   *         module_loaded=false and a non-empty last_error string.
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
  /**
   * @brief List Hermes-KMS outputs. Returns {"HERMES-1"} if the device is
   *        present, {} otherwise.
   * @param hwdevice_type Unused — Hermes-KMS always exposes a single virtual
   *        output regardless of the encoder memory type.
   * @return A vector containing "HERMES-1" if the device is present, else empty.
   */
  std::vector<std::string> hermes_kms_display_names(mem_type_e hwdevice_type);
  /**
   * @brief Build a display_t bound to the Hermes-KMS card node.
   * @details Opens the card node, runs GET_VERSION + GET_CAPS, then enters the
   *          WAIT_FRAME -> ACQUIRE_FRAME loop, importing each frame's DMA-BUF
   *          into the encoder consumer (VAAPI / NVENC / AMF).
   * @param hwdevice_type Encoder memory type (DMA / VAAPI / CUDA).
   * @param display_name The output name; expected to be "HERMES-1".
   * @param config The video config for the current stream.
   * @return A display_t on success, nullptr on failure.
   */
  std::shared_ptr<display_t> hermes_kms_display(mem_type_e hwdevice_type,
                                                const std::string &display_name,
                                                const video::config_t &config);
  /**
   * @brief Quick check: is a working Hermes-KMS device present?
   * @return true if module_loaded AND a card node was found.
   */
  bool verify_hermes_kms();

}  // namespace platf
