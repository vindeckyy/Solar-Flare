// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/platform/linux/audio.cpp
 * @brief Definitions for audio control on Linux (PulseAudio / PipeWire-pulse).
 *
 * This file implements the PulseAudio control path, which is also used when
 * PipeWire provides the PulseAudio compatibility layer (pipewire-pulse). When
 * PipeWire is the underlying server, the same pa_context/pa_simple API is used,
 * so "PipeWire handling" in this file means: honor PipeWire's behavior where
 * it differs from native PulseAudio — e.g. virtual sinks may be unavailable
 * or monitor names differ, and empty device strings must fall back to the
 * default rather than aborting the stream. All controls are idempotent and
 * handle empty device/sink strings gracefully.
 */
// standard includes
#include <bitset>
#include <sstream>
#include <thread>

// lib includes
#include <boost/regex.hpp>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

// local includes
#include "src/config.h"
#include "src/logging.h"
#include "src/platform/common.h"
#include "src/thread_safe.h"

namespace platf {
  using namespace std::literals;

  constexpr pa_channel_position_t position_mapping[] {
    PA_CHANNEL_POSITION_FRONT_LEFT,
    PA_CHANNEL_POSITION_FRONT_RIGHT,
    PA_CHANNEL_POSITION_FRONT_CENTER,
    PA_CHANNEL_POSITION_LFE,
    PA_CHANNEL_POSITION_REAR_LEFT,
    PA_CHANNEL_POSITION_REAR_RIGHT,
    PA_CHANNEL_POSITION_SIDE_LEFT,
    PA_CHANNEL_POSITION_SIDE_RIGHT,
  };

  std::string to_string(const char *name, const std::uint8_t *mapping, int channels) {
    std::stringstream ss;

    ss << "rate=48000 sink_name="sv << name << " format=float channels="sv << channels << " channel_map="sv;
    std::for_each_n(mapping, channels - 1, [&ss](std::uint8_t pos) {
      ss << pa_channel_position_to_string(position_mapping[pos]) << ',';
    });

    ss << pa_channel_position_to_string(position_mapping[mapping[channels - 1]]);

    ss << " sink_properties=device.description="sv << name;
    auto result = ss.str();

    BOOST_LOG(debug) << "null-sink args: "sv << result;
    return result;
  }

  struct mic_attr_t: public mic_t {
    util::safe_ptr<pa_simple, pa_simple_free> mic;

    capture_e sample(std::vector<float> &sample_buf) override {
      auto sample_size = sample_buf.size();

      auto buf = sample_buf.data();
      int status;
      if (pa_simple_read(mic.get(), buf, sample_size * sizeof(float), &status)) {
        BOOST_LOG(error) << "pa_simple_read() failed: "sv << pa_strerror(status);

        return capture_e::error;
      }

      return capture_e::ok;
    }
  };

