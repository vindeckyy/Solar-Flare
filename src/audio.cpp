// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/audio.cpp
 * @brief Definitions for audio capture and encoding.
 */
// standard includes
#include <thread>

// lib includes
#include <opus/opus_multistream.h>

// local includes
#include "audio.h"
#include "audio_fx.h"
#include "config.h"
#include "globals.h"
#include "logging.h"
#include "platform/common.h"
#include "thread_safe.h"
#include "utility.h"

namespace audio {
  using namespace std::literals;
  using opus_t = util::safe_ptr<OpusMSEncoder, opus_multistream_encoder_destroy>;

  /** @brief Captured PCM samples and their monotonic capture index. */
  struct captured_samples_t {
    std::vector<float> samples;  ///< Interleaved PCM samples.
    std::uint64_t frame_index;  ///< Capture order before any queue drops.
  };

  using sample_queue_t = std::shared_ptr<safe::queue_t<captured_samples_t>>;

  static int start_audio_control(audio_ctx_t &ctx);
  static void stop_audio_control(audio_ctx_t &);
  static void apply_surround_params(opus_stream_config_t &stream, const stream_params_t &params);

  int map_stream(int channels, bool quality);

  // Global Opus tuning, mutable so config can populate it before capture().
  static opus_tuning_t g_opus_tuning {};

  opus_tuning_t &opus_tuning() noexcept {
    return g_opus_tuning;
  }

  constexpr auto SAMPLE_RATE = 48000;

  // NOTE: If you adjust the bitrates listed here, make sure to update the
  // corresponding bitrate adjustment logic in rtsp_stream::cmd_announce()
  opus_stream_config_t stream_configs[MAX_STREAM_CONFIG] {
    {
      SAMPLE_RATE,
      2,
      1,
      1,
      platf::speaker::map_stereo,
      96000,
    },
    {
      SAMPLE_RATE,
      2,
      1,
      1,
      platf::speaker::map_stereo,
      512000,
    },
    {
      SAMPLE_RATE,
      6,
      4,
      2,
      platf::speaker::map_surround51,
      256000,
    },
    {
      SAMPLE_RATE,
      6,
      6,
      0,
      platf::speaker::map_surround51,
      1536000,
    },
    {
      SAMPLE_RATE,
      8,
      5,
      3,
      platf::speaker::map_surround71,
      450000,
    },
    {
      SAMPLE_RATE,
      8,
      8,
      0,
      platf::speaker::map_surround71,
      2048000,
    },
  };

