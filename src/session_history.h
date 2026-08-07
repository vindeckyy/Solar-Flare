// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/session_history.h
 * @brief Persistent JSONL record of past streaming sessions.
 *
 * One line per completed stream is appended to
 * `<appdata>/session_history.jsonl`. The record captures the app, client,
 * resolution, codec and average latency/bitrate so users can review what
 * streamed when, and webhooks can react to session lifecycle events.
 *
 * The file grows unbounded by design (users may prune it); the Web UI reads
 * it through GET /api/sessions. Writes are serialized by an internal mutex.
 */
#pragma once

// standard includes
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace sunshine::session_history {

  /**
   * @brief One completed streaming session.
   */
  struct record_t {
    std::chrono::system_clock::time_point t_start;  ///< Session start time.
    std::chrono::system_clock::time_point t_end;  ///< Session end time.
    std::string app_name;  ///< Launched application name.
    std::string client_name;  ///< Client device name from the RTSP announce.
    std::string client_address;  ///< Client IP address.
    std::string codec;  ///< Codec used (e.g. "hevc_vaapi").
    int width {0};  ///< Stream width.
    int height {0};  ///< Stream height.
    int fps {0};  ///< Stream framerate.
    double avg_bitrate_kbps {0.0};  ///< Average bitrate over the session.
    double avg_rtt_ms {0.0};  ///< Average round-trip time.
    double avg_encode_ms {0.0};  ///< Average encode time.
    std::uint64_t dropped_frames {0};  ///< Number of dropped frames.
    std::string error;  ///< End reason (empty = clean, e.g. "idle_timeout").
  };

  /**
   * @brief Append a completed session record to the history file.
   *
   * @param record The record to append. Serialized to one JSON line.
   */
  void record(const record_t &record);

  /**
   * @brief Return the path of the history file.
   *
   * @return `<appdata>/session_history.jsonl`.
   */
  std::string history_file_path();

  /**
   * @brief Read the most recent @p limit records (oldest first within the
   *        window), optionally filtered by app or client name.
   *
   * @param limit Maximum number of records to return.
   * @param app_filter Optional app-name substring filter.
   * @param client_filter Optional client-name substring filter.
   * @return The matching records in chronological order.
   */
  std::vector<record_t> recent(std::size_t limit, const std::string &app_filter, const std::string &client_filter);

}  // namespace sunshine::session_history