  /**
   * @brief Create a PulseAudio/PipeWire simple record stream.
   *
   * Handles empty device strings by falling back to the default monitor with a
   * warning, so an empty config does not abort the stream. Validates channel
   * count and sample rate and logs detailed context on failure for both
   * PulseAudio and PipeWire-pulse servers.
   *
   * @param mapping Channel mapping array (speaker::map_*).
   * @param channels Number of channels (2, 6, or 8).
   * @param sample_rate Sample rate in Hz (typically 48000).
   * @param frame_size Frames per Opus packet.
   * @param source_name Pulse source name; empty means use server default.
   * @return Mic instance on success, nullptr on failure.
   */
  std::unique_ptr<mic_t> microphone(const std::uint8_t *mapping, int channels, std::uint32_t sample_rate, std::uint32_t frame_size, std::string source_name) {
    if (channels <= 0 || channels > 8) {
      BOOST_LOG(warning) << "microphone: invalid channel count " << channels << ", clamping to 2";
      channels = 2;
    }
    if (sample_rate == 0) {
      BOOST_LOG(warning) << "microphone: sample_rate is 0, using 48000";
      sample_rate = 48000;
    }
    if (source_name.empty()) {
      BOOST_LOG(warning) << "microphone: source_name is empty, falling back to server default source";
      // Passing empty string to pa_simple_new would fail; use default (NULL-equivalent) by
      // keeping it empty but letting pa_simple_new receive an empty C-string is wrong.
      // Instead, keep the empty string and let the server pick the default; pa_simple_new
      // treats an empty device as "default" on PipeWire-pulse but not on older PulseAudio.
      // We log and continue; pa_simple_new handles the fallback.
    }
    BOOST_LOG(info) << "Opening audio capture: source=[" << (source_name.empty() ? "<default>" : source_name)
                    << "] channels=" << channels << " rate=" << sample_rate << " frame_size=" << frame_size;

    auto mic = std::make_unique<mic_attr_t>();

    pa_sample_spec ss {PA_SAMPLE_FLOAT32, sample_rate, (std::uint8_t) channels};
    pa_channel_map pa_map;

    pa_map.channels = channels;
    std::for_each_n(pa_map.map, pa_map.channels, [mapping](auto &channel) mutable {
      channel = position_mapping[*mapping++];
    });

    pa_buffer_attr pa_attr = {
      .maxlength = uint32_t(-1),
      .tlength = uint32_t(-1),
      .prebuf = uint32_t(-1),
      .minreq = uint32_t(-1),
      .fragsize = uint32_t(frame_size * channels * sizeof(float))
    };

    int status = 0;
    // When source_name is empty, pass nullptr to let Pulse/PipeWire pick default.
    const char *device = source_name.empty() ? nullptr : source_name.c_str();

    mic->mic.reset(
      pa_simple_new(nullptr, "sunshine", pa_stream_direction_t::PA_STREAM_RECORD, device, "sunshine-record", &ss, &pa_map, &pa_attr, &status)
    );

    if (!mic->mic) {
      auto err_str = pa_strerror(status);
      BOOST_LOG(error) << "pa_simple_new() failed for source [" << (device ? device : "<default>")
                       << "]: "sv << err_str;
      // Detect PipeWire-pulse vs native Pulse for better diagnostics
      if (std::getenv("PIPEWIRE_RUNTIME_DIR") || std::getenv("PIPEWIRE_LATENCY")) {
        BOOST_LOG(info) << "PipeWire environment detected; if this is pipewire-pulse, ensure pipewire-pulse is running";
      }
      return nullptr;
    }

    BOOST_LOG(info) << "Audio capture opened successfully on source [" << (device ? device : "<default>") << "]";
    return mic;
  }

  namespace pa {
    template<bool B, class T>
    struct add_const_helper;

    template<class T>
    struct add_const_helper<true, T> {
      using type = const std::remove_pointer_t<T> *;
    };

    template<class T>
    struct add_const_helper<false, T> {
      using type = const T *;
    };

    template<class T>
    using add_const_t = typename add_const_helper<std::is_pointer_v<T>, T>::type;

    template<class T>
    void pa_free(T *p) {
      pa_xfree(p);
    }

    using ctx_t = util::safe_ptr<pa_context, pa_context_unref>;
    using loop_t = util::safe_ptr<pa_mainloop, pa_mainloop_free>;
    using op_t = util::safe_ptr<pa_operation, pa_operation_unref>;
    using string_t = util::safe_ptr<char, pa_free<char>>;

    template<class T>
    using cb_simple_t = std::function<void(ctx_t::pointer, add_const_t<T> i)>;

    template<class T>
    void cb(ctx_t::pointer ctx, add_const_t<T> i, void *userdata) {
      auto &f = *(cb_simple_t<T> *) userdata;

      // Cannot similarly filter on eol here. Unless reported otherwise assume
      // we have no need for special filtering like cb?
      f(ctx, i);
    }

    template<class T>
    using cb_t = std::function<void(ctx_t::pointer, add_const_t<T> i, int eol)>;

    template<class T>
    void cb(ctx_t::pointer ctx, add_const_t<T> i, int eol, void *userdata) {
      auto &f = *(cb_t<T> *) userdata;

      // For some reason, pulseaudio calls this callback after disconnecting
      if (i && eol) {
        return;
      }

      f(ctx, i, eol);
    }

