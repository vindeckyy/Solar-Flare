/**
 * @file src/config.h
 * @brief Declarations for the configuration of Sunshine.
 */
#pragma once

// standard includes
#include <array>
#include <bitset>
#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// local includes
#include "nvenc/nvenc_config.h"

namespace config {
  // Valid range for the packetsize limit
  constexpr int PACKETSIZE_MIN = 200;
  constexpr int PACKETSIZE_MAX = 65535;
  constexpr int PACKETSIZE_SMALL = 500;
  constexpr int PACKETSIZE_LARGE = 1456;

  // track modified config options
  inline std::unordered_map<std::string, std::string> modified_config_settings;

  // sensitive values that should be redacted from logging
  inline constexpr std::array redacted_config = {
    "csrf_allowed_origins"
  };

  void log_config_settings(const std::unordered_map<std::string, std::string> &vars, bool save);

  struct video_t {
    // ffmpeg params
    int qp;  // higher == more compression and less quality

    int hevc_mode;
    int av1_mode;

    int min_threads;  // Minimum number of threads/slices for CPU encoding

    struct {
      std::string sw_preset;
      std::string sw_tune;
      std::optional<int> svtav1_preset;
    } sw;

    nvenc::nvenc_config nv;
    bool nv_realtime_hags;
    bool nv_opengl_vulkan_on_dxgi;
    bool nv_sunshine_high_power_mode;

    // NVENC tuning preset. -1 = manual (don't touch anything); 0 = latency-
    // optimised (P1, bframes=0, zerolatency=true, lookahead=0); 1 =
    // balanced (P4, bframes=2, lookahead=20); 2 = quality-optimised
    // (P7, bframes=4, lookahead=40, twopass=full). Manual lets every
    // other nvenc_* key take effect; the presets override them.
    int nv_preset = -1;

    struct {
      int preset;
      int multipass;
      int h264_coder;
      int aq;
      int vbv_percentage_increase;
    } nv_legacy;

    struct {
      std::optional<int> qsv_preset;
      std::optional<int> qsv_cavlc;
      bool qsv_slow_hevc;
    } qsv;

    struct {
      std::optional<int> amd_usage_h264;
      std::optional<int> amd_usage_hevc;
      std::optional<int> amd_usage_av1;
      std::optional<int> amd_rc_h264;
      std::optional<int> amd_rc_hevc;
      std::optional<int> amd_rc_av1;
      std::optional<int> amd_enforce_hrd;
      std::optional<int> amd_quality_h264;
      std::optional<int> amd_quality_hevc;
      std::optional<int> amd_quality_av1;
      std::optional<int> amd_preanalysis;
      std::optional<int> amd_vbaq;
      int amd_coder;
    } amd;

    struct {
      int vt_allow_sw;
      int vt_require_sw;
      int vt_realtime;
      int vt_coder;
    } vt;

    struct {
      bool strict_rc_buffer;
    } vaapi;

    struct {
      int tune;  // 0=default, 1=hq, 2=ll, 3=ull, 4=lossless
      int rc_mode;  // 0=driver, 1=cqp, 2=cbr, 4=vbr
    } vk;

    std::string capture;
    std::string encoder;
    std::string adapter_name;
    std::string output_name;

    struct dd_t {
      struct workarounds_t {
        std::chrono::milliseconds hdr_toggle_delay;  ///< Specify whether to apply HDR high-contrast color workaround and what delay to use.
      };

      enum class config_option_e {
        disabled,  ///< Disable the configuration for the device.
        verify_only,  ///< @seealso{display_device::SingleDisplayConfiguration::DevicePreparation}
        ensure_active,  ///< @seealso{display_device::SingleDisplayConfiguration::DevicePreparation}
        ensure_primary,  ///< @seealso{display_device::SingleDisplayConfiguration::DevicePreparation}
        ensure_only_display  ///< @seealso{display_device::SingleDisplayConfiguration::DevicePreparation}
      };

