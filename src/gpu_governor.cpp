// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/gpu_governor.cpp
 * @brief Definitions for the AMD GPU performance-mode governor.
 */
// standard includes
#include <fstream>
#include <utility>

// local includes
#include "gpu_governor.h"
#include "logging.h"

using namespace std::literals;

namespace video {

  /**
   * @brief Build the sysfs path for one card's force-performance level node.
   * @param sysfs_root Root that mirrors `/sys/class/drm`.
   * @param card Zero-based DRM card index.
   * @return Absolute path to `power_dpm_force_performance_level` for @p card.
   */
  std::string gpu_governor_level_path(std::string_view sysfs_root, int card) {
    if (card < 0 || card >= gpu_governor_max_cards) {
      return {};
    }
    if (sysfs_root.empty()) {
      return {};
    }
    return std::string {sysfs_root} + "/card" + std::to_string(card) +
           "/device/power_dpm_force_performance_level";
  }

  /**
   * @brief Write a performance-level string to every probed card.
   * @param sysfs_root Root that mirrors `/sys/class/drm`.
   * @param level Value to write (typically `performance` or `auto`).
   */
  void gpu_governor_write_levels(std::string_view sysfs_root, std::string_view level) {
    if (level.empty() || sysfs_root.empty()) {
      return;
    }
#ifdef __linux__
    for (int card = 0; card < gpu_governor_max_cards; ++card) {
      auto path = gpu_governor_level_path(sysfs_root, card);
      if (path.empty()) {
        continue;
      }
      std::ofstream f(path);
      if (f) {
        f << level;
        if (!f) {
          // Write failed (permission, read-only fs); log at debug to avoid
          // spamming warning on systems without AMD GPUs.
          BOOST_LOG(debug) << "gpu_governor: failed to write '"sv << level << "' to "sv << path;
        }
      }
    }
#else
    (void) sysfs_root;
    (void) level;
#endif
  }

  gpu_governor_guard_t::gpu_governor_guard_t(bool enabled, std::string sysfs_root):
      sysfs_root_ {std::move(sysfs_root)} {
#ifdef __linux__
    if (enabled) {
      restore_ = true;
      gpu_governor_write_levels(sysfs_root_, "performance");
    }
#else
    (void) enabled;
#endif
  }

  gpu_governor_guard_t::~gpu_governor_guard_t() noexcept {
#ifdef __linux__
    if (restore_) {
      gpu_governor_write_levels(sysfs_root_, "auto");
    }
#endif
  }

  gpu_governor_guard_t::gpu_governor_guard_t(gpu_governor_guard_t &&other) noexcept:
      restore_ {other.restore_},
      sysfs_root_ {std::move(other.sysfs_root_)} {
    other.release();
  }

  gpu_governor_guard_t &gpu_governor_guard_t::operator=(gpu_governor_guard_t &&other) noexcept {
    if (this == &other) {
      return *this;
    }

#ifdef __linux__
    if (restore_) {
      gpu_governor_write_levels(sysfs_root_, "auto");
    }
#endif

    restore_ = other.restore_;
    sysfs_root_ = std::move(other.sysfs_root_);
    other.release();
    return *this;
  }

  bool gpu_governor_guard_t::active() const noexcept {
    return restore_;
  }

  void gpu_governor_guard_t::release() noexcept {
    restore_ = false;
  }

}  // namespace video
