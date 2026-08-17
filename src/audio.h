// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/audio.h
 * @brief Declarations for audio capture, Opus encoding, and pre-encoder FX chain.
 *
 * The capture pipeline is: platform mic -> FX PreProcessor
 * (AGC -> VAD -> noise gate -> ducker) -> Opus encode -> RTP.
 *
 * FX stages are optional and tuned via config::solarflare.audio_fx. All
 * controls are idempotent: calling reset() or stop multiple times is safe,
 * and an empty device string falls back to the host or virtual sink with a
 * warning instead of failing the stream.
 */
#pragma once

// local includes
#include "platform/common.h"
#include "thread_safe.h"
#include "utility.h"

#include <bitset>

namespace audio {
  enum stream_config_e : int {
    STEREO,  ///< Stereo
    HIGH_STEREO,  ///< High stereo
    SURROUND51,  ///< Surround 5.1
    HIGH_SURROUND51,  ///< High surround 5.1
    SURROUND71,  ///< Surround 7.1
    HIGH_SURROUND71,  ///< High surround 7.1
    MAX_STREAM_CONFIG  ///< Maximum audio stream configuration
  };

  struct opus_stream_config_t {
    std::int32_t sampleRate;
    int channelCount;
    int streams;
    int coupledStreams;
    const std::uint8_t *mapping;
    int bitrate;
  };

  struct stream_params_t {
    int channelCount;
    int streams;
    int coupledStreams;
    std::uint8_t mapping[8];
  };

  extern opus_stream_config_t stream_configs[MAX_STREAM_CONFIG];

  /**
   * @brief Audio capture and encode configuration for a single stream.
   *
   * Channels and packet duration drive the Opus frame size. An empty
   * device sink is treated as "use host/virtual fallback" rather than an error,
   * so streams never fail solely due to a missing config string.
   */
  struct config_t {
    enum flags_e : int {
      HIGH_QUALITY,  ///< High quality audio (use high-bitrate stream variant)
      HOST_AUDIO,  ///< Mix host audio into the captured stream
      CUSTOM_SURROUND_PARAMS,  ///< Use customStreamParams instead of the preset mapping
      CONTINUOUS_AUDIO,  ///< Capture continuously even during silence (avoids PipeWire suspend)
      MAX_FLAGS  ///< Maximum number of flags
    };

    int packetDuration;  ///< Opus packet duration in ms (typically 5 or 10).
    int channels;  ///< Requested channel count (2, 6, or 8).
    int mask;  ///< Channel mask (reserved, currently unused, must be 0).

    stream_params_t customStreamParams;  ///< Custom mapping when CUSTOM_SURROUND_PARAMS is set.

    std::bitset<MAX_FLAGS> flags;  ///< Behavior flags (see flags_e).
  };

  /**
   * @brief Per-process audio context owning the platform control and sink state.
   *
   * Lifetime is tied to the shared audio control singleton. Controls are
   * idempotent: init and shutdown may be called multiple times without leaking
   * sinks or double-restoring defaults, and an empty sink is handled gracefully.
   */
  struct audio_ctx_t {
    std::unique_ptr<std::atomic_bool> sink_flag;  ///< True after the first stream claims sink selection.
    std::unique_ptr<platf::audio_control_t> control;  ///< Platform audio backend (Pulse/PipeWire).

    bool restore_sink = false;  ///< Whether the default sink must be restored on shutdown.
    platf::sink_t sink;  ///< Snapshot of host and virtual sinks at init.
  };

  /**
   * @brief Opus encoder tuning (per-stream).
   *
   * Selected at encode-thread start based on the @c config_t for the stream.
   * Defaults preserve the historical Sunshine behaviour (RESTRICTED_LOWDELAY,
   * CBR).
   */
  struct opus_tuning_t {
    enum class application_e : int {
      LOWDELAY = 0,  ///< @c OPUS_APPLICATION_RESTRICTED_LOWDELAY (default).
      VOIP = 1,  ///< @c OPUS_APPLICATION_VOIP — better for speech.
      AUDIO = 2,  ///< @c OPUS_APPLICATION_AUDIO — best for music/SFX.
    };

    enum class vbr_e : int {
      OFF = 0,  ///< Hard CBR (legacy behaviour).
      CONSTRAINED = 1,  ///< Constrained VBR (recommended).
      FULL = 2,  ///< Full VBR (variable packet size).
    };

    /// Opus application mode (default: LOWDELAY).
    application_e application = application_e::LOWDELAY;
    /// VBR mode (default: OFF = CBR).
    vbr_e vbr = vbr_e::OFF;
    /// Expected packet-loss percentage hint for PLC (0–100). 0 disables.
    int expected_packet_loss_pct = 0;
    /// Enable in-band FEC (forward error correction).
    bool enable_fec = true;
    /// Complexity (0–10; higher = more CPU, better quality). Default: 10.
    int complexity = 10;
    /// Use Opus bandwidth extension for speech. Default: true.
    bool enable_bandwidth_extension = true;
  };

  using buffer_t = util::buffer_t<std::uint8_t>;

  /** @brief Encoded audio packet with capture-order metadata. */
  struct packet_t {
    void *channel_data;  ///< Owning streaming session.
    buffer_t data;  ///< Encoded Opus payload.
    std::uint64_t frame_index;  ///< Monotonic capture index used to preserve RTP gaps after drops.
  };

  using audio_ctx_ref_t = safe::shared_t<audio_ctx_t>::ptr_t;

  /**
   * @brief Capture audio, apply FX chain (AGC/VAD/ducking/gate), and encode Opus.
   *
   * FX chain order: capture -> AGC -> VAD (observes) -> noise gate -> ducker -> Opus.
   * Each stage is independently switchable via config::solarflare.audio_fx; when a stage
   * is disabled its cost is avoided. The function handles empty device strings by
   * falling back to the host sink or virtual null sink, and all sink switches are
   * idempotent so repeated capture starts do not leak or double-restore.
   *
   * @param mail Mailbox for shutdown signaling and packet queuing.
   * @param config Audio stream parameters (channels, duration, flags).
   * @param channel_data Opaque per-stream context forwarded to encoded packets.
   */
  void capture(safe::mail_t mail, config_t config, void *channel_data);

  /**
   * @brief Access the active Opus tuning for a session.
   * @return Reference to the global tuning struct (modifiable by tests and
   *         by the config loader before capture() is invoked).
   */
  opus_tuning_t &opus_tuning() noexcept;

  /**
   * @brief Get the reference to the audio context.
   * @returns A shared pointer reference to audio context.
   * @note Aside from the configuration purposes, it can be used to extend the
   *       audio sink lifetime to capture sink earlier and restore it later.
   *
   * @examples
   * audio_ctx_ref_t audio = get_audio_ctx_ref()
   * @examples_end
   */
  audio_ctx_ref_t get_audio_ctx_ref();

  /**
   * @brief Check if the audio sink held by audio context is available.
   * @returns True if available (and can probably be restored), false otherwise.
   * @note Useful for delaying the release of audio context shared pointer (which
   *       tries to restore original sink).
   *
   * @examples
   * audio_ctx_ref_t audio = get_audio_ctx_ref()
   * if (audio.get()) {
   *     return is_audio_ctx_sink_available(*audio.get());
   * }
   * return false;
   * @examples_end
   */
  bool is_audio_ctx_sink_available(const audio_ctx_t &ctx);
}  // namespace audio