      enum class resolution_option_e {
        disabled,  ///< Do not change resolution.
        automatic,  ///< Change resolution and use the one received from Moonlight.
        manual  ///< Change resolution and use the manually provided one.
      };

      enum class refresh_rate_option_e {
        disabled,  ///< Do not change refresh rate.
        automatic,  ///< Change refresh rate and use the one received from Moonlight.
        manual  ///< Change refresh rate and use the manually provided one.
      };

      enum class hdr_option_e {
        disabled,  ///< Do not change HDR settings.
        automatic  ///< Change HDR settings and use the state requested by Moonlight.
      };

      struct mode_remapping_entry_t {
        std::string requested_resolution;
        std::string requested_fps;
        std::string final_resolution;
        std::string final_refresh_rate;
      };

      struct mode_remapping_t {
        std::vector<mode_remapping_entry_t> mixed;  ///< To be used when `resolution_option` and `refresh_rate_option` is set to `automatic`.
        std::vector<mode_remapping_entry_t> resolution_only;  ///< To be use when only `resolution_option` is set to `automatic`.
        std::vector<mode_remapping_entry_t> refresh_rate_only;  ///< To be use when only `refresh_rate_option` is set to `automatic`.
      };

      config_option_e configuration_option;
      resolution_option_e resolution_option;
      std::string manual_resolution;  ///< Manual resolution in case `resolution_option == resolution_option_e::manual`.
      refresh_rate_option_e refresh_rate_option;
      std::string manual_refresh_rate;  ///< Manual refresh rate in case `refresh_rate_option == refresh_rate_option_e::manual`.
      hdr_option_e hdr_option;
      std::chrono::milliseconds config_revert_delay;  ///< Time to wait until settings are reverted (after stream ends/app exists).
      bool config_revert_on_disconnect;  ///< Specify whether to revert display configuration on client disconnect.
      mode_remapping_t mode_remapping;
      workarounds_t wa;
    } dd;

    int max_bitrate;  // Maximum bitrate, sets ceiling in kbps for bitrate requested from client
    double minimum_fps_target;  ///< Lowest framerate that will be used when streaming. Range 0-1000, 0 = half of client's requested framerate.
  };

  struct audio_t {
    std::string sink;  ///< Audio output device/sink to use for audio capture
    std::string virtual_sink;  ///< Virtual audio sink for audio routing
    bool stream;  ///< Enable audio streaming to clients
    bool install_steam_drivers;  ///< Install Steam audio drivers for enhanced compatibility
  };

  constexpr int ENCRYPTION_MODE_NEVER = 0;  // Never use video encryption, even if the client supports it
  constexpr int ENCRYPTION_MODE_OPPORTUNISTIC = 1;  // Use video encryption if available, but stream without it if not supported
  constexpr int ENCRYPTION_MODE_MANDATORY = 2;  // Always use video encryption and refuse clients that can't encrypt

  struct stream_t {
    std::chrono::milliseconds ping_timeout;

    std::string file_apps;

    int fec_percentage;

    // Video encryption settings for LAN and WAN streams
    int lan_encryption_mode;
    int wan_encryption_mode;

    // Limit the packetsize to avoid fragmentation on a low MTU link
    int packetsize;
  };

  struct nvhttp_t {
    // Could be any of the following values:
    // pc|lan|wan
    std::string origin_web_ui_allowed;

    std::string pkey;
    std::string cert;

    std::string sunshine_name;

    std::string file_state;

    std::string external_ip;
  };

  struct input_t {
    std::unordered_map<int, int> keybindings;

    std::chrono::milliseconds back_button_timeout;
    std::chrono::milliseconds key_repeat_delay;
    std::chrono::duration<double> key_repeat_period;

    std::string gamepad;
    bool ds4_back_as_touchpad_click;
    bool motion_as_ds4;
    bool touchpad_as_ds4;
    bool ds5_inputtino_randomize_mac;

