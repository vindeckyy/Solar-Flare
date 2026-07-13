// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/stream.h
 * @brief Declarations for the streaming protocols.
 */
#pragma once

// standard includes
#include <utility>

// lib includes
#include <boost/asio.hpp>

// local includes
#include "audio.h"
#include "crypto.h"
#include "rtsp.h"
#include "video.h"

namespace stream {
  /// @brief UDP port offset for the video stream (relative to config::sunshine.port).
  constexpr auto VIDEO_STREAM_PORT = 16;
  /// @brief UDP port offset for the ENet control channel (relative to config::sunshine.port).
  constexpr auto CONTROL_PORT = 26;
  /// @brief UDP port offset for the audio stream (relative to config::sunshine.port).
  constexpr auto AUDIO_STREAM_PORT = 27;

  struct session_t;

  struct config_t {
    audio::config_t audio;
    video::config_t monitor;

    int packetsize;
    int minRequiredFecPackets;
    int mlFeatureFlags;
    int controlProtocolType;
    int audioQosType;
    int videoQosType;

    uint32_t encryptionFlagsEnabled;

    std::optional<int> gcmap;
  };

  namespace session {
    enum class state_e : int {
      STOPPED,  ///< The session is stopped
      STOPPING,  ///< The session is stopping
      STARTING,  ///< The session is starting
      RUNNING,  ///< The session is running
    };

    std::shared_ptr<session_t> alloc(config_t &config, rtsp_stream::launch_session_t &launch_session);
    int start(session_t &session, const std::string &addr_string);
    void stop(session_t &session);
    void join(session_t &session);
    state_e state(session_t &session);
    const std::string &client_cert(session_t &session);
  }  // namespace session
}  // namespace stream
