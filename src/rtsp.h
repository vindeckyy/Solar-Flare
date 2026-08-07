// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/rtsp.h
 * @brief Declarations for RTSP streaming.
 */
#pragma once

// standard includes
#include <atomic>
#include <optional>
#include <string_view>
#include <unordered_map>

// local includes
#include "crypto.h"
#include "thread_safe.h"

namespace rtsp_stream {
  /// @brief TCP port offset for the RTSP SETUP listener (relative to config::sunshine.port).
  constexpr auto RTSP_SETUP_PORT = 21;

  /**
   * @brief Parsed SDP fields from an RTSP ANNOUNCE payload.
   */
  struct announce_payload_t {
    std::string_view client;  ///< SDP session name from the s= field.
    std::unordered_map<std::string_view, std::string_view> attributes;  ///< SDP a= fields by name.
  };

  /**
   * @brief Parse SDP fields used by an RTSP ANNOUNCE request.
   *
   * @param payload SDP payload from the request.
   * @return Parsed fields, or std::nullopt when an a= field lacks a separator.
   */
  std::optional<announce_payload_t> parse_announce_payload(std::string_view payload);

  struct launch_session_t {
    uint32_t id;

    crypto::aes_t gcm_key;
    crypto::aes_t iv;

    std::string av_ping_payload;
    uint32_t control_connect_data;

    bool host_audio;
    std::string unique_id;
    int width;
    int height;
    int fps;
    int gcmap;
    int appid;
    int surround_info;
    std::string surround_params;
    bool continuous_audio;
    bool enable_hdr;
    bool enable_sops;

    std::optional<crypto::cipher::gcm_t> rtsp_cipher;
    std::string rtsp_url_scheme;
    uint32_t rtsp_iv_counter;
    std::string client_cert;
    std::string client_name;  ///< Client device name from the RTSP announce s= field.
    std::string client_address;  ///< Client IP address (populated at session start).
  };

  void launch_session_raise(std::shared_ptr<launch_session_t> launch_session);

  /**
   * @brief Clear state for the specified launch session.
   * @param launch_session_id The ID of the session to clear.
   */
  void launch_session_clear(uint32_t launch_session_id);

  /**
   * @brief Get the number of active sessions.
   * @return Count of active sessions.
   */
  int session_count();

  /**
   * @brief Terminates all running streaming sessions.
   */
  void terminate_sessions();
  void terminate_sessions_by_cert(std::string_view cert);

  /**
   * @brief Runs the RTSP server loop.
   */
  void start();
}  // namespace rtsp_stream