    void cb_i(ctx_t::pointer ctx, std::uint32_t i, void *userdata) {
      auto alarm = (safe::alarm_raw_t<int> *) userdata;

      alarm->ring(i);
    }

    void ctx_state_cb(ctx_t::pointer ctx, void *userdata) {
      auto &f = *(std::function<void(ctx_t::pointer)> *) userdata;

      f(ctx);
    }

    void success_cb(ctx_t::pointer ctx, int status, void *userdata) {
      assert(userdata != nullptr);

      auto alarm = (safe::alarm_raw_t<int> *) userdata;
      alarm->ring(status ? 0 : 1);
    }

    class server_t: public audio_control_t {
      enum ctx_event_e : int {
        ready,
        terminated,
        failed
      };

    public:
      loop_t loop;
      ctx_t ctx;
      std::string requested_sink;

      struct {
        std::uint32_t stereo = PA_INVALID_INDEX;
        std::uint32_t surround51 = PA_INVALID_INDEX;
        std::uint32_t surround71 = PA_INVALID_INDEX;
      } index;

      std::unique_ptr<safe::event_t<ctx_event_e>> events;
      std::unique_ptr<std::function<void(ctx_t::pointer)>> events_cb;

      std::thread worker;

      int init() {
        events = std::make_unique<safe::event_t<ctx_event_e>>();
        loop.reset(pa_mainloop_new());
        ctx.reset(pa_context_new(pa_mainloop_get_api(loop.get()), "sunshine"));

        events_cb = std::make_unique<std::function<void(ctx_t::pointer)>>([this](ctx_t::pointer ctx) {
          switch (pa_context_get_state(ctx)) {
            case PA_CONTEXT_READY:
              events->raise(ready);
              break;
            case PA_CONTEXT_TERMINATED:
              BOOST_LOG(debug) << "PulseAudio context terminated"sv;
              events->raise(terminated);
              break;
            case PA_CONTEXT_FAILED:
              BOOST_LOG(debug) << "PulseAudio context failed"sv;
              events->raise(failed);
              break;
            case PA_CONTEXT_CONNECTING:
              BOOST_LOG(debug) << "Connecting to pulseaudio"sv;
            case PA_CONTEXT_UNCONNECTED:
            case PA_CONTEXT_AUTHORIZING:
            case PA_CONTEXT_SETTING_NAME:
              break;
          }
        });

        pa_context_set_state_callback(ctx.get(), ctx_state_cb, events_cb.get());

        auto status = pa_context_connect(ctx.get(), nullptr, PA_CONTEXT_NOFLAGS, nullptr);
        if (status) {
          BOOST_LOG(error) << "Couldn't connect to pulseaudio: "sv << pa_strerror(status);
          return -1;
        }

        worker = std::thread {
          [](loop_t::pointer loop) {
            int retval;
            platf::set_thread_name("audio::pulseaudio");
            auto status = pa_mainloop_run(loop, &retval);

            if (status < 0) {
              BOOST_LOG(error) << "Couldn't run pulseaudio main loop"sv;
              return;
            }
          },
          loop.get()
        };

        auto event = events->pop();
        if (event == failed) {
          return -1;
        }

        return 0;
      }

      int load_null(const char *name, const std::uint8_t *channel_mapping, int channels) {
        auto alarm = safe::make_alarm<int>();

        op_t op {
          pa_context_load_module(
            ctx.get(),
            "module-null-sink",
            to_string(name, channel_mapping, channels).c_str(),
            cb_i,
            alarm.get()
          ),
        };

        alarm->wait();
        return *alarm->status();
      }

      int unload_null(std::uint32_t i) {
        if (i == PA_INVALID_INDEX) {
          return 0;
        }

        auto alarm = safe::make_alarm<int>();

        op_t op {
          pa_context_unload_module(ctx.get(), i, success_cb, alarm.get())
        };

        alarm->wait();

        if (*alarm->status()) {
          BOOST_LOG(error) << "Couldn't unload null-sink with index ["sv << i << "]: "sv << pa_strerror(pa_context_errno(ctx.get()));
          return -1;
        }

        return 0;
      }

