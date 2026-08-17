// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/session_history.cpp
 * @brief Implementation of the persistent JSONL session history store.
 */
#include "session_history.h"

// standard includes
#include <algorithm>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

// third-party includes
#include <nlohmann/json.hpp>

// local includes
#include "config.h"
#include "logging.h"
#include "platform/common.h"

using namespace std::literals;

namespace sunshine::session_history {

  namespace {

    std::mutex history_mutex;  ///< Serializes append + read.

  }  // namespace

  std::string history_file_path() {
    return (platf::appdata() / "session_history.jsonl").string();
  }

  /**
   * @brief Append a completed session record to the history file.
   * @param record The record to append. Invalid timestamps (t_end < t_start)
   *        are clamped so duration never goes negative.
   */
  void record(const record_t &record) {
    // Clamp invalid duration: some early-exit paths set t_end == epoch.
    auto safe_record = record;
    if (safe_record.t_end < safe_record.t_start) {
      safe_record.t_end = safe_record.t_start;
    }
    if (safe_record.app_name.empty() && safe_record.client_name.empty()) {
      BOOST_LOG(debug) << "session_history: skipping empty record"sv;
      return;
    }

    std::lock_guard<std::mutex> lock(history_mutex);

    nlohmann::json line;
    line["t_start"] = std::chrono::duration_cast<std::chrono::seconds>(safe_record.t_start.time_since_epoch()).count();
    line["t_end"] = std::chrono::duration_cast<std::chrono::seconds>(safe_record.t_end.time_since_epoch()).count();
    line["app_name"] = safe_record.app_name;
    line["client_name"] = safe_record.client_name;
    line["client_address"] = safe_record.client_address;
    line["codec"] = safe_record.codec;
    line["width"] = safe_record.width;
    line["height"] = safe_record.height;
    line["fps"] = safe_record.fps;
    line["avg_bitrate_kbps"] = safe_record.avg_bitrate_kbps;
    line["avg_rtt_ms"] = safe_record.avg_rtt_ms;
    line["avg_encode_ms"] = safe_record.avg_encode_ms;
    line["dropped_frames"] = safe_record.dropped_frames;
    line["error"] = safe_record.error;

    std::ofstream out(history_file_path(), std::ios::app);
    if (!out) {
      BOOST_LOG(warning) << "session_history: could not open "sv << history_file_path() << " for append: "sv << std::strerror(errno);
      return;
    }
    out << line.dump() << '\n';
    if (!out) {
      BOOST_LOG(warning) << "session_history: failed to write to "sv << history_file_path();
    }
  }

  /**
   * @brief Read recent session records with optional filters.
   * @param limit Maximum number of records to return (0 = none).
   * @param app_filter Substring filter for app_name (empty = no filter).
   * @param client_filter Substring filter for client_name (empty = no filter).
   * @return Matching records in chronological order (oldest first within window).
   */
  std::vector<record_t> recent(std::size_t limit, const std::string &app_filter, const std::string &client_filter) {
    if (limit == 0) {
      return {};
    }
    std::lock_guard<std::mutex> lock(history_mutex);

    std::vector<record_t> result;
    result.reserve(std::min<std::size_t>(limit, 128));
    std::ifstream in(history_file_path());
    if (!in) {
      BOOST_LOG(debug) << "session_history: no history file at "sv << history_file_path();
      return result;
    }

    std::string line;
    while (std::getline(in, line)) {
      if (line.empty()) {
        continue;
      }
      try {
        auto json = nlohmann::json::parse(line);
        record_t rec;
        rec.t_start = std::chrono::system_clock::time_point {
          std::chrono::seconds {json.value("t_start", std::int64_t {0})}
        };
        rec.t_end = std::chrono::system_clock::time_point {
          std::chrono::seconds {json.value("t_end", std::int64_t {0})}
        };
        rec.app_name = json.value("app_name", std::string {});
        rec.client_name = json.value("client_name", std::string {});
        rec.client_address = json.value("client_address", std::string {});
        rec.codec = json.value("codec", std::string {});
        rec.width = json.value("width", 0);
        rec.height = json.value("height", 0);
        rec.fps = json.value("fps", 0);
        rec.avg_bitrate_kbps = json.value("avg_bitrate_kbps", 0.0);
        rec.avg_rtt_ms = json.value("avg_rtt_ms", 0.0);
        rec.avg_encode_ms = json.value("avg_encode_ms", 0.0);
        rec.dropped_frames = json.value("dropped_frames", std::uint64_t {0});
        rec.error = json.value("error", std::string {});

        if (!app_filter.empty() && rec.app_name.find(app_filter) == std::string::npos) {
          continue;
        }
        if (!client_filter.empty() && rec.client_name.find(client_filter) == std::string::npos) {
          continue;
        }
        result.push_back(std::move(rec));
      } catch (const std::exception &e) {
        BOOST_LOG(debug) << "session_history: skipping malformed line: "sv << e.what();
      }
    }

    if (result.size() > limit) {
      result.erase(std::begin(result), std::end(result) - static_cast<std::ptrdiff_t>(limit));
    }
    return result;
  }

}  // namespace sunshine::session_history
