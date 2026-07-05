/**
 * @file src/platform/linux/hermes_kms.cpp
 * @brief Hermes-KMS capture backend. Stub — capture loop is TODO.
 *
 * Why a stub: the full capture path needs working hardware + kernel module
 * for end-to-end verification, which doesn't fit in a single commit. This
 * file implements:
 *
 *   1. Module + device detection (probe_hermes_kms)
 *   2. The source-selector entry points (display_names / display / verify)
 *   3. A display_t stub that returns a clear "not implemented" error on init
 *      so the user gets a useful log instead of a confusing segfault.
 *
 * Capture loop (WAIT_FRAME -> ACQUIRE_FRAME -> import DMA-BUF into encoder)
 * is left as a follow-up. The header documents the planned API.
 */
#include "hermes_kms.h"
#include "hermes_kms_drm.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#include "src/logging.h"

namespace platf {

  namespace {

    // Ponytail: read /sys/module/hermes_kms to detect the module is loaded.
    // find_hermes_kms_card() below scans /dev/dri/card* for the card node.
    // The card node is what Hermes-KMS ioctls work on — render nodes
    // (renderD*) pass through to the DRM render path which does not forward
    // Hermes-KMS ioctls.
    bool module_loaded() {
      std::error_code ec;
      return std::filesystem::exists("/sys/module/hermes_kms", ec);
    }

    int find_hermes_kms_card() {
      // ponytail: Hermes-KMS ioctls work on the card node, not the
      // render node. Scan /dev/dri/cardN.
      for (int i = 0; i < 16; ++i) {
        std::string path = "/dev/dri/card" + std::to_string(i);
        int fd = open(path.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) continue;
        struct drm_hermes_kms_version ver = {};
        if (ioctl(fd, DRM_IOCTL_HERMES_KMS_GET_VERSION, &ver) == 0) {
          close(fd);
          return i;
        }
        close(fd);
      }
      return -1;
    }

  }  // namespace

  hermes_kms_status_t probe_hermes_kms() {
    hermes_kms_status_t s;
    s.module_loaded = module_loaded();
    if (!s.module_loaded) {
      s.last_error = "hermes_kms kernel module not loaded";
      return s;
    }
    int rn = find_hermes_kms_card();
    if (rn < 0) {
      s.last_error = "no /dev/dri/card* device found";
      return s;
    }
    s.card_index = rn;

    // Open the card node and probe capabilities via the UAPI.
    std::string node_path = "/dev/dri/card" + std::to_string(rn);
    int fd = open(node_path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
      s.last_error = "cannot open " + node_path + ": " + strerror(errno);
      return s;
    }

    // GET_VERSION
    struct drm_hermes_kms_version ver = {};
    if (ioctl(fd, DRM_IOCTL_HERMES_KMS_GET_VERSION, &ver) < 0) {
      s.last_error = "GET_VERSION ioctl failed: " + std::string(strerror(errno));
      close(fd);
      return s;
    }
    s.uapi_version = ver.uapi_version;
    s.driver_version = std::to_string(ver.driver_major) + "."
                     + std::to_string(ver.driver_minor) + "."
                     + std::to_string(ver.driver_patch);

    // GET_CAPS
    struct drm_hermes_kms_caps caps = {};
    if (ioctl(fd, DRM_IOCTL_HERMES_KMS_GET_CAPS, &caps) < 0) {
      s.last_error = "GET_CAPS ioctl failed: " + std::string(strerror(errno));
      close(fd);
      return s;
    }
    s.caps.flags           = caps.flags;
    s.caps.min_width       = caps.min_width;
    s.caps.min_height      = caps.min_height;
    s.caps.max_width       = caps.max_width;
    s.caps.max_height      = caps.max_height;
    s.caps.preferred_width = caps.preferred_width;
    s.caps.preferred_height= caps.preferred_height;
    s.caps.max_refresh_hz  = caps.max_refresh_hz;

    close(fd);

    // ponytail: check that the device advertises the minimum set of
    // capabilities we need. A real capture implementation needs
    // DMABUF_EXPORT + FRAME_WAIT + FRAME_ACQUIRE.
    constexpr std::uint64_t NEEDED = HERMES_KMS_CAP_DMABUF_EXPORT
                                   | HERMES_KMS_CAP_FRAME_WAIT
                                   | HERMES_KMS_CAP_FRAME_ACQUIRE;
    if ((s.caps.flags & NEEDED) != NEEDED) {
      s.last_error = "device missing required caps (need DMABUF_EXPORT + FRAME_WAIT + FRAME_ACQUIRE)";
    }
    return s;
  }

  std::vector<std::string> hermes_kms_display_names(mem_type_e) {
    auto status = probe_hermes_kms();
    if (!status.module_loaded || status.card_index < 0) return {};
    // Hermes-KMS exports exactly one virtual output named "HERMES-1".
    return {"HERMES-1"};
  }

  /**
   * @brief Build a display_t bound to the Hermes-KMS card node.
   * @details STUB. See the header declaration for the full contract.
   *          The real implementation requires working hardware (a loaded
   *          hermes_kms kernel module + a /dev/dri/card* node).
   */
  std::shared_ptr<display_t> hermes_kms_display(mem_type_e hwdevice_type,
                                                const std::string &display_name,
                                                const video::config_t &config) {
    auto status = probe_hermes_kms();
    if (!status.module_loaded) {
      BOOST_LOG(error) << "hermes_kms: kernel module not loaded. "
                          "Install hermes_kms from github.com/MrOz59/Hermes-KMS "
                          "and load it with 'modprobe hermes_kms'.";
      return nullptr;
    }
    if (status.card_index < 0) {
      BOOST_LOG(error) << "hermes_kms: no card node found. "
                          "Check 'dmesg | grep hermes_kms' for driver errors.";
      return nullptr;
    }
    // ponytail: capture loop not yet wired. The card node is present,
    // but the WAIT_FRAME → ACQUIRE_FRAME → import-DMA-BUF path needs
    // the upstream UAPI structs and a real hermes_kms module to test against.
    // Until then, this is a compile-time-present, runtime-detected option
    // that fails with a clear diagnostic instead of a segfault.
    BOOST_LOG(warning) << "hermes_kms: card node /dev/dri/card"
                          << status.card_index
                          << " found (uapi "
                          << status.uapi_version
                          << "), but the capture loop is not yet wired. "
                             "See src/platform/linux/hermes_kms.h for the planned API.";
    return nullptr;
  }

  bool verify_hermes_kms() {
    auto status = probe_hermes_kms();
    return status.module_loaded && status.card_index >= 0;
  }

}  // namespace platf