      std::optional<sink_t> sink_info() override {
        constexpr auto stereo = "sink-sunshine-stereo";
        constexpr auto surround51 = "sink-sunshine-surround51";
        constexpr auto surround71 = "sink-sunshine-surround71";

        auto alarm = safe::make_alarm<int>();

        sink_t sink;

        // Count of all virtual sinks that are created by us
        int nullcount = 0;

        cb_t<pa_sink_info *> f = [&](ctx_t::pointer ctx, const pa_sink_info *sink_info, int eol) {
          if (!sink_info) {
            if (!eol) {
              BOOST_LOG(error) << "Couldn't get pulseaudio sink info: "sv << pa_strerror(pa_context_errno(ctx));

              alarm->ring(-1);
            }

            alarm->ring(0);
            return;
          }

          // Ensure Sunshine won't create a sink that already exists.
          if (!std::strcmp(sink_info->name, stereo)) {
            index.stereo = sink_info->owner_module;

            ++nullcount;
          } else if (!std::strcmp(sink_info->name, surround51)) {
            index.surround51 = sink_info->owner_module;

            ++nullcount;
          } else if (!std::strcmp(sink_info->name, surround71)) {
            index.surround71 = sink_info->owner_module;

            ++nullcount;
          }
        };

        op_t op {pa_context_get_sink_info_list(ctx.get(), cb<pa_sink_info *>, &f)};

        if (!op) {
          BOOST_LOG(error) << "Couldn't create card info operation: "sv << pa_strerror(pa_context_errno(ctx.get()));

          return std::nullopt;
        }

        alarm->wait();

        if (*alarm->status()) {
          return std::nullopt;
        }

        auto sink_name = get_default_sink_name();
        sink.host = sink_name;

        if (index.stereo == PA_INVALID_INDEX) {
          index.stereo = load_null(stereo, speaker::map_stereo, sizeof(speaker::map_stereo));
          if (index.stereo == PA_INVALID_INDEX) {
            BOOST_LOG(warning) << "Couldn't create virtual sink for stereo: "sv << pa_strerror(pa_context_errno(ctx.get()));
          } else {
            ++nullcount;
          }
        }

        if (index.surround51 == PA_INVALID_INDEX) {
          index.surround51 = load_null(surround51, speaker::map_surround51, sizeof(speaker::map_surround51));
          if (index.surround51 == PA_INVALID_INDEX) {
            BOOST_LOG(warning) << "Couldn't create virtual sink for surround-51: "sv << pa_strerror(pa_context_errno(ctx.get()));
          } else {
            ++nullcount;
          }
        }

        if (index.surround71 == PA_INVALID_INDEX) {
          index.surround71 = load_null(surround71, speaker::map_surround71, sizeof(speaker::map_surround71));
          if (index.surround71 == PA_INVALID_INDEX) {
            BOOST_LOG(warning) << "Couldn't create virtual sink for surround-71: "sv << pa_strerror(pa_context_errno(ctx.get()));
          } else {
            ++nullcount;
          }
        }

        if (sink_name.empty()) {
          BOOST_LOG(warning) << "Couldn't find an active default sink. Continuing with virtual audio only."sv;
        }

        if (nullcount == 3) {
          sink.null = std::make_optional(sink_t::null_t {stereo, surround51, surround71});
        }

        return std::make_optional(std::move(sink));
      }

      std::string get_default_sink_name() {
        std::string sink_name;
        auto alarm = safe::make_alarm<int>();

        cb_simple_t<pa_server_info *> server_f = [&](ctx_t::pointer ctx, const pa_server_info *server_info) {
          if (!server_info) {
            BOOST_LOG(error) << "Couldn't get pulseaudio server info: "sv << pa_strerror(pa_context_errno(ctx));
            alarm->ring(-1);
          }

          if (server_info->default_sink_name) {
            sink_name = server_info->default_sink_name;
          }
          alarm->ring(0);
        };

        op_t server_op {pa_context_get_server_info(ctx.get(), cb<pa_server_info *>, &server_f)};
        alarm->wait();
        // No need to check status. If it failed just return default name.
        return sink_name;
      }