    bool keyboard;
    bool key_rightalt_to_key_win;
    bool mouse;
    bool controller;

    bool always_send_scancodes;

    bool high_resolution_scrolling;
    bool native_pen_touch;
  };

  namespace flag {
    enum flag_e : std::size_t {
      PIN_STDIN = 0,  ///< Read PIN from stdin instead of http
      FRESH_STATE,  ///< Do not load or save state
      FORCE_VIDEO_HEADER_REPLACE,  ///< force replacing headers inside video data
      UPNP,  ///< Try Universal Plug 'n Play
      CONST_PIN,  ///< Use "universal" pin
      FLAG_SIZE  ///< Number of flags
    };
  }  // namespace flag

  struct prep_cmd_t {
    prep_cmd_t(std::string &&do_cmd, std::string &&undo_cmd, bool &&elevated):
        do_cmd(std::move(do_cmd)),
        undo_cmd(std::move(undo_cmd)),
        elevated(std::move(elevated)) {
    }

    explicit prep_cmd_t(std::string &&do_cmd, bool &&elevated):
        do_cmd(std::move(do_cmd)),
        elevated(std::move(elevated)) {
    }

    std::string do_cmd;
    std::string undo_cmd;
    bool elevated;
  };

  struct sunshine_t {
    std::string locale;
    int min_log_level;
    std::bitset<flag::FLAG_SIZE> flags;
    std::string credentials_file;

    std::string username;
    std::string password;
    std::string salt;

    std::string config_file;

    struct cmd_t {
      std::string name;
      int argc;
      char **argv;
    } cmd;

    std::uint16_t port;
    std::string address_family;
    std::string bind_address;

    std::string log_file;
    bool notify_pre_releases;
    bool system_tray;
    std::vector<prep_cmd_t> prep_cmds;

    // List of allowed origins for CSRF protection (e.g., "https://example.com,https://app.example.com")
    // Comma-separated list of additional origins. Default includes localhost variants and web UI port.
    std::vector<std::string> csrf_allowed_origins;
  };

  // ----------------------------------------------------------------------
  // SolarFlare fork tunables (Linux local-LAN fast path).
  //
  // These are the knobs the README promises exist. Every default here
  // matches the previous hardcoded value, so a vanilla install behaves
  // identically to the pre-config-fork build. Set any value to its
  // "fall back to upstream" choice to disable the SolarFlare tuning
  // for that subsystem without rebuilding.
  // ----------------------------------------------------------------------
  struct solarflare_t {
    // SO_BUSY_POLL on the ENet socket, in microseconds. 0 disables
    // busy polling entirely. 50 is a good middle ground for 1-2.5 GbE;
    // 0-200 is the practical range (kernel cap is 10000 = 10 ms).
    int busy_poll_us = 50;

    // Percent of the negotiated link speed used as the rate-control
    // pacer in src/stream.cpp. Valid range 50-95. The previous
    // hardcoded value was 80.
    int rate_cap_pct = 80;

    // Grow ENet send/recv buffers to 4 MiB on Linux so a 4K60 stream
    // never blocks on sendmsg(). Set false to use the kernel default.
    bool enet_4mib_buffer = true;

    // PW_KEY_NODE_LATENCY hint (ms) passed to the compositor. Mutter
    // and most other compositors honour the hint, cutting 1-2 frames
    // of pre-encoder buffering. Range 1-40; values below 4 may cause
    // pipewire to drop frames under load.
    int pipewire_latency_ms = 8;

    // On Linux, when adjust_thread_priority(critical) is called we also
    // push onto SCHED_RR prio 10 and pin to a non-IRQ, non-SMT core.
    // Set false to fall back to upstream's nice-only behaviour.
    bool cpu_pinning = true;

