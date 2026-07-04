/**
 * @file src/adaptive_bitrate.h
 * @brief EWMA-based adaptive bitrate controller.
 */
#pragma once

namespace video {

  /**
   * @brief EWMA-based adaptive bitrate controller.
   *
   * Uses client-reported network stats (packet loss, RTT) and host-side
   * stream health (fps ratio, encode time, dropped frames) to dynamically
   * adjust the streaming bitrate within configured [min, max] bounds.
   */
  class AdaptiveBitrate {
  public:
    struct config_t {
      bool enabled = false;
      int min_bitrate = 2000;   ///< kbps, floor
      int max_bitrate = 100000;  ///< kbps, ceiling
    };

    explicit AdaptiveBitrate(const config_t &cfg);

    /**
     * @brief Feed network stats from client reports.
     * @param packet_loss_pct Packet loss percentage (0-100).
     * @param rtt_ms Round-trip time in milliseconds.
     */
    void update_network_stats(float packet_loss_pct, float rtt_ms);

    /**
     * @brief Feed stream health from host encoder pacing.
     * @param fps_ratio Actual FPS / target FPS.
     * @param encode_time_ms Average encode time per frame.
     * @param dropped_frame_ratio Dropped frames / total frames.
     */
    void update_stream_health(float fps_ratio, float encode_time_ms, float dropped_frame_ratio);

    /**
     * @brief Get the recommended bitrate (kbps).
     * @param base_bitrate The client-requested base bitrate.
     * @return Adjusted bitrate clamped to [min, max].
     */
    int get_target_bitrate(int base_bitrate);

    void reset();

  private:
    static constexpr float EWMA_ALPHA = 0.3f;
    static constexpr float RECOVERY_RATE = 0.4f;
    static constexpr std::chrono::seconds RECOVERY_TIMEOUT {10};

    config_t _cfg;
    float _ewma_loss = 0.0f;
    float _ewma_rtt = 0.0f;
    float _current_scale = 1.0f;

    bool _primed = false;  ///< True once the EWMA has consumed its first sample.
    bool _in_recovery = false;
    std::chrono::steady_clock::time_point _recovery_start;
  };

}  // namespace video
