// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/latency_stats.cpp
 * @brief Implementation of the process-wide latency statistics singleton.
 */
#include "latency_stats.h"

// standard includes
#include <algorithm>

namespace sunshine {

  void metric_accumulator_t::collect(double value_ms) {
    if (value_ms < 0.0) {
      value_ms = 0.0;
    }

    auto s = samples.fetch_add(1, std::memory_order_relaxed) + 1;
    total.fetch_add(value_ms, std::memory_order_relaxed);

    if (s == 1) {
      min.store(value_ms, std::memory_order_relaxed);
      max.store(value_ms, std::memory_order_relaxed);
      return;
    }

    auto cur_min = min.load(std::memory_order_relaxed);
    while (value_ms < cur_min && !min.compare_exchange_weak(cur_min, value_ms, std::memory_order_relaxed)) {
    }

    auto cur_max = max.load(std::memory_order_relaxed);
    while (value_ms > cur_max && !max.compare_exchange_weak(cur_max, value_ms, std::memory_order_relaxed)) {
    }
  }

  stat_snapshot_t metric_accumulator_t::snapshot() const {
    stat_snapshot_t result;
    auto s = samples.load(std::memory_order_relaxed);
    if (s > 0) {
      result.min = min.load(std::memory_order_relaxed);
      result.max = max.load(std::memory_order_relaxed);
      result.avg = total.load(std::memory_order_relaxed) / static_cast<double>(s);
      result.samples = s;
    }
    return result;
  }

  void metric_accumulator_t::reset() {
    min.store(0.0, std::memory_order_relaxed);
    max.store(0.0, std::memory_order_relaxed);
    total.store(0.0, std::memory_order_relaxed);
    samples.store(0, std::memory_order_relaxed);
  }

  void latency_stats_t::set_effective_settings(effective_settings_t settings) {
    std::lock_guard<std::mutex> lock(settings_mutex);
    effective_settings_ = std::move(settings);
  }

  effective_settings_t latency_stats_t::effective_settings() const {
    std::lock_guard<std::mutex> lock(settings_mutex);
    return effective_settings_;
  }

  void latency_stats_t::reset() {
    capture_ms.reset();
    convert_ms.reset();
    encode_ms.reset();
    network_total_ms.reset();
    network_queue_dwell_ms.reset();
    network_fec_ms.reset();
    network_send_ms.reset();
    rtt_ms.reset();
  }

  latency_stats_t &latency_stats() {
    static latency_stats_t stats;
    return stats;
  }

}  // namespace sunshine
