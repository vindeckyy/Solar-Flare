/**
 * @file src/audio.h
 * @brief Declarations for audio capture and encoding.
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

  struct config_t {
    enum flags_e : int {
      HIGH_QUALITY,  ///< High quality audio
      HOST_AUDIO,  ///< Host audio
      CUSTOM_SURROUND_PARAMS,  ///< Custom surround parameters
      CONTINUOUS_AUDIO,  ///< Continuous audio
      MAX_FLAGS  ///< Maximum number of flags
    };

    int packetDuration;
    int channels;
    int mask;

    stream_params_t customStreamParams;

    std::bitset<MAX_FLAGS> flags;
  };

  struct audio_ctx_t {
    // We want to change the sink for the first stream only
    std::unique_ptr<std::atomic_bool> sink_flag;

    std::unique_ptr<platf::audio_control_t> control;

    bool restore_sink;
    platf::sink_t sink;
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
  using packet_t = std::pair<void *, buffer_t>;
  using audio_ctx_ref_t = safe::shared_t<audio_ctx_t>::ptr_t;

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