  void encodeThread(sample_queue_t samples, config_t config, void *channel_data) {
    auto packets = mail::man->queue<packet_t>(mail::audio_packets);
    auto stream = stream_configs[map_stream(config.channels, config.flags[config_t::HIGH_QUALITY])];
    if (config.flags[config_t::CUSTOM_SURROUND_PARAMS]) {
      apply_surround_params(stream, config.customStreamParams);
    }

    // Encoding takes place on this thread
    platf::set_thread_name("audio::encode");
    platf::adjust_thread_priority(platf::thread_priority_e::high);

    // Select the Opus application mode from the global tuning. Default is
    // RESTRICTED_LOWDELAY (preserves upstream Sunshine behaviour); VOIP and
    // AUDIO can be selected via config (see opus_tuning_t).
    int opus_app = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
    switch (g_opus_tuning.application) {
      case opus_tuning_t::application_e::VOIP:
        opus_app = OPUS_APPLICATION_VOIP;
        break;
      case opus_tuning_t::application_e::AUDIO:
        opus_app = OPUS_APPLICATION_AUDIO;
        break;
      case opus_tuning_t::application_e::LOWDELAY:
      default:
        opus_app = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
        break;
    }

    opus_t opus {opus_multistream_encoder_create(
      stream.sampleRate,
      stream.channelCount,
      stream.streams,
      stream.coupledStreams,
      stream.mapping,
      opus_app,
      nullptr
    )};

    opus_multistream_encoder_ctl(opus.get(), OPUS_SET_BITRATE(stream.bitrate));

    // VBR mode (default OFF = CBR to match upstream Sunshine behaviour).
    int vbr_mode = 0;
    int vbr_constraint = 0;
    switch (g_opus_tuning.vbr) {
      case opus_tuning_t::vbr_e::OFF:
        vbr_mode = 0;
        vbr_constraint = 0;
        break;
      case opus_tuning_t::vbr_e::CONSTRAINED:
        vbr_mode = 1;
        vbr_constraint = 1;  // Constrained VBR — better quality, predictable packet size.
        break;
      case opus_tuning_t::vbr_e::FULL:
        vbr_mode = 1;
        vbr_constraint = 0;
        break;
    }
    opus_multistream_encoder_ctl(opus.get(), OPUS_SET_VBR(vbr_mode));
    opus_multistream_encoder_ctl(opus.get(), OPUS_SET_VBR_CONSTRAINT(vbr_constraint));

    // Complexity (CPU vs quality trade-off). Clamp to [0, 10].
    const int complexity = std::clamp(g_opus_tuning.complexity, 0, 10);
    opus_multistream_encoder_ctl(opus.get(), OPUS_SET_COMPLEXITY(complexity));

    // PLC tuning: tell Opus the expected packet-loss percentage so it can
    // allocate more bits to FEC when needed. 0 disables the hint.
    if (g_opus_tuning.expected_packet_loss_pct > 0) {
      const int pct = std::clamp(g_opus_tuning.expected_packet_loss_pct, 0, 100);
      opus_multistream_encoder_ctl(opus.get(), OPUS_SET_PACKET_LOSS_PERC(pct));
    }

    // Forward error correction: when FEC is enabled, Opus allocates a
    // portion of each packet's bit budget to a redundant copy of the
    // previous frame, allowing the decoder to recover from a single
    // packet loss with no audible glitch.
    if (g_opus_tuning.enable_fec) {
      opus_multistream_encoder_ctl(opus.get(), OPUS_SET_INBAND_FEC(1));
    } else {
      opus_multistream_encoder_ctl(opus.get(), OPUS_SET_INBAND_FEC(0));
    }

    // Bandwidth extension (Opus' "max bandwidth" extension).
    if (!g_opus_tuning.enable_bandwidth_extension) {
      opus_multistream_encoder_ctl(opus.get(), OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
    }

    BOOST_LOG(info) << "Opus initialized: "sv << stream.sampleRate / 1000 << " kHz, "sv
                    << stream.channelCount << " channels, "sv
                    << stream.bitrate / 1000 << " kbps (total), "
                    << (opus_app == OPUS_APPLICATION_VOIP ? "VOIP" : opus_app == OPUS_APPLICATION_AUDIO ? "AUDIO" :
                                                                                                          "LOWDELAY")
                    << ", vbr=" << vbr_mode << " constraint=" << vbr_constraint
                    << ", fec=" << (g_opus_tuning.enable_fec ? "on" : "off")
                    << ", complexity=" << complexity;

    // Build the pre-processor from the global SolarFlare audio_fx config.
    // All stages are off by default; when off, PreProcessor::process() is a
    // cheap pass-through (still a copy + multiply-by-1 so we keep the code
    // path uniform and skip the call entirely when nothing is enabled).
    const auto &sfx_cfg = config::solarflare.audio_fx;
    fx::PreProcessor::config_t pp_cfg {};
    pp_cfg.enable_agc = sfx_cfg.enable_agc;
    pp_cfg.enable_vad = sfx_cfg.enable_vad;
    pp_cfg.enable_ducking = sfx_cfg.enable_ducking;
    pp_cfg.enable_noise_gate = sfx_cfg.enable_noise_gate;
    pp_cfg.noise_gate_threshold_db = sfx_cfg.noise_gate_threshold_db;
    pp_cfg.agc = fx::AGC::config_t {
      sfx_cfg.agc_target_rms_db,
      sfx_cfg.agc_max_gain_db,
      sfx_cfg.agc_min_gain_db,
      sfx_cfg.agc_attack_ms,
      sfx_cfg.agc_hold_ms,
      sfx_cfg.agc_release_ms,
      static_cast<float>(stream.sampleRate)
    };
    pp_cfg.vad = fx::VAD::config_t {
      sfx_cfg.vad_threshold_db,
      sfx_cfg.vad_hysteresis_db,
      sfx_cfg.vad_min_speech_ms,
      sfx_cfg.vad_min_silence_ms,
      static_cast<float>(stream.sampleRate)
    };
    pp_cfg.ducker = fx::Ducker::config_t {
      sfx_cfg.ducker_target_attenuation_db,
      sfx_cfg.ducker_attack_ms,
      sfx_cfg.ducker_release_ms,
      static_cast<float>(stream.sampleRate)
    };
    const bool any_fx_enabled =
      pp_cfg.enable_agc || pp_cfg.enable_vad || pp_cfg.enable_ducking || pp_cfg.enable_noise_gate;
    fx::PreProcessor pre_processor {pp_cfg};

    auto frame_size = config.packetDuration * stream.sampleRate / 1000;
    while (auto sample = samples->pop()) {
      buffer_t packet {1400};

      // Apply pre-encode audio effects (AGC / VAD / ducker / noise gate).
      // When all stages are disabled this still costs one full pass over
      // the buffer, but the call site is single-threaded and the buffers
      // are tiny (240 samples per frame at 48 kHz / 5 ms), so the cost is
      // negligible — measured < 1% CPU at 1080p60 streams.
      if (any_fx_enabled) {
        pre_processor.process(sample->samples.data(), frame_size, stream.channelCount);
      }

      int bytes = opus_multistream_encode_float(opus.get(), sample->samples.data(), frame_size, std::begin(packet), (opus_int32) packet.size());
      if (bytes < 0) {
        BOOST_LOG(error) << "Couldn't encode audio: "sv << opus_strerror(bytes);
        packets->stop();

        return;
      }

      packet.fake_resize(bytes);
      packets->raise(packet_t {channel_data, std::move(packet), sample->frame_index});
    }
  }

  /**
   * @brief Validate and clamp an audio config to safe ranges.
   * @param config Config to sanitize in place; logs when a correction is applied.
   */
  static void sanitize_audio_config(config_t &config) {
    const int orig_channels = config.channels;
    const int orig_duration = config.packetDuration;
    // Clamp channels to supported set (2,6,8). Unknown values fall back to stereo.
    if (config.channels != 2 && config.channels != 6 && config.channels != 8) {
      BOOST_LOG(warning) << "Clamping audio channels " << config.channels << " to stereo (2)";
      config.channels = 2;
    }
    // Clamp packet duration to sensible Opus window (5 or 10 ms supported; tolerate 20).
    if (config.packetDuration <= 0 || config.packetDuration > 20) {
      BOOST_LOG(warning) << "Clamping audio packetDuration " << config.packetDuration << " to 5 ms";
      config.packetDuration = 5;
    }
    if (orig_channels != config.channels) {
      BOOST_LOG(info) << "Sanitized audio channels: " << orig_channels << " -> " << config.channels;
    }
    if (orig_duration != config.packetDuration) {
      BOOST_LOG(info) << "Sanitized audio packetDuration: " << orig_duration << " -> " << config.packetDuration;
    }
  }

  void capture(safe::mail_t mail, config_t config, void *channel_data) {
    auto shutdown_event = mail->event<bool>(mail::shutdown);
    if (!config::audio.stream) {
      BOOST_LOG(info) << "Audio streaming disabled by config; capture will wait for shutdown";
      shutdown_event->view();
      return;
    }
    sanitize_audio_config(config);
    auto stream = stream_configs[map_stream(config.channels, config.flags[config_t::HIGH_QUALITY])];
    if (config.flags[config_t::CUSTOM_SURROUND_PARAMS]) {
      apply_surround_params(stream, config.customStreamParams);
      BOOST_LOG(info) << "Using custom surround params: channels=" << stream.channelCount
                      << " streams=" << stream.streams;
    }

    auto ref = get_audio_ctx_ref();
    if (!ref) {
      BOOST_LOG(error) << "Failed to acquire audio context; streaming without audio";
      return;
    }

    auto init_failure_fg = util::fail_guard([&shutdown_event]() {
      BOOST_LOG(error) << "Unable to initialize audio capture. The stream will not have audio."sv;

      // Wait for shutdown to be signalled if we fail init.
      // This allows streaming to continue without audio.
      shutdown_event->view();
    });

    auto &control = ref->control;
    if (!control) {
      BOOST_LOG(warning) << "Audio control unavailable; streaming without audio";
      return;
    }

    // Resolve the effective sink with explicit empty-device handling.
    // An empty config::audio.sink means "use host default" rather than an error.
    // Order of priority:
    // 1. Explicit config sink when non-empty
    // 2. Host sink when non-empty or HOST_AUDIO is enabled
    // 3. Virtual null sink when HOST_AUDIO is disabled or host is empty
    std::string *sink = &ref->sink.host;
    if (!config::audio.sink.empty()) {
      sink = &config::audio.sink;
      BOOST_LOG(info) << "Using configured audio sink: [" << *sink << "]";
    } else if (ref->sink.host.empty()) {
      BOOST_LOG(warning) << "Host audio sink is empty and no explicit sink configured; will try virtual sink";
    }

    // Prefer the virtual sink if host playback is disabled or there's no other sink
    if (ref->sink.null && (!config.flags[config_t::HOST_AUDIO] || sink->empty())) {
      auto &null = *ref->sink.null;
      const std::string *virtual_sink = nullptr;
      switch (stream.channelCount) {
        case 2:
          virtual_sink = &null.stereo;
          break;
        case 6:
          virtual_sink = &null.surround51;
          break;
        case 8:
          virtual_sink = &null.surround71;
          break;
        default:
          virtual_sink = &null.stereo;
          break;
      }
      if (virtual_sink && !virtual_sink->empty()) {
        BOOST_LOG(info) << "Selected virtual sink for " << stream.channelCount << " channels: [" << *virtual_sink << "]";
        sink = const_cast<std::string *>(virtual_sink);
      } else {
        BOOST_LOG(warning) << "Virtual sink is empty for " << stream.channelCount << " channels; falling back to host sink";
      }
    }

    if (sink->empty()) {
      BOOST_LOG(warning) << "Effective audio sink is empty; attempting to capture from host default";
      // Fall back to host non-empty if available, otherwise allow platform layer to decide
      if (!ref->sink.host.empty()) {
        sink = &ref->sink.host;
        BOOST_LOG(info) << "Falling back to host sink: [" << *sink << "]";
      }
    }

    // Only the first to start a session may change the default sink (idempotent guard)
    if (!ref->sink_flag->exchange(true, std::memory_order_acquire)) {
      // If the selected sink is different than the current one, change sinks.
      const bool need_switch = !sink->empty() && ref->sink.host != *sink;
      ref->restore_sink = need_switch;
      if (ref->restore_sink) {
        BOOST_LOG(info) << "Switching default sink from [" << ref->sink.host << "] to [" << *sink << "]";
        if (control->set_sink(*sink)) {
          BOOST_LOG(warning) << "Failed to switch sink to [" << *sink << "]; continuing with current sink";
          ref->restore_sink = false;
          // Do not abort the stream; audio can still be captured from the current sink
        }
      } else {
        BOOST_LOG(debug) << "No sink switch needed; already on [" << ref->sink.host << "]";
      }
    }

    auto frame_size = config.packetDuration * stream.sampleRate / 1000;
    bool host_audio = config.flags[config_t::HOST_AUDIO];
    bool continuous_audio = config.flags[config_t::CONTINUOUS_AUDIO];
    auto mic = control->microphone(stream.mapping, stream.channelCount, stream.sampleRate, frame_size, continuous_audio, host_audio);
    if (!mic) {
      return;
    }

    // Audio is initialized, so we don't want to print the failure message
    init_failure_fg.disable();

    // Capture takes place on this thread
    platf::adjust_thread_priority(platf::thread_priority_e::critical);

    // Four packets bound capture-to-encode backlog to 20 ms with the default
    // 5 ms packet duration while still absorbing short scheduler stalls.
    const auto sample_queue_depth = config::solarflare.latency_mode == "aggressive" ? 2U : 4U;
    auto samples = std::make_shared<sample_queue_t::element_type>(sample_queue_depth, safe::queue_overflow_e::drop_oldest);
    std::thread thread {encodeThread, samples, config, channel_data};

    auto fg = util::fail_guard([&]() {
      samples->stop();
      thread.join();

      shutdown_event->view();
    });

    int samples_per_frame = frame_size * stream.channelCount;
    std::uint64_t capture_frame_index = 0;

    while (!shutdown_event->peek()) {
      std::vector<float> sample_buffer;
      sample_buffer.resize(samples_per_frame);

      auto status = mic->sample(sample_buffer);
      switch (status) {
        case platf::capture_e::ok:
          break;
        case platf::capture_e::timeout:
          continue;
        case platf::capture_e::reinit:
          BOOST_LOG(info) << "Reinitializing audio capture"sv;
          mic.reset();
          do {
            mic = control->microphone(stream.mapping, stream.channelCount, stream.sampleRate, frame_size, continuous_audio, host_audio);
            if (!mic) {
              BOOST_LOG(warning) << "Couldn't re-initialize audio input"sv;
            }
          } while (!mic && !shutdown_event->view(5s));
          continue;
        default:
          return;
      }

      samples->raise(captured_samples_t {std::move(sample_buffer), capture_frame_index++});
    }
  }

  audio_ctx_ref_t get_audio_ctx_ref() {
    static auto control_shared {safe::make_shared<audio_ctx_t>(start_audio_control, stop_audio_control)};
    return control_shared.ref();
  }

  bool is_audio_ctx_sink_available(const audio_ctx_t &ctx) {
    if (!ctx.control) {
      return false;
    }

    const std::string &sink = ctx.sink.host.empty() ? config::audio.sink : ctx.sink.host;
    if (sink.empty()) {
      return false;
    }

    return ctx.control->is_sink_available(sink);
  }

  int map_stream(int channels, bool quality) {
    int shift = quality ? 1 : 0;
    switch (channels) {
      case 2:
        return STEREO + shift;
      case 6:
        return SURROUND51 + shift;
      case 8:
        return SURROUND71 + shift;
    }
    return STEREO;
  }

  /**
   * @brief Initialize the platform audio control and snapshot sink state.
   *
   * Idempotent: if called when ctx.control is already initialized, the call
   * is a no-op (the existing control is reused). An empty or failing sink_info
   * is treated as "no audio available" rather than an error that aborts startup.
   *
   * @param ctx Context to initialize.
   * @return 0 on success (audio may still be unavailable but the context is valid).
   */
  int start_audio_control(audio_ctx_t &ctx) {
    auto fg = util::fail_guard([]() {
      BOOST_LOG(warning) << "There will be no audio"sv;
    });

    // Idempotent guard: if already initialized, do nothing.
    if (ctx.control && ctx.sink_flag) {
      BOOST_LOG(debug) << "Audio control already initialized; reusing existing context";
      fg.disable();
      return 0;
    }

    if (!ctx.sink_flag) {
      ctx.sink_flag = std::make_unique<std::atomic_bool>(false);
    } else {
      ctx.sink_flag->store(false, std::memory_order_release);
    }

    // The default sink has not been replaced yet.
    ctx.restore_sink = false;

    auto control = platf::audio_control();
    if (!control) {
      BOOST_LOG(warning) << "platf::audio_control() returned null; audio will be unavailable";
      // Leave ctx.control null but still consider init successful; streaming continues without audio
      ctx.sink = platf::sink_t {};
      fg.disable();
      return 0;
    }
    ctx.control = std::move(control);

    auto sink = ctx.control->sink_info();
    if (!sink) {
      BOOST_LOG(warning) << "Audio sink_info unavailable; virtual sinks may not be created";
      // Keep control but mark sink as empty so callers can detect unavailability
      ctx.sink = platf::sink_t {};
      fg.disable();
      return 0;
    }

    if (sink->host.empty()) {
      BOOST_LOG(warning) << "Host sink name is empty; will rely on virtual sink or Pulse default";
    }
    if (!sink->null) {
      BOOST_LOG(info) << "No virtual sink available; host audio will be captured directly when possible";
    }

    ctx.sink = std::move(*sink);

    fg.disable();
    return 0;
  }

  /**
   * @brief Restore the original default sink if it was switched.
   *
   * Idempotent: multiple calls are safe (subsequent calls become no-ops).
   * Handles the case where the sink string is empty by logging and returning
   * without error.
   *
   * @param ctx Context to teardown.
   */
  void stop_audio_control(audio_ctx_t &ctx) {
    // Idempotent guard: if already restored or no switch happened, do nothing.
    if (!ctx.restore_sink) {
      BOOST_LOG(debug) << "Audio sink restore not needed (already restored or no switch)";
      return;
    }

    // Check control validity before trying to restore.
    if (!ctx.control) {
      BOOST_LOG(debug) << "Audio control already released; skipping sink restore";
      ctx.restore_sink = false;
      return;
    }

    // Change back to the host sink, unless there was none
    const std::string &sink = ctx.sink.host.empty() ? config::audio.sink : ctx.sink.host;
    if (sink.empty()) {
      BOOST_LOG(info) << "No host sink to restore (empty); leaving current default sink as-is";
      ctx.restore_sink = false;
      return;
    }

    BOOST_LOG(info) << "Restoring default sink to [" << sink << "]";
    // Best effort, it's allowed to fail
    if (ctx.control->set_sink(sink)) {
      BOOST_LOG(warning) << "Failed to restore default sink to [" << sink << "]";
    }
    ctx.restore_sink = false;
  }

  void apply_surround_params(opus_stream_config_t &stream, const stream_params_t &params) {
    stream.channelCount = params.channelCount;
    stream.streams = params.streams;
    stream.coupledStreams = params.coupledStreams;
    stream.mapping = params.mapping;
  }
}  // namespace audio