    /**
     * @brief SolarFlare audio_fx pre-processor and Opus tuning.
     *
     * All values default to "disabled / upstream-compatible". When @c
     * opus_application / @c opus_vbr / @c opus_complexity / @c opus_fec /
     * @c opus_expected_loss_pct are left at their defaults, the encoder
     * behaves identically to upstream Sunshine. Turning on the FX stages
     * (@c enable_agc, @c enable_vad, etc.) adds a small CPU cost in
     * exchange for smoother loudness, intelligibility, and noise
     * suppression.
     */
    struct audio_fx_t {
      // --- Pre-encoder audio FX (all off by default) ---
      /// Apply automatic gain control before encoding.
      bool enable_agc = false;
      /// Run voice activity detection (used by the ducker).
      bool enable_vad = false;
      /// Apply ducking when voice is active.
      bool enable_ducking = false;
      /// Apply a noise gate (suppress signal below @c noise_gate_threshold_db).
      bool enable_noise_gate = false;
      /// Noise-gate threshold (dBFS). Signal below this is attenuated.
      float noise_gate_threshold_db = -55.0f;

      // --- AGC tunables ---
      float agc_target_rms_db = -20.0f;
      float agc_max_gain_db = 12.0f;
      float agc_min_gain_db = -12.0f;
      float agc_attack_ms = 10.0f;
      float agc_hold_ms = 200.0f;
      float agc_release_ms = 100.0f;

      // --- VAD tunables ---
      float vad_threshold_db = -45.0f;
      float vad_hysteresis_db = 6.0f;
      float vad_min_speech_ms = 100.0f;
      float vad_min_silence_ms = 200.0f;

      // --- Ducker tunables ---
      float ducker_target_attenuation_db = -12.0f;
      float ducker_attack_ms = 50.0f;
      float ducker_release_ms = 500.0f;

      // --- Opus encoder tunables ---
      /// Opus application mode: 0 = LOWDELAY (default), 1 = VOIP, 2 = AUDIO.
      int opus_application = 0;
      /// Opus VBR mode: 0 = OFF (CBR), 1 = CONSTRAINED, 2 = FULL.
      int opus_vbr = 0;
      /// Opus complexity (0-10). Default 10 (max quality).
      int opus_complexity = 10;
      /// Enable Opus in-band FEC. Default true.
      bool opus_fec = true;
      /// Expected packet loss percentage (0-100). 0 disables the hint.
      int opus_expected_loss_pct = 0;
      /// Enable Opus bandwidth extension (super-wideband / fullband).
      bool opus_bandwidth_extension = true;
    } audio_fx {};

    /// Enable DSCP QoS tagging (IPTOS_LOWDELAY | IPTOS_THROUGHPUT) on the
    /// ENet streaming socket. Routers honour this to prioritize the stream
    /// over bulk traffic. Linux-only; no-op elsewhere.
    /// ponytail: one setsockopt, measurable on congested LANs.
    bool dscp_qos = true;

    /// Auto-set GPU to performance power profile during stream (via sysfs on
    /// AMD, nvidia-smi on NVIDIA), restore to auto on disconnect. Linux-only.
    /// ponytail: two sysfs writes, ~0.3ms of latency saved at high FPS.
    bool gpu_governor = true;

    /// Create a virtual DRM display if no physical outputs are detected, so
    /// the headless server can stream. Linux-only; uses xrandr dummy output.
    /// ponytail: one xrandr --auto call, no kernel params needed.
    bool headless_virtual_display = false;
  };

  /**
   * @brief Apply the NVENC tuning preset to nv_* fields.
   *
   * Call after changing @c video.nv_preset at runtime (e.g. per-game override).
   * ponytail: small helper so the big switch lives in one place.
   */
  void apply_nvenc_tuning_preset();

  extern video_t video;
  extern audio_t audio;
  extern stream_t stream;
  extern nvhttp_t nvhttp;
  extern input_t input;
  extern sunshine_t sunshine;
  extern solarflare_t solarflare;

  int parse(int argc, char *argv[]);
  std::unordered_map<std::string, std::string> parse_config(const std::string_view &file_content);
}  // namespace config
