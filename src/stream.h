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

  namespace detail {
    /**
     * @brief Compare an IPv4 interface address with a local streaming address.
     *
     * @param interface_address The interface address in network byte order.
     * @param local_address The local streaming address.
     * @return `true` when both addresses contain the same bytes.
     */
    bool ipv4_address_matches(const boost::asio::ip::address_v4::bytes_type &interface_address, const boost::asio::ip::address_v4 &local_address);

    /**
     * @brief Calculate the next packet batch without exceeding a pacing interval.
     *
     * @param pending_packets The number of prepared packets waiting to be sent.
     * @param maximum_batch_size The platform batch-size limit.
     * @param interval_packets_sent The packets already sent in the current interval.
     * @param interval_packet_limit The maximum packets allowed in the interval.
     * @return The number of packets to send, or zero when the interval is exhausted.
     */
    std::size_t next_pacing_batch_size(std::size_t pending_packets, std::size_t maximum_batch_size, std::size_t interval_packets_sent, std::size_t interval_packet_limit);

    /**
     * @brief Calculate the maximum encoded-video queue age.
     * @param framerate Client-requested integer frame rate.
     * @param framerate_x100 Exact frame rate multiplied by 100, or zero.
     * @param aggressive Whether the aggressive latency policy is active.
     * @return Queue-age budget derived from 3 safe or 1.5 aggressive frame intervals.
     */
    std::chrono::nanoseconds video_queue_age_budget(int framerate, int framerate_x100, bool aggressive);

    /**
     * @brief Parse a Moonlight per-frame FEC status into adaptive network statistics.
     * @param payload Packed big-endian SS_FRAME_FEC_STATUS payload.
     * @param rtt_ms Current ENet round-trip time in milliseconds.
     * @return Packet-loss percentage and RTT, or no value for malformed input.
     */
    std::optional<std::pair<float, float>> parse_frame_fec_status(std::string_view payload, std::uint32_t rtt_ms);
  }  // namespace detail

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
