// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/gpu_governor.h
 * @brief RAII AMD GPU performance-mode governor for Linux DRM sysfs.
 *
 * When enabled on Linux, construction writes `performance` to
 * `power_dpm_force_performance_level` under each DRM card, and destruction
 * restores `auto`. Missing or unwritable paths are skipped silently. The
 * sysfs root is injectable so unit tests can point at a fake tree.
 */
#pragma once

// standard includes
#include <string>
#include <string_view>

namespace video {

  /// Maximum DRM card index probed (card0 .. cardN-1).
  constexpr int gpu_governor_max_cards = 4;

  /// Default Linux DRM sysfs root used when no test root is injected.
  constexpr std::string_view gpu_governor_default_sysfs_root = "/sys/class/drm";

  /**
   * @brief Build the sysfs path for one card's force-performance level node.
   *
   * @param sysfs_root Root that mirrors `/sys/class/drm`.
   * @param card Zero-based DRM card index.
   * @return Absolute path to `power_dpm_force_performance_level` for @p card.
   */
  std::string gpu_governor_level_path(std::string_view sysfs_root, int card);

  /**
   * @brief Write a performance-level string to every probed card under a root.
   *
   * On non-Linux platforms this is a no-op. On Linux, each card path that
   * cannot be opened for writing is skipped without error.
   *
   * @param sysfs_root Root that mirrors `/sys/class/drm`.
   * @param level Value to write (typically `performance` or `auto`).
   */
  void gpu_governor_write_levels(std::string_view sysfs_root, std::string_view level);

  /**
   * @brief RAII guard that raises AMD GPU power profile for a stream session.
   *
   * Construction applies `performance` when @p enabled is true on Linux.
   * Destruction restores `auto` for the same condition. Disabled, non-Linux,
   * and moved-from instances do nothing.
   */
  class gpu_governor_guard_t {
  public:
    /**
     * @brief Construct a governor guard and optionally apply performance mode.
     *
     * @param enabled When true on Linux, write `performance` immediately and
     *        schedule an `auto` restore on destruction.
     * @param sysfs_root DRM sysfs root; defaults to `/sys/class/drm`. Tests
     *        may inject a temporary directory that mirrors the card layout.
     */
    explicit gpu_governor_guard_t(
      bool enabled,
      std::string sysfs_root = std::string {gpu_governor_default_sysfs_root}
    );

    /**
     * @brief Restore `auto` when this guard still owns an active apply.
     */
    ~gpu_governor_guard_t() noexcept;

    gpu_governor_guard_t(const gpu_governor_guard_t &) = delete;
    gpu_governor_guard_t &operator=(const gpu_governor_guard_t &) = delete;

    /**
     * @brief Move-construct, transferring restore ownership from @p other.
     *
     * @param other Source guard; left inactive after the move.
     */
    gpu_governor_guard_t(gpu_governor_guard_t &&other) noexcept;

    /**
     * @brief Move-assign, restoring any prior ownership then taking @p other.
     *
     * @param other Source guard; left inactive after the move.
     * @return Reference to this guard.
     */
    gpu_governor_guard_t &operator=(gpu_governor_guard_t &&other) noexcept;

    /**
     * @brief Whether destruction will attempt to restore `auto`.
     *
     * @return True when an enabled Linux apply is still owned by this instance.
     */
    [[nodiscard]] bool active() const noexcept;

  private:
    /**
     * @brief Clear restore ownership without writing `auto`.
     *
     * Used by move operations so the source does not restore twice.
     */
    void release() noexcept;

    bool restore_ {false};  ///< When true, destructor writes `auto`.
    std::string sysfs_root_;  ///< DRM sysfs root used for apply and restore.
  };

}  // namespace video