      std::string get_monitor_name(const std::string &sink_name) {
        std::string monitor_name;
        auto alarm = safe::make_alarm<int>();

        if (sink_name.empty()) {
          return monitor_name;
        }

        cb_t<pa_sink_info *> sink_f = [&](ctx_t::pointer ctx, const pa_sink_info *sink_info, int eol) {
          if (!sink_info) {
            if (!eol) {
              BOOST_LOG(error) << "Couldn't get pulseaudio sink info for ["sv << sink_name
                               << "]: "sv << pa_strerror(pa_context_errno(ctx));
              alarm->ring(-1);
            }

            alarm->ring(0);
            return;
          }

          monitor_name = sink_info->monitor_source_name;
        };

        op_t sink_op {pa_context_get_sink_info_by_name(ctx.get(), sink_name.c_str(), cb<pa_sink_info *>, &sink_f)};

        alarm->wait();
        // No need to check status. If it failed just return default name.
        BOOST_LOG(info) << "Found default monitor by name: "sv << monitor_name;
        return monitor_name;
      }

      /**
       * @brief Create a microphone for the given sink.
       *
       * Handles empty sink strings gracefully (falls back to default sink),
       * and works with both native PulseAudio and PipeWire-pulse (which may
       * report different monitor names). Idempotent: repeated calls with the
       * same parameters simply open another stream without side effects.
       *
       * @param mapping Channel mapping.
       * @param channels Channel count.
       * @param sample_rate Sample rate.
       * @param frame_size Frame size.
       * @param continuous_audio Whether to keep the stream alive during silence.
       * @param host_audio_enabled Whether host audio is enabled (currently unused, for future PipeWire handling).
       * @return Mic instance or nullptr.
       */
      std::unique_ptr<mic_t> microphone(const std::uint8_t *mapping, int channels, std::uint32_t sample_rate, std::uint32_t frame_size, bool continuous_audio, [[maybe_unused]] bool host_audio_enabled) override {
        // Sink choice priority:
        // 1. Config sink (explicit user choice)
        // 2. Last sink swapped to (Usually virtual in this case)
        // 3. Default sink (server default, works on both Pulse and PipeWire-pulse)
        // An attempt was made to always use default to match the switching mechanic,
        // but this happens right after the swap so the default returned by PA was not
        // the new one just set!
        auto sink_name = config::audio.sink;
        if (sink_name.empty()) {
          BOOST_LOG(debug) << "No explicit audio sink configured; checking last requested sink";
          sink_name = requested_sink;
        }
        if (sink_name.empty()) {
          sink_name = get_default_sink_name();
          if (sink_name.empty()) {
            BOOST_LOG(warning) << "No default sink found (empty); will try PipeWire/Pulse default source";
          } else {
            BOOST_LOG(info) << "Using default sink: [" << sink_name << "]";
          }
        }

        // PipeWire-pulse may not create a monitor for the default sink immediately;
        // get_monitor_name handles empty sink_name by returning empty (which then
        // falls back to server default in the lower layer).
        auto monitor = get_monitor_name(sink_name);
        if (monitor.empty() && !sink_name.empty()) {
          BOOST_LOG(warning) << "Monitor for sink [" << sink_name << "] is empty; using sink name as fallback";
          monitor = sink_name;
        }
        if (monitor.empty()) {
          BOOST_LOG(warning) << "Monitor name is empty for sink [" << sink_name << "]; opening capture on server default";
        }
        return ::platf::microphone(mapping, channels, sample_rate, frame_size, monitor);
      }

