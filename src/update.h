// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/update.h
 * @brief SolarFlare Linux self-update engine.
 */
#pragma once

// standard includes
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

// lib includes
#include <nlohmann/json.hpp>

namespace update {

  /**
   * @brief High-level update phase shown in the Web UI status bar.
   */
  enum class phase_e {
    idle,  ///< No update in progress.
    checking,  ///< Resolving the latest GitHub release.
    downloading,  ///< Fetching SHA256SUMS and the release tarball.
    verifying,  ///< Checking hashes and extracting the staging tree.
    ready,  ///< Staging is ready to apply.
    waiting_idle,  ///< Apply queued until no streaming sessions remain.
    applying,  ///< Installing binary/assets and reapplying capabilities.
    restarting,  ///< Host restart requested after a successful apply.
    error,  ///< Last operation failed; see @c status_t::message and @c log.
    unsupported,  ///< Built on a platform without the Linux updater.
  };

  /**
   * @brief Snapshot of updater state for `/api/update`.
   */
  struct status_t {
    phase_e phase = phase_e::idle;  ///< Current phase.
    int percent = -1;  ///< 0-100 when known, otherwise -1 (indeterminate).
    std::string message;  ///< Short human-readable status line.
    std::string latest_tag;  ///< Latest release tag, when known.
    std::string html_url;  ///< GitHub release page URL, when known.
    std::vector<std::string> log;  ///< Command / action lines for the banner terminal.
    bool outdated = false;  ///< True when latest_tag is newer than the installed build.
    bool can_apply = false;  ///< True when staging is ready and apply may be requested.
    bool busy = false;  ///< True while a worker thread owns the engine.
  };

  /**
   * @brief Convert @p phase to the JSON/UI string token.
   * @param phase Phase value.
   * @return Stable lowercase token (for example `downloading`).
   */
  std::string to_string(phase_e phase);

  /**
   * @brief Serialize @p status for the Web UI.
   * @param status Status snapshot.
   * @return JSON object with phase, percent, message, log, and flags.
   */
  nlohmann::json to_json(const status_t &status);

  /**
   * @brief Return a copy of the current updater status.
   * @return Status snapshot.
   */
  status_t status();

  /**
   * @brief Start downloading and staging the latest Linux release.
   * @return Empty on accept, otherwise an error string for the HTTP layer.
   */
  std::optional<std::string> start();

  /**
   * @brief Apply a previously staged update.
   * @param when_idle When true, wait until no streaming sessions are active.
   * @return Empty on accept, otherwise an error string for the HTTP layer.
   */
  std::optional<std::string> apply(bool when_idle);

  /**
   * @brief Cancel a pending when-idle apply (or a no-op cancel while staged/ready).
   *
   * Clears @c apply_when_idle so @c wait_idle_then_apply() / @c apply_now() honor
   * cancel. When phase is @c waiting_idle but no wait worker is alive, restores
   * @c ready immediately. Rejected unless the phase is @c ready or @c waiting_idle.
   *
   * @return Empty on accept, otherwise an error string for the HTTP layer.
   */
  std::optional<std::string> cancel();

  /**
   * @brief Build an install-failure status message, optionally with rollback detail.
   * @param what_failed Short description of the primary failure (for example chmod).
   * @param primary Error from the primary operation.
   * @param rollback Optional error from a failed rollback; ignored when null or empty.
   * @return Human-readable message including both failures when rollback is set.
   */
  std::string format_install_error(std::string_view what_failed, const std::error_code &primary, const std::error_code *rollback = nullptr);

  /**
   * @brief Parse a SHA256SUMS body into filename -> lowercase hex digest.
   * @param body File contents.
   * @return Map of basename to digest. Lines that do not match are skipped.
   */
  std::unordered_map<std::string, std::string> parse_sha256sums(std::string_view body);

  /**
   * @brief Compare two SolarFlare build tags / version strings.
   * @param lhs Left version (for example `v2026.728.1-solarflare`).
   * @param rhs Right version.
   * @return Negative if lhs < rhs, zero if equal, positive if lhs > rhs.
   *         Unparseable inputs compare as equal (0).
   */
  int compare_versions(std::string_view lhs, std::string_view rhs);

  /**
   * @brief Resolve the expected privileged apply-helper path.
   * @return Absolute path used by the engine and installer.
   */
  std::string apply_helper_path();

#ifdef SUNSHINE_TESTS
  /**
   * @brief Test-only helpers for updater state transitions.
   *
   * Available only when the test binary is built (`SUNSHINE_TESTS`).
   */
  namespace test_access {
    /**
     * @brief Set the updater phase and message without starting a worker.
     * @param phase Phase to store.
     * @param message Status message.
     */
    void force_phase(phase_e phase, std::string message);

    /**
     * @brief Set the when-idle apply flag observed by @c wait_idle_then_apply().
     * @param value New flag value.
     */
    void force_apply_when_idle(bool value);

    /**
     * @brief Read the when-idle apply flag.
     * @return Current flag value.
     */
    bool apply_when_idle();

    /**
     * @brief Set whether a worker thread is marked as running.
     * @param value New flag value.
     */
    void force_worker_running(bool value);

    /**
     * @brief Run the cancelled wait-idle cleanup (ready phase + clear worker).
     *
     * Mirrors the branch taken when @c wait_idle_then_apply() exits because
     * @c apply_when_idle was cleared.
     */
    void complete_idle_cancel();

    /**
     * @brief Invoke @c apply_now(true) without a live wait-idle thread.
     *
     * Used to cover the cancel re-check at the start of idle apply.
     */
    void apply_now_from_idle();
  }  // namespace test_access
#endif

}  // namespace update
