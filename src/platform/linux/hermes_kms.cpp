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

#include <filesystem>
#include <fstream>
#include <string>

#include "src/logging.h"

namespace platf {

  namespace {

    // Ponytail: read /sys/module/hermes_kms to detect the module is loaded.
    // Read /dev/dri/renderD* to find the render node. The render node is
    // the only thing Hermes-KMS exposes — the actual DRM_KMS device lives
    // on the compositor side.
    bool module_loaded() {
      std::error_code ec;
      return std::filesystem::exists("/sys/module/hermes_kms", ec);
    }

    int find_render_node() {
      for (int i = 128; i < 128 + 16; ++i) {
        std::string path = "/dev/dri/renderD" + std::to_string(i - 128);
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) return i;
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
    int rn = find_render_node();
    if (rn < 0) {
      s.last_error = "no /dev/dri/renderD* device found";
      return s;
    }
    s.card_index = rn;
    // Full GET_VERSION / GET_CAPS ioctls go here once the consumer is
    // implemented. The header documents the planned shape.
    s.last_error = "probe OK (capture loop not yet implemented)";
    return s;
  }

  std::vector<std::string> hermes_kms_display_names(mem_type_e) {
    auto status = probe_hermes_kms();
    if (!status.module_loaded || status.card_index < 0) return {};
    // Hermes-KMS exports exactly one virtual output named "HERMES-1".
    return {"HERMES-1"};
  }

  /**
   * @copydoc platf::hermes_kms_display
   * @details STUB. See header for the planned behavior. The real
   *          implementation requires working hardware (a loaded
   *          hermes_kms kernel module + a /dev/dri/renderD* node).
   */
  std::shared_ptr<display_t> hermes_kms_display(mem_type_e, const std::string &, const video::config_t &) {
    BOOST_LOG(warning) << "hermes_kms: capture loop not yet implemented. "
                          "Compile-time stub returns no display. See hermes_kms.h "
                          "for the planned API.";
    return nullptr;
  }

  bool verify_hermes_kms() {
    auto status = probe_hermes_kms();
    return status.module_loaded && status.card_index >= 0;
  }

}  // namespace platf
