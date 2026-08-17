// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/telemetry.cpp
 * @brief Implementation of the time-series telemetry store and the
 *        Linux host resource poll thread.
 */
#include "telemetry.h"

// standard includes
#include <atomic>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string_view>
#include <thread>
#include <unordered_map>

// third-party includes
#include <nlohmann/json.hpp>

#ifdef __linux__
#endif

namespace sunshine::telemetry {

  namespace {

    std::mutex series_mutex;  ///< Guards series_map.
    std::unordered_map<std::string, series_t> series_map;  ///< Metric name -> ring buffer.

    std::atomic<bool> resource_monitor_running {false};
    std::thread resource_monitor_thread;

    /// Most recent raw /proc/stat CPU jiffies, for delta computation.
    std::atomic<unsigned long long> last_cpu_total {0};
    std::atomic<unsigned long long> last_cpu_idle {0};

    /**
     * @brief Add one value to a ring buffer, wrapping at capacity.
     *
     * @param series Ring buffer to update.
     * @param value Sample value, caller must clamp negatives before call.
     */
    void push_ring(series_t &series, double value) {
      if (series.values.size() < kSeriesCapacity) {
        series.values.push_back(value);
        // head points to next write index, which equals size when not yet wrapped
        series.head = series.values.size() % kSeriesCapacity;
      }
      else {
        series.values[series.head] = value;
        series.head = (series.head + 1) % kSeriesCapacity;
      }
      if (series.count < kSeriesCapacity) {
        series.count++;
      }
      series.live = series.count == kSeriesCapacity;
    }

    /// Resolve a series by name, creating it if needed (caller holds the lock).
    series_t &get_or_create(std::string &name) {
      auto it = series_map.find(name);
      if (it == series_map.end()) {
        it = series_map.emplace(std::move(name), series_t {}).first;
      }
      return it->second;
    }

#ifdef __linux__
    /// Read the first integer on a line matching @p prefix from @p path.
    long long read_proc_value(const char *path, std::string_view prefix) {
      std::ifstream f(path);
      std::string line;
      while (std::getline(f, line)) {
        if (line.rfind(prefix, 0) == 0) {
          long long value {0};
          std::sscanf(line.c_str() + prefix.size(), " %lld", &value);
          return value;
        }
      }
      return -1;
    }

    /// Total physical RAM in MiB (from /proc/meminfo MemTotal).
    double read_total_ram_mib() {
      auto kb = read_proc_value("/proc/meminfo", "MemTotal:");
      return kb < 0 ? 0.0 : static_cast<double>(kb) / 1024.0;
    }

    /// Available RAM in MiB (from /proc/meminfo MemAvailable).
    double read_available_ram_mib() {
      auto kb = read_proc_value("/proc/meminfo", "MemAvailable:");
      return kb < 0 ? 0.0 : static_cast<double>(kb) / 1024.0;
    }

    /// Read the aggregate CPU busy/total jiffies from /proc/stat.
    bool read_cpu_jiffies(unsigned long long &total, unsigned long long &idle) {
      std::ifstream f("/proc/stat");
      std::string line;
      if (!std::getline(f, line) || line.rfind("cpu ", 0) != 0) {
        return false;
      }
      unsigned long long user {0}, nice {0}, system {0}, idle_ull {0}, iowait {0}, irq {0}, softirq {0}, steal {0};
      std::sscanf(line.c_str() + 4, "%llu %llu %llu %llu %llu %llu %llu %llu",
        &user, &nice, &system, &idle_ull, &iowait, &irq, &softirq, &steal);
      total = user + nice + system + idle_ull + iowait + irq + softirq + steal;
      idle = idle_ull + iowait;
      return true;
    }

    /// GPU utilisation percent (0-100) from AMD sysfs, or -1 when unavailable.
    double read_amd_gpu_busy_percent() {
      static const std::vector<std::string> cards {"card0", "card1", "card2", "card3"};
      for (const auto &card : cards) {
        std::string path = "/sys/class/drm/" + card + "/device/gpu_busy_percent";
        std::ifstream f(path);
        if (f) {
          double value {0.0};
          f >> value;
          return value;
        }
      }
      return -1.0;
    }
#endif

    /// One pass of the host resource sampler (Linux only).
    void sample_host_resources() {
#ifdef __linux__
      unsigned long long total {0}, idle {0};
      if (read_cpu_jiffies(total, idle)) {
        auto last_total = last_cpu_total.exchange(total);
        auto last_idle = last_cpu_idle.exchange(idle);
        if (last_total > 0 && total > last_total) {
          auto delta_total = total - last_total;
          auto delta_idle = idle - last_idle;
          auto busy_pct = 100.0 * static_cast<double>(delta_total - delta_idle) / static_cast<double>(delta_total);
          record("host_cpu_pct", busy_pct);
        }
      }

      auto total_ram = read_total_ram_mib();
      auto available_ram = read_available_ram_mib();
      if (total_ram > 0.0 && available_ram >= 0.0) {
        record("host_ram_used_mb", total_ram - available_ram);
      }

      auto gpu_busy = read_amd_gpu_busy_percent();
      if (gpu_busy >= 0.0) {
        record("host_gpu_pct", gpu_busy);
      }
#endif
    }

    /// Background loop: sample host resources once per second.
    void resource_monitor_loop() {
      while (resource_monitor_running.load()) {
        sample_host_resources();
        for (int i = 0; i < 10 && resource_monitor_running.load(); i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
    }

  }  // namespace

  void record(std::string name, double value) {
    // Clamp negatives and non-finite values, telemetry is for display and must
    // not propagate NaN/inf into JSON which would break the Web UI charts.
    if (!std::isfinite(value) || value < 0.0) {
      value = 0.0;
    }
    // Cap absurdly large values (e.g. corrupt /proc read) to keep charts usable.
    if (value > 1e9) {
      value = 1e9;
    }
    std::lock_guard<std::mutex> lock(series_mutex);
    auto &series = get_or_create(name);
    push_ring(series, value);
  }

  nlohmann::json snapshot() {
    std::lock_guard<std::mutex> lock(series_mutex);
    nlohmann::json out = nlohmann::json::object();
    for (auto &[name, series] : series_map) {
      if (series.count == 0) {
        continue;
      }
      nlohmann::json arr = nlohmann::json::array();
      for (std::size_t i = 0; i < series.count; i++) {
        auto idx = (series.head + kSeriesCapacity - series.count + i) % kSeriesCapacity;
        arr.push_back(series.values[idx]);
      }
      out[name] = std::move(arr);
    }
    out["window_s"] = static_cast<std::size_t>(kSeriesCapacity);
    return out;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(series_mutex);
    series_map.clear();
  }

  void start_resource_monitor() {
#ifdef __linux__
    if (resource_monitor_running.exchange(true)) {
      return;
    }
    resource_monitor_thread = std::thread {resource_monitor_loop};
#else
    // No-op on non-Linux platforms; the charts show "unavailable".
#endif
  }

  void stop_resource_monitor() {
#ifdef __linux__
    if (!resource_monitor_running.exchange(false)) {
      return;
    }
    if (resource_monitor_thread.joinable()) {
      resource_monitor_thread.join();
    }
#endif
  }

}  // namespace sunshine::telemetry
