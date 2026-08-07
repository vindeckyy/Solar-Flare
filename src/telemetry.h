// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/telemetry.h
 * @brief Process-wide time-series store for host resource telemetry.
 *
 * A dedicated poll thread samples host CPU / memory / GPU utilisation once
 * per second on Linux and feeds ring-buffer series (10-minute window).
 * The HTTP handler for GET /api/stream/telemetry calls snapshot() to render
 * the charts on the Web UI Dashboard. The store intentionally tracks host
 * resources only -- per-frame latency/bitrate statistics already have their
 * own min/avg/max panel (latency_stats.cpp) and are not duplicated here.
 *
 * All methods are safe to call from any thread; producers never block or
 * allocate in the hot path.
 */
#pragma once

// standard includes
#include <cstddef>
#include <string>
#include <vector>

// third-party includes
#include <nlohmann/json_fwd.hpp>

namespace sunshine::telemetry {

  /// Number of one-second buckets kept per metric (10 minutes at 1 Hz).
  constexpr std::size_t kSeriesCapacity {600};

  /// One time-series ring buffer.
  struct series_t {
    std::vector<double> values;  ///< Ring buffer of samples.
    std::size_t head {0};  ///< Ring index of the most recent sample.
    std::size_t count {0};  ///< Number of live samples.
    bool live {false};  ///< True once the buffer is full and wraps.
  };

  /**
   * @brief Record one sample for @p name.
   *
   * The value is appended to the named ring buffer. A missing buffer is
   * created on demand. Negative values are clamped to 0.
   *
   * @param name Metric name, e.g. "host_cpu_pct".
   * @param value Sample value (any unit).
   */
  void record(std::string name, double value);

  /**
   * @brief Return a JSON-ready snapshot of all series.
   *
   * The object maps metric name -> array of up to kSeriesCapacity samples
   * (oldest first). Missing / empty series are omitted.
   *
   * @return Snapshot as an nlohmann::json object.
   */
  nlohmann::json snapshot();

  /**
   * @brief Clear every series. Used on session teardown so idle polls show
   *        empty charts instead of stale activity.
   */
  void reset();

  /**
   * @brief Start the host resource poll thread (1 Hz on Linux).
   *
   * The thread reads /proc/stat and /proc/meminfo, plus AMD GPU utilisation
   * when available, and feeds the host_* series. Non-Linux platforms are a
   * no-op (no thread is started).
   */
  void start_resource_monitor();

  /**
   * @brief Stop the host resource poll thread. Safe to call when not running.
   */
  void stop_resource_monitor();

}  // namespace sunshine::telemetry
