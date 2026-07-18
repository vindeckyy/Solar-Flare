// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/platform/windows/misc.h
 * @brief Miscellaneous declarations for Windows.
 */
#pragma once

// standard includes
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>

// platform includes
#include <Windows.h>
#include <winnt.h>

namespace platf {
  void print_status(const std::string_view &prefix, HRESULT status);
  HDESK syncThreadDesktop();

  /**
   * @brief Read the Windows performance counter.
   * @return The current raw QueryPerformanceCounter tick value, or zero on failure.
   */
  int64_t qpc_counter();

  /**
   * @brief Convert the difference between two performance counter values to time.
   * @param performance_counter1 The newer raw QueryPerformanceCounter value.
   * @param performance_counter2 The older raw QueryPerformanceCounter value.
   * @return The elapsed time between the counter values.
   */
  std::chrono::nanoseconds qpc_time_difference(int64_t performance_counter1, int64_t performance_counter2);

  /**
   * @brief Measure the age of a Windows Graphics Capture timestamp.
   * @param current_performance_counter The current raw QueryPerformanceCounter value.
   * @param frame_system_relative_time The frame SystemRelativeTime in 100-nanosecond units.
   * @return The elapsed time since the frame timestamp.
   */
  std::chrono::nanoseconds wgc_time_difference(int64_t current_performance_counter, int64_t frame_system_relative_time);

  /**
   * @brief Get file version information from a Windows executable or driver file.
   * @param file_path Path to the file to query.
   * @param version_str Output parameter for version string in format "major.minor.build.revision".
   * @return true if version info was successfully extracted, false otherwise.
   */
  bool getFileVersionInfo(const std::filesystem::path &file_path, std::string &version_str);
}  // namespace platf
