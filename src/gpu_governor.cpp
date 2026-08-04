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

namespace video {

  std::string gpu_governor_level_path(std::string_view sysfs_root, int card) {
    return std::string {sysfs_root} + "/card" + std::to_string(card) +
           "/device/power_dpm_force_performance_level";
  }

  void gpu_governor_write_levels(std::string_view sysfs_root, std::string_view level) {
#ifdef __linux__
    for (int card = 0; card < gpu_governor_max_cards; ++card) {
      std::ofstream f(gpu_governor_level_path(sysfs_root, card));
      if (f) {
        f << level;
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
