/**
 * @file src/adaptive_bitrate.cpp
 * @brief Definitions for EWMA-based adaptive bitrate controller.
 */
// standard includes
#include <algorithm>
#include <chrono>

// local includes
#include "adaptive_bitrate.h"

namespace video {

  AdaptiveBitrate::AdaptiveBitrate(const config_t &cfg):
      _cfg {cfg} {
  }

  void AdaptiveBitrate::update_network_stats(float packet_loss_pct, float rtt_ms) {
    // Prime the EWMA on the first sample. Without this, the rtt_spike check
    // below would compare the first rtt_ms against 2x(0.3 * rtt_ms) = 0.6x the
    // first sample, falsely flagging every fresh session as congested on its
    // very first update.
    if (!_primed) {
      _ewma_loss = packet_loss_pct;
      // Floor the EWMA seed at MIN_EWMA_RTT_MS so a first sample with rtt_ms=0
      // (the client hasn't measured RTT yet, which happens frequently on
      // session start) cannot pin the long-term EWMA near zero and then have
      // every subsequent reading register as an rtt_spike (rtt_ms > 2x ~0).
      _ewma_rtt = std::max(MIN_EWMA_RTT_MS, rtt_ms);
      _primed = true;
      return;
    }
    _ewma_loss = EWMA_ALPHA * packet_loss_pct + (1.0f - EWMA_ALPHA) * _ewma_loss;
    _ewma_rtt = EWMA_ALPHA * rtt_ms + (1.0f - EWMA_ALPHA) * _ewma_rtt;

    bool rtt_spike = rtt_ms > 2.0f * _ewma_rtt;
    bool is_congested = rtt_spike || _ewma_loss > 0.0f;

    if (is_congested) {
      _in_recovery = false;

      float loss_penalty = 1.0f;
      if (_ewma_loss > 5.0f) {
        loss_penalty = 0.50f;
      } else if (_ewma_loss > 1.0f) {
        loss_penalty = 1.0f - (_ewma_loss / 100.0f);
      }

      float rtt_penalty = 1.0f;
      if (rtt_spike) {
        rtt_penalty = 0.70f;
      }

      _current_scale = std::min(_current_scale * loss_penalty * rtt_penalty, 1.0f);
    } else {
      if (!_in_recovery) {
        _in_recovery = true;
        _recovery_start = std::chrono::steady_clock::now();
      }
    }
  }

  void AdaptiveBitrate::update_stream_health(float fps_ratio, float encode_time_ms, float dropped_frame_ratio) {
    bool unhealthy = false;

    if (encode_time_ms > 11.0f) {
      _current_scale *= 0.88f;
      unhealthy = true;
    }

    if (fps_ratio < 0.88f) {
      _current_scale *= 0.88f;
      unhealthy = true;
    }

    if (dropped_frame_ratio > 0.05f) {
      _current_scale *= 0.90f;
      unhealthy = true;
    }

    if (unhealthy) {
      _in_recovery = false;
      return;
    }

    // If the stream is healthy but the network-driven recovery timer hasn't
    // been started yet (e.g. update_network_stats hasn't been called recently
    // or the network just transitioned out of congestion), arm it now so the
    // user can actually see bitrate recover. Without this, bitrate would only
    // ever decrease until the process restarts on a flaky link.
    if (!_in_recovery) {
      _in_recovery = true;
      _recovery_start = std::chrono::steady_clock::now();
      return;
    }

    auto now = std::chrono::steady_clock::now();
    if (now - _recovery_start >= RECOVERY_TIMEOUT) {
      _current_scale = std::min(_current_scale + RECOVERY_RATE * 0.01f, 1.0f);
    }
  }

  int AdaptiveBitrate::get_target_bitrate(int base_bitrate) {
    int result = static_cast<int>(base_bitrate * _current_scale);
    // Clamp order matters: honour the client-requested ceiling and the
    // configured ceiling first, then ensure we never drop below the
    // configured floor. Without this, when base_bitrate < min_bitrate the
    // earlier max() would set result = min_bitrate only to be clobbered by
    // the final min(result, base_bitrate), returning a value below the floor.
    result = std::min(result, base_bitrate);
    result = std::min(result, _cfg.max_bitrate);
    result = std::max(result, _cfg.min_bitrate);
    return result;
  }

  void AdaptiveBitrate::reset() {
    _ewma_loss = 0.0f;
    _ewma_rtt = 0.0f;
    _current_scale = 1.0f;
    _in_recovery = false;
    _primed = false;
  }

}  // namespace video