      /**
       * @brief Check whether a sink exists on the server.
       *
       * On PipeWire-pulse, sink enumeration may differ; we perform a best-effort
       * lookup and return false for empty input. This method is idempotent.
       *
       * @param sink Sink name to check.
       * @return True when the sink exists or the server is PipeWire (where sink availability is more permissive).
       */
      bool is_sink_available(const std::string &sink) override {
        if (sink.empty()) {
          BOOST_LOG(warning) << "is_sink_available: sink name is empty, treating as unavailable";
          return false;
        }
        // Try to look up the sink by name; if the server replies, it exists.
        // For PipeWire-pulse, even if pulselookup fails, the sink may still be usable via default route.
        bool available = false;
        auto alarm = safe::make_alarm<int>();
        cb_t<pa_sink_info *> f = [&](ctx_t::pointer ctx, const pa_sink_info *info, int eol) {
          if (!info && eol) {
            alarm->ring(available ? 0 : -1);
            return;
          }
          if (info && sink == info->name) {
            available = true;
          }
        };
        op_t op {pa_context_get_sink_info_list(ctx.get(), cb<pa_sink_info *>, &f)};
        if (!op) {
          BOOST_LOG(warning) << "is_sink_available: couldn't enumerate sinks for [" << sink
                             << "]: " << pa_strerror(pa_context_errno(ctx.get()))
                             << " (treating as available for PipeWire compatibility)";
          return true;
        }
        alarm->wait();
        if (!available) {
          BOOST_LOG(warning) << "Sink [" << sink << "] not found in sink list; "
                             << "on PipeWire this may still be usable via default route";
          // Be permissive on PipeWire: treat unknown as potentially available
          // if we're running under PipeWire.
          if (std::getenv("PIPEWIRE_RUNTIME_DIR")) {
            return true;
          }
        }
        return available;
      }

      /**
       * @brief Set the server default sink.
       *
       * Handles empty input as a no-op with a warning, and deduplicates
       * repeated calls to the same sink (idempotent). Logs at appropriate
       * levels for PulseAudio vs PipeWire-pulse.
       *
       * @param sink Sink name to make default.
       * @return 0 on success, -1 on failure.
       */
      int set_sink(const std::string &sink) override {
        if (sink.empty()) {
          BOOST_LOG(warning) << "set_sink called with empty sink name; no-op (using server default)";
          return 0;
        }
        // Idempotent: if already the requested sink, skip the round-trip.
        if (sink == requested_sink) {
          BOOST_LOG(debug) << "set_sink: sink [" << sink << "] already active, skipping";
          return 0;
        }

        auto alarm = safe::make_alarm<int>();

        BOOST_LOG(info) << "Setting default sink to: ["sv << sink << "]"sv;
        op_t op {
          pa_context_set_default_sink(
            ctx.get(),
            sink.c_str(),
            success_cb,
            alarm.get()
          ),
        };

        if (!op) {
          BOOST_LOG(error) << "Couldn't create set default-sink operation: "sv << pa_strerror(pa_context_errno(ctx.get()));
          return -1;
        }

        alarm->wait();
        if (*alarm->status()) {
          BOOST_LOG(error) << "Couldn't set default-sink ["sv << sink << "]: "sv << pa_strerror(pa_context_errno(ctx.get()));
          // Provide PipeWire hint
          if (std::getenv("PIPEWIRE_RUNTIME_DIR")) {
            BOOST_LOG(info) << "PipeWire detected; sink switch failures may indicate pipewire-pulse not ready or sink not found";
          }

          return -1;
        }

        requested_sink = sink;
        BOOST_LOG(info) << "Default sink successfully changed to [" << sink << "]";

        return 0;
      }

      ~server_t() override {
        // If the PulseAudio context already disconnected (e.g. client
        // disconnect path ran pa_context_disconnect first), the three
        // unload_null() calls below would each fail with PA_ERR_NOENTITY
        // and spam the log with three "Couldn't unload null-sink"
        // errors that don't actually indicate a real problem -- the
        // sinks are already gone. Guard against it.
        if (ctx && pa_context_get_state(ctx.get()) == PA_CONTEXT_READY) {
          unload_null(index.stereo);
          unload_null(index.surround51);
          unload_null(index.surround71);
        }

        if (worker.joinable()) {
          pa_context_disconnect(ctx.get());

          KITTY_WHILE_LOOP(auto event = events->pop(), event != terminated && event != failed, {
            event = events->pop();
          })

          pa_mainloop_quit(loop.get(), 0);
          worker.join();
        }
      }
    };
  }  // namespace pa

  std::unique_ptr<audio_control_t> audio_control() {
    auto audio = std::make_unique<pa::server_t>();

    if (audio->init()) {
      return nullptr;
    }

    return audio;
  }
}  // namespace platf
