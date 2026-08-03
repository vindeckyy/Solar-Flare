// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/latency_stats.h
 * @brief Process-wide host-side latency statistics and effective encoder
 *        settings, exposed through the GET /api/stream/latency endpoint.
 *
 * The streaming pipeline (capture, conversion, encoding, network send)
 * records one sample per frame into per-metric accumulators. Each
 * accumulator keeps a lock-free min/max/avg so the HTTP handler can
 * return a live snapshot at any time without blocking the streaming
 * threads. The effective encoder settings snapshot records what the
 * active session actually negotiated (codec, driver, rate-control mode,
 * slice count, queue depth, QP bounds, buffer size, bitrate, framerate)
 * so a config change can be verified against the running encoder.
 */
#pragma once

// standard includes
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

namespace sunshine {

  /**
   * @brief A min/max/avg snapshot of one latency metric, in milliseconds.
   */
  struct stat_snapshot_t {
    double min {0.0};  ///< Minimum observed value in milliseconds.
    double max {0.0};  ///< Maximum observed value in milliseconds.
    double avg {0.0};  ///< Average observed value in milliseconds.
    std::uint32_t samples {0};  ///< Number of collected samples.
  };

  /**
   * @brief Lock-free accumulator for a single latency metric.
   *
   * Collectors (streaming threads) only call collect() with the new
   * sample; readers (HTTP handler) only call snapshot(). Both are safe
   * to call concurrently from any thread.
   */
  class metric_accumulator_t {
  public:
    /**
     * @brief Record one sample.
     *
     * @param value_ms Sample value in milliseconds. Negative values are
     *        clamped to zero.
     */
    void collect(double value_ms);

    /**
     * @brief Return the current min/max/avg snapshot.
     *
     * @return The snapshot. All fields are zero when no samples have
     *         been collected.
     */
    stat_snapshot_t snapshot() const;

    /**
     * @brief Reset all accumulated state.
     */
    void reset();

  private:
    std::atomic<double> min {0.0};  ///< Minimum observed value.
    std::atomic<double> max {0.0};  ///< Maximum observed value.
    std::atomic<double> total {0.0};  ///< Sum of all observed values.
    std::atomic<std::uint32_t> samples {0};  ///< Number of collected samples.
  };

  /**
   * @brief Effective encoder settings of the active stream.
   */
  struct effective_settings_t {
    std::string codec;  ///< FFmpeg codec name, e.g. "hevc_vaapi".
    std::string hwdevice;  ///< Hardware device type, e.g. "vaapi".
    std::string vendor;  ///< Driver vendor string.
    std::string va_entrypoint;  ///< VA-API entrypoint name.
    std::string rc_mode;  ///< Effective rate-control mode string.
    int quality {0};  ///< Effective quality level (0 = driver default).
    int slices {0};  ///< Effective slice count.
    int async_depth {0};  ///< Effective encoder queue depth.
    int qmin {0};  ///< Effective minimum QP (0 = unset).
    int qmax {0};  ///< Effective maximum QP (0 = unset).
    std::int64_t rc_buffer_size {0};  ///< Rate-control buffer size in bytes.
    std::int64_t bit_rate {0};  ///< Bitrate in bits per second.
    int framerate {0};  ///< Framerate.
  };

  /**
   * @brief Process-wide latency statistics and effective encoder settings.
   */
  class latency_stats_t {
  public:
    metric_accumulator_t capture_ms;  ///< Capture backend duration per frame.
    metric_accumulator_t convert_ms;  ///< Frame conversion duration per frame.
    metric_accumulator_t encode_ms;  ///< Frame encode duration per frame.
    metric_accumulator_t network_total_ms;  ///< Capture-to-send duration per frame.
    metric_accumulator_t network_queue_dwell_ms;  ///< Network queue dwell per frame.
    metric_accumulator_t network_fec_ms;  ///< FEC encode duration per frame.
    metric_accumulator_t network_send_ms;  ///< Packet batch send duration per frame.
    metric_accumulator_t rtt_ms;  ///< Network round-trip time.

    /**
     * @brief Store the effective encoder settings snapshot.
     *
     * @param settings The settings to store. Written at session open.
     */
    void set_effective_settings(effective_settings_t settings);

    /**
     * @brief Return a copy of the stored effective encoder settings.
     *
     * @return The effective settings, or an all-default struct when no
     *         session has stored them yet.
     */
    effective_settings_t effective_settings() const;

    /**
     * @brief Reset all accumulators.
     */
    void reset();

  private:
    mutable std::mutex settings_mutex;  ///< Guards effective_settings_.
    effective_settings_t effective_settings_;  ///< Stored effective settings.
  };

  /**
   * @brief Access the process-wide latency statistics singleton.
   *
   * @return Reference to the singleton.
   */
  latency_stats_t &latency_stats();

}  // namespace sunshine
