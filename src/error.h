/**
 * @file src/error.h
 * @brief SolarFlare enhanced error handling: tagged error codes, source
 *        location in every error log, and a process-wide counter that the
 *        /api/errors HTTP endpoint reports.
 *
 * ponytail: one header, one .cpp, one macro. The previous fork used
 * bare BOOST_LOG(error) calls with a generic "Could not encode video
 * packet" log at the call site -- the actual root cause was already
 * logged deeper inside encode_nvenc/encode_avcodec, but the call site
 * had no way to thread the encoder type, frame number, or session id
 * into its own message. User pasted one generic line into GitHub;
 * the diagnostic info was two lines above it. SUN_ERR() captures
 * __FILE__:__LINE__:__func__ at the call site, increments a process
 * counter, and accepts a short tag so the Web UI can group errors
 * by category (encoder / capture / network / session).
 */
#pragma once

// standard includes
#include <atomic>
#include <cstdint>
#include <source_location>
#include <string_view>

// lib includes
#include <boost/log/sources/severity_logger.hpp>

#include "logging.h"

namespace sunshine {

  /**
   * @brief Coarse error category, used to group errors in the /api/errors
   *        payload. The numeric values are stable across releases so the
   *        Web UI can match against them; new categories are added at
   *        the end.
   */
  enum class error_category_e : std::uint8_t {
    ENCODER = 0,
    CAPTURE = 1,
    NETWORK = 2,
    SESSION = 3,
    PROCESS = 4,
    CONFIG  = 5,
    CRYPTO  = 6,
    UNKNOWN = 7,
  };

  /**
   * @brief Encoder-specific failure reason, returned by encode() so the
   *        caller can branch on it.
   */
  enum class encode_error_e : std::uint8_t {
    NONE = 0,
    EMPTY_PACKET,
    FRAME_INDEX_MISMATCH,
    UNSUPPORTED_SESSION,
  };

  constexpr std::string_view to_string(error_category_e cat) {
    switch (cat) {
      case error_category_e::ENCODER: return "encoder";
      case error_category_e::CAPTURE: return "capture";
      case error_category_e::NETWORK: return "network";
      case error_category_e::SESSION: return "session";
      case error_category_e::PROCESS: return "process";
      case error_category_e::CONFIG:  return "config";
      case error_category_e::CRYPTO:  return "crypto";
      case error_category_e::UNKNOWN: return "unknown";
    }
    return "unknown";
  }

  constexpr std::string_view to_string(encode_error_e err) {
    switch (err) {
      case encode_error_e::NONE:                 return "none";
      case encode_error_e::EMPTY_PACKET:         return "empty_packet";
      case encode_error_e::FRAME_INDEX_MISMATCH: return "frame_index_mismatch";
      case encode_error_e::UNSUPPORTED_SESSION:  return "unsupported_session";
    }
    return "none";
  }

  struct error_counters_t {
    std::atomic<std::uint64_t> encoder {0};
    std::atomic<std::uint64_t> capture {0};
    std::atomic<std::uint64_t> network {0};
    std::atomic<std::uint64_t> session {0};
    std::atomic<std::uint64_t> process {0};
    std::atomic<std::uint64_t> config  {0};
    std::atomic<std::uint64_t> crypto  {0};
    std::atomic<std::uint64_t> unknown {0};
    std::atomic<std::uint64_t> total   {0};
  };

  error_counters_t &counters();

  template <typename... Args>
  void log_error(
      error_category_e cat,
      std::string_view tag,
      std::string_view fmt_or_msg,
      const std::source_location &loc = std::source_location::current()) {
    auto &c = counters();
    auto bump = [&c](std::atomic<std::uint64_t> &counter) {
      counter.fetch_add(1, std::memory_order_relaxed);
      c.total.fetch_add(1, std::memory_order_relaxed);
    };
    switch (cat) {
      case error_category_e::ENCODER: bump(c.encoder); break;
      case error_category_e::CAPTURE: bump(c.capture); break;
      case error_category_e::NETWORK: bump(c.network); break;
      case error_category_e::SESSION: bump(c.session); break;
      case error_category_e::PROCESS: bump(c.process); break;
      case error_category_e::CONFIG:  bump(c.config);  break;
      case error_category_e::CRYPTO:  bump(c.crypto);  break;
      case error_category_e::UNKNOWN: bump(c.unknown); break;
    }
    // ponytail: build one string_view so the stream is a single
    // operator<< chain. Boost.Log's stream operator still works on
    // std::string_view via the implicit string conversion; only the
    // user-defined literal `sv` suffix needs a using-directive, which
    // we don't want in a header.
    auto loc_str = std::string_view(loc.file_name());
    auto fn_str = std::string_view(loc.function_name());
    auto cat_str = to_string(cat);
    std::string prefix;
    prefix.reserve(64);
    prefix += "["; prefix += cat_str; prefix += ":"; prefix += tag; prefix += "] ";
    prefix += fmt_or_msg;
    prefix += "  ("; prefix += loc_str; prefix += ":";
    prefix += std::to_string(loc.line()); prefix += " "; prefix += fn_str; prefix += ")";
    BOOST_LOG(error) << prefix;
  }

}  // namespace sunshine

#define SUN_ERR(cat, tag, ...) \
  ::sunshine::log_error((cat), (tag), ##__VA_ARGS__)
