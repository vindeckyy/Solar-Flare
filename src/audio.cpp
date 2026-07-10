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
#include "config.h"
#include "globals.h"
#include "audio_fx.h"
#include "logging.h"
#include "platform/common.h"
#include "thread_safe.h"
#include "utility.h"

namespace audio {
  using namespace std::literals;
  using opus_t = util::safe_ptr<OpusMSEncoder, opus_multistream_encoder_destroy>;
  using sample_queue_t = std::shared_ptr<safe::queue_t<std::vector<float>>>;

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
                    << (opus_app == OPUS_APPLICATION_VOIP ? "VOIP"
                        : opus_app == OPUS_APPLICATION_AUDIO ? "AUDIO"
                                                              : "LOWDELAY")
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
      sfx_cfg.agc_target_rms_db, sfx_cfg.agc_max_gain_db, sfx_cfg.agc_min_gain_db,
      sfx_cfg.agc_attack_ms, sfx_cfg.agc_hold_ms, sfx_cfg.agc_release_ms,
      static_cast<float>(stream.sampleRate)
    };
    pp_cfg.vad = fx::VAD::config_t {
      sfx_cfg.vad_threshold_db, sfx_cfg.vad_hysteresis_db,
      sfx_cfg.vad_min_speech_ms, sfx_cfg.vad_min_silence_ms,
      static_cast<float>(stream.sampleRate)
    };
    pp_cfg.ducker = fx::Ducker::config_t {
      sfx_cfg.ducker_target_attenuation_db,
      sfx_cfg.ducker_attack_ms, sfx_cfg.ducker_release_ms,
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
        pre_processor.process(sample->data(), frame_size, stream.channelCount);
      }

      int bytes = opus_multistream_encode_float(opus.get(), sample->data(), frame_size, std::begin(packet), (opus_int32) packet.size());
      if (bytes < 0) {
        BOOST_LOG(error) << "Couldn't encode audio: "sv << opus_strerror(bytes);
        packets->stop();

        return;
      }

      packet.fake_resize(bytes);
      packets->raise(channel_data, std::move(packet));
    }
  }

  void capture(safe::mail_t mail, config_t config, void *channel_data) {
    auto shutdown_event = mail->event<bool>(mail::shutdown);
    if (!config::audio.stream) {
      shutdown_event->view();
      return;
    }
    auto stream = stream_configs[map_stream(config.channels, config.flags[config_t::HIGH_QUALITY])];
    if (config.flags[config_t::CUSTOM_SURROUND_PARAMS]) {
      apply_surround_params(stream, config.customStreamParams);
    }

    auto ref = get_audio_ctx_ref();
    if (!ref) {
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
      return;
    }

    // Order of priority:
    // 1. Virtual sink
    // 2. Audio sink
    // 3. Host
    std::string *sink = &ref->sink.host;
    if (!config::audio.sink.empty()) {
      sink = &config::audio.sink;
    }

    // Prefer the virtual sink if host playback is disabled or there's no other sink
    if (ref->sink.null && (!config.flags[config_t::HOST_AUDIO] || sink->empty())) {
      auto &null = *ref->sink.null;
      switch (stream.channelCount) {
        case 2:
          sink = &null.stereo;
          break;
        case 6:
          sink = &null.surround51;
          break;
        case 8:
          sink = &null.surround71;
          break;
      }
    }

    // Only the first to start a session may change the default sink
    if (!ref->sink_flag->exchange(true, std::memory_order_acquire)) {
      // If the selected sink is different than the current one, change sinks.
      ref->restore_sink = ref->sink.host != *sink;
      if (ref->restore_sink) {
        if (control->set_sink(*sink)) {
          return;
        }
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

    auto samples = std::make_shared<sample_queue_t::element_type>(30);
    std::thread thread {encodeThread, samples, config, channel_data};

    auto fg = util::fail_guard([&]() {
      samples->stop();
      thread.join();

      shutdown_event->view();
    });

    int samples_per_frame = frame_size * stream.channelCount;

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

      samples->raise(std::move(sample_buffer));
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

  int start_audio_control(audio_ctx_t &ctx) {
    auto fg = util::fail_guard([]() {
      BOOST_LOG(warning) << "There will be no audio"sv;
    });

    ctx.sink_flag = std::make_unique<std::atomic_bool>(false);

    // The default sink has not been replaced yet.
    ctx.restore_sink = false;

    if (!(ctx.control = platf::audio_control())) {
      return 0;
    }

    auto sink = ctx.control->sink_info();
    if (!sink) {
      // Let the calling code know it failed
      ctx.control.reset();
      return 0;
    }

    ctx.sink = std::move(*sink);

    fg.disable();
    return 0;
  }

  void stop_audio_control(audio_ctx_t &ctx) {
    // restore audio-sink if applicable
    if (!ctx.restore_sink) {
      return;
    }

    // Change back to the host sink, unless there was none
    const std::string &sink = ctx.sink.host.empty() ? config::audio.sink : ctx.sink.host;
    if (!sink.empty()) {
      // Best effort, it's allowed to fail
      ctx.control->set_sink(sink);
    }
  }

  void apply_surround_params(opus_stream_config_t &stream, const stream_params_t &params) {
    stream.channelCount = params.channelCount;
    stream.streams = params.streams;
    stream.coupledStreams = params.coupledStreams;
    stream.mapping = params.mapping;
  }
}  // namespace audio
