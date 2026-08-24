# SolarFlare fork configuration

The SolarFlare fork adds Linux-only tunables for the local-LAN fast path.
You can turn each one on or off without rebuilding. (Originally tuned on
CachyOS; available on all distros supported by `scripts/linux-install.sh`.)
All live under the `solarflare_t` struct in `src/config.h` and are read
from the same `~/.config/sunshine/sunshine.conf` file as the upstream
options. They appear in `sunshine --version` (with
`min_log_level = 1`) as `config: '...' = ...` lines.

This document describes only the **fork-specific** keys. The full
upstream config documentation lives at
[docs/configuration.md](configuration.md); every option there is
still supported.

## The tunables at a glance

| Key | Type | Default | Range | What it does |
|---|---|---|---|---|
| `busy_poll_us`        | int    | 50   | 0-10000 | `SO_BUSY_POLL` on the ENet UDP socket, in microseconds. 0 disables. |
| `rate_cap_pct`        | int    | 80   | 50-95   | Percent of the negotiated link speed used as the rate-control pacer. |
| `enet_4mib_buffer`    | bool   | true | -       | Grow ENet UDP send/recv buffers to 4 MiB (Linux only). |
| `pipewire_latency_ms` | int    | 8    | 1-40    | `PW_KEY_NODE_LATENCY` hint passed to the PipeWire compositor. |
| `cpu_pinning`         | bool   | true | -       | Push the capture thread onto `SCHED_RR` and pin it to a non-IRQ, non-SMT core. |
| `dscp_qos`            | bool   | true | -       | Tag ENet packets with DSCP CS3 so routers prioritize streaming over bulk traffic (Linux only). |
| `gpu_governor`        | bool   | true | -       | Raise AMD DRM cards to `performance` for async capture; RAII restores `auto` on teardown (Linux only). |
| `headless_virtual_display` | bool | false | -    | If no displays detected, try creating a virtual xrandr output (Linux only, opt-in). |
| `headless_mode`       | bool | false | - | Route game launches into a private nested compositor (Web UI Headless tab). |
| `linux_use_cage_compositor` | bool | false | - | Use labwc nested compositor for headless streaming. |
| `compositor_backend`  | string | auto | auto/labwc/krfb/gamescope | Headless display backend selection. |
| `headless_width`      | int | 0   | 0-7680  | Override the headless virtual display width in pixels. 0 follows the client's requested resolution. |
| `headless_height`     | int | 0   | 0-4320  | Override the headless virtual display height in pixels. 0 follows the client's requested resolution. |
| `headless_refresh`    | int | 0   | 0-240   | Override the headless virtual display refresh rate in Hz. 0 follows the client's requested framerate. |
| `idle_timeout_min`    | int | 0   | 0-600   | Automatically stop a stream after this many minutes without client input. 0 disables the watchdog. |
| `skip_wayland_correlation` | bool | false | -    | Skip Wayland monitor correlation during KMS display enumeration. Only needed if the compositor still fails to report output metadata; leaving it `false` preserves absolute mouse coordinates. |
| `latency_mode`        | string | safe | safe/aggressive | Select bounded safe defaults or tighter latency-first media/scaling behavior. |
| `nvenc_tuning_preset` | int    | -1   | -1 to 2 | One-click NVENC profile: -1 (manual), 0 (latency), 1 (balanced), 2 (quality). |
| `trusted_subnets`     | string | ""   | -       | Comma-separated CIDR subnets allowed for auto-pairing without PIN (e.g. `10.0.0.0/8,192.168.1.0/24`). |
| `trusted_subnet_auto_pairing` | bool | false | - | Automatically pair clients connecting from `trusted_subnets`. |
| `api_tokens`          | string | ""   | -       | Scoped automation API tokens (`name\thash\tsalt\tscopes\|...`). Managed via Web UI or `/api/tokens`. |
| `webhook_secret`      | string | "" | -      | HMAC-SHA256 secret used to sign webhook payloads (`X-Solarflare-Signature` header). |
| `webhook_url_<n>`     | string | "" | -      | A webhook URL notified on stream start/stop. Numbered keys, e.g. `webhook_url_0`. |

Audio pre-processor and Opus encoder tunables are documented in the
[Audio FX](#audio-fx-pre-encoder-processing) section below. All 24
`sf_audio_*` / `sf_opus_*` keys default to upstream-compatible values and are
available in the Web UI's Audio/Video configuration tab.

Each one is opt-out: setting it back to its "fall back to upstream"
choice (`busy_poll_us = 0`, `rate_cap_pct = 80` is already upstream's
default, `enet_4mib_buffer = false`, `pipewire_latency_ms = 8` is
upstream's default, `cpu_pinning = false`) effectively undoes the
SolarFlare tuning for that subsystem without a rebuild. This is
useful if you want to A/B-test the patch on a per-knob basis.

## Detailed behaviour

### `busy_poll_us`

`setsockopt(SO_BUSY_POLL, ...)` on the ENet UDP socket. The kernel
polls the NIC for incoming packets for the configured number of
microseconds instead of sleeping until the next interrupt.

- **0**: disables busy polling entirely. Falls back to interrupt-driven
  receive.
- **50** (default): the sweet spot for 1-2.5 GbE and Wi-Fi 6/7.
- **100-200**: pushes the receive-side wakeup latency down further at
  the cost of ~1% extra CPU on a busy core.
- **> 1000** (i.e. 1 ms+): almost always wrong. The kernel silently
  clamps to `net.core.busy_poll` (`/proc/sys/net/core/busy_read`).

Out-of-range values (>10000) are silently rejected by the
`int_between_f` clamp and the default is retained.

### `rate_cap_pct`

Percent of the negotiated link speed used as the rate-control pacer
in `src/stream.cpp`. The active interface's speed is auto-detected
from `/sys/class/net/<iface>/speed` on Linux (via `getifaddrs`), so a
2.4 Gbps Wi-Fi 7 card or 2.5 GbE NIC is no longer capped at the old
hardcoded 1 Gbps. On other platforms the rate-control falls back to 1
Gbps.

- **50**: very conservative; leaves 50% headroom for TCP retransmits
  and other LAN traffic. Use this if you're sharing the link with
  other devices.
- **80** (default): the upstream behaviour. Solid for dedicated
  point-to-point links.
- **90-95**: aggressive; pushes close to the link ceiling. Pick this
  on a clean wired link with no other traffic.

Values outside 50-95 are silently rejected.

### `enet_4mib_buffer`

If `true` (default), grow the ENet UDP socket's send/recv buffers to
4 MiB so a 4K60 HEVC stream (~25 Mbps) never blocks on `sendmsg()`.

`SO_RCVBUFFORCE` / `SO_SNDBUFFORCE` are tried first. These let us
exceed `rmem_max`/`wmem_max` without sysctl changes (they require
`CAP_NET_ADMIN`, which Sunshine doesn't run with, so the call
silently no-ops if it fails). The code then falls back to plain
`SO_RCVBUF`/`SO_SNDBUF` for the rmem_max-limited path.

If `false`, the kernel default (~200 KiB on a fresh install) is used.
This is fine for 1080p60 but causes sendmsg stalls on 4K60.

### `pipewire_latency_ms`

`PW_KEY_NODE_LATENCY` hint passed to the PipeWire compositor via
`pw_properties_set`. The default 8 ms cuts 1-2 frames of pre-encoder
buffering compared to the upstream PipeWire default of ~20-40 ms.
Mutter (GNOME) and most other compositors honour the hint; KWin
sometimes does, sometimes doesn't.

- **1-3 ms**: aggressive; only use on wired links with a beefy GPU.
- **4-8 ms** (default range): the sweet spot for local LAN at
  1080p120 / 4K60.
- **12-20 ms**: relaxed; if you're seeing compositor-side frame
  drops, bump into this range.
- **> 20 ms**: defeats the point; PipeWire's internal buffer is
  already in this range upstream.

Values outside 1-40 are silently rejected. The value is formatted as a
fraction of a second (`ms/1000`), as required by PipeWire. For example,
the default is sent as `8/1000`.

### `latency_mode`

`safe` keeps the latency reductions that do not intentionally trade visual
quality or scheduler headroom. `aggressive` tightens the audio capture queue
from four packet durations to two and uses FFmpeg's fast-bilinear software
scaler. Hardware scaling and encoding are unchanged. Unknown values are
ignored and leave the previous valid mode active.

### `cpu_pinning`

If `true` (default), on `adjust_thread_priority(critical)` the thread
is also pushed onto `SCHED_RR` priority 10 and pinned to a physical
core (round-robin across cores 1..N/2, skipping core 0 to avoid the
default IRQ affinity shadow and skipping SMT siblings).

Removes the 5-15 ms CFS tail-latency spikes that show up as frame
jitter under load, and keeps the thread's L1/L2 cache warm
frame-to-frame. The calls fail silently under containers, systemd-run
units, or non-`CAP_SYS_NICE` users. The upstream nice-only path
still applies.

If `false`, only the upstream `nice -15` is applied. Use this if:

- You're running under `systemd-run --user --scope` and the SCHED_RR
  call is producing noisy warnings in
  `journalctl --user -u app-dev.lizardbyte.app.Sunshine.service`.
- You're on a Zen 1 / Bulldozer-era CPU where pinning to a single
  physical core actually hurts throughput more than it helps.

### `dscp_qos`

Tag ENet streaming packets with IP DSCP CS3 (Differentiated Services Code
Point, class selector 3). Routers that honour QoS can then prioritise the
game-stream traffic over bulk downloads, web browsing, or other LAN traffic
when the link is congested.

The tag is applied via `setsockopt(IP_TOS, ...)` with `IPTOS_LOWDELAY |
IPTOS_THROUGHPUT` on the same ENet socket that carries stream data.

- **true** (default): CS3 tag is set. Routers can queue the stream ahead of
  best-effort traffic.
- **false**: no QoS tag. The stream competes equally with all other traffic.

Linux-only. Has no effect on macOS or Windows (the socket option exists but
no common consumer router groks DSCP from those platforms the same way).

### `gpu_governor`

When async capture starts on Linux and this is enabled, write `performance`
to `/sys/class/drm/cardN/device/power_dpm_force_performance_level` for DRM
cards 0-3. A RAII guard (`gpu_governor_guard_t`) lives on the async capture
context, so destruction restores `auto` even when teardown skips the
happy-path end-capture loop. Missing or unwritable paths are skipped
silently. Non-AMD hardware is a no-op.

- **true** (default): raise to `performance` for the capture lifetime, restore `auto` on teardown.
- **false**: no GPU power-profile changes.

Linux AMD sysfs only.

### `headless_virtual_display`

If the system has no physical display outputs detected during startup, try
creating a virtual display via `xrandr --setprovideroutputsource` and
`xrandr --auto` so the capture backend has something to grab.

Designed for headless servers (no monitor plugged in) that still want to
stream a desktop. The virtual output is the `VIRTUAL1` provider driven by
`xrandr`. Use [`headless_width`](#headless_width) /
[`headless_height`](#headless_height) / [`headless_refresh`](#headless_refresh)
to set an explicit mode; without them `xrandr --auto` reports a
1920x1080@60 mode.

- **true**: create virtual display if no physical outputs are found.
- **false** (default): no virtual display; the capture backend will report
  "no display" and Sunshine will refuse to start a stream.

Linux-only, requires an X11 display server running (Xorg or XWayland).

### `headless_width` / `headless_height` / `headless_refresh` <a id="headless_width"></a><a id="headless_height"></a><a id="headless_refresh"></a>

Override the resolution and refresh rate of the headless virtual display
instead of always following the client's requested mode. Applied both to the
headless compositor (labwc / krfb / gamescope) and to the `xrandr --output
VIRTUAL1` fallback path.

- `headless_width`: virtual display width in pixels. 0 = follow the client.
- `headless_height`: virtual display height in pixels. 0 = follow the client.
- `headless_refresh`: virtual display refresh rate in Hz. 0 = follow the
  client's requested framerate.

These are read at launch, so edits to `sunshine.conf` take effect on the next
streamed session.

### `skip_wayland_correlation`

By default Sunshine correlates Wayland output IDs with KMS connector IDs so
absolute mouse coordinates land on the right monitor. This requires output
metadata from the compositor via the `wl_output` and `xdg_output` protocols.

- **true**: skip the correlation step entirely. The KMS display enumeration
  proceeds directly without waiting for Wayland output events. You lose
  absolute mouse-to-monitor mapping (useful for multi-monitor setups).
- **false** (default): normal Wayland correlation. With current SolarFlare
  builds this no longer requires a blocking `wl_display_roundtrip()` on KWin,
  and you should leave it disabled so KMS capture keeps the correct resolution
  and absolute mouse coordinates work.

Set this to `true` only if your compositor still fails to deliver output
metadata and you need a workaround.

## Audio FX (pre-encoder processing)

SolarFlare adds a lightweight audio pre-processor that runs between the
PipeWire capture callback and the Opus encoder. All stages are opt-in:
every toggle defaults to `false` (off), matching upstream behaviour.

### At a glance

| Key | Type | Default | Range | What it does |
|---|---|---|---|---|
| `sf_audio_agc` | bool | false | - | Automatic gain control; smooths loudness |
| `sf_audio_agc_target_db` | float | -20 | -40 to -6 | Target RMS loudness (dBFS) |
| `sf_audio_agc_max_gain_db` | float | 12 | 0 to 30 | Max boost the AGC can apply (dB) |
| `sf_audio_agc_min_gain_db` | float | -12 | -30 to 0 | Max cut the AGC can apply (dB) |
| `sf_audio_agc_attack_ms` | float | 10 | 1 to 500 | How fast AGC rides up to target |
| `sf_audio_agc_hold_ms` | float | 200 | 0 to 5000 | Hold period before gain release |
| `sf_audio_agc_release_ms` | float | 100 | 1 to 5000 | How fast AGC returns to baseline |
| `sf_audio_vad` | bool | false | - | Voice activity detection (drives ducking) |
| `sf_audio_vad_threshold_db` | float | -45 | -80 to -10 | VAD trigger level (dBFS) |
| `sf_audio_vad_hysteresis_db` | float | 6 | 0 to 30 | Hysteresis band around threshold |
| `sf_audio_vad_min_speech_ms` | float | 100 | 10 to 2000 | Min speech duration to trigger VAD |
| `sf_audio_vad_min_silence_ms` | float | 200 | 10 to 5000 | Min silence to release VAD |
| `sf_audio_ducking` | bool | false | - | Lower game volume when speech detected |
| `sf_audio_ducker_attenuation_db` | float | -12 | -40 to 0 | How much to cut game audio during speech |
| `sf_audio_ducker_attack_ms` | float | 50 | 1 to 2000 | Ramp-down speed when speech starts |
| `sf_audio_ducker_release_ms` | float | 500 | 1 to 5000 | Ramp-up speed when speech ends |
| `sf_audio_noise_gate` | bool | false | - | Mute signal below threshold |
| `sf_audio_noise_gate_db` | float | -55 | -90 to -10 | Noise gate threshold (dBFS) |
| `sf_opus_application` | int | 0 | 0-2 | 0 = LOWDELAY, 1 = VOIP, 2 = AUDIO |
| `sf_opus_vbr` | int | 0 | 0-2 | 0 = CBR, 1 = Constrained VBR, 2 = Full VBR |
| `sf_opus_complexity` | int | 10 | 0-10 | Opus encoder CPU/quality trade-off |
| `sf_opus_fec` | bool | true | - | In-band forward error correction |
| `sf_opus_expected_loss_pct` | int | 0 | 0-100 | Hint to Opus for FEC bit allocation |
| `sf_opus_bandwidth_extension` | bool | true | - | Allow >16 kHz audio bandwidth |

### AGC

When `sf_audio_agc = true`, the pre-processor measures the RMS level of
each audio frame and applies a smooth gain correction to push it toward
`sf_audio_agc_target_db`. The gain is clamped between `sf_audio_agc_min_gain_db`
and `sf_audio_agc_max_gain_db`, and ramps at the rate set by `*_attack_ms`
(when the signal is too quiet) and `*_release_ms` (when it's too loud).
A hold period (`sf_audio_agc_hold_ms`) prevents gain pumping on short
transients.

The default target of -20 dBFS is a good middle ground: louder than a
mixed-content stream but with enough headroom for sudden peaks.

### VAD

When `sf_audio_vad = true`, the pre-processor classifies each frame as
speech or non-speech by comparing its power to `sf_audio_vad_threshold_db`.
A hysteresis band (`sf_audio_vad_hysteresis_db`) prevents chatter toggling
on low-frequency noise. Minimum durations (`sf_audio_vad_min_speech_ms` /
`sf_audio_vad_min_silence_ms`) filter out clicks and brief pauses.

VAD alone changes nothing about the audio output; it only produces a
voice-active signal that other stages (ducking, noise gate) can consume.

### Ducking

When both `sf_audio_vad = true` and `sf_audio_ducking = true`, the
pre-processor attenuates the game-audio channel by
`sf_audio_ducker_attenuation_db` whenever the microphone channel is
voice-active. This makes speech more intelligible during loud gameplay
without the listener needing to adjust volume manually.

The ducker ramps in (`sf_audio_ducker_attack_ms`) and out
(`sf_audio_ducker_release_ms`) smoothly to avoid audible pumping.

### Noise gate

When `sf_audio_noise_gate = true`, any audio frame whose power is below
`sf_audio_noise_gate_db` is zeroed. This kills constant background hiss,
fan noise, or open-mic floor noise without affecting louder content.

A threshold of -55 dBFS works for most desktop microphones; lower values
(-70 to -90) for very quiet rooms, higher (-30 to -40) for noisy
environments.

### Opus encoder tuning

These keys tune the Opus encoder that produces the audio stream sent to the
client. Upstream Sunshine defaults are: LOWDELAY application, CBR,
complexity 10, FEC on, bandwidth extension on. Every default here matches
that, so a vanilla install is unchanged.

- `sf_opus_application`: 0 (LOWDELAY) is best for game streaming where
  every millisecond matters. 1 (VOIP) trades a small latency increase for
  better speech intelligibility. 2 (AUDIO) prioritises music and
  sound-effect quality.
- `sf_opus_vbr`: 0 (CBR) is the safe default for variable-bandwidth
  networks. 1 (Constrained VBR) uses slightly fewer bits on easy passages
  and more on complex ones but stays near the target bitrate. 2 (Full VBR)
  can spike above the target; use only on links with generous headroom.
- `sf_opus_complexity`: 10 gives the best quality/bitrate ratio. Lower
  values save CPU at the cost of audio quality at the same bitrate.
- `sf_opus_fec`: When on, Opus embeds a redundant, lower-bitrate copy of
  each frame so a single lost packet can be reconstructed. Adds ~1 kbps of
  overhead. Disable if you need every bit for video.
- `sf_opus_expected_loss_pct`: Tells Opus how much packet loss to expect
  so it can pre-allocate FEC bits efficiently. 0 = no hint (Opus adapts
  naturally). Set to 5-10 on Wi-Fi links with spotty coverage.
- `sf_opus_bandwidth_extension`: Allows Opus to encode up to 48 kHz
  (fullband). Disable if the client or network can't handle >16 kHz audio.

## Where these are used

| Tunable              | Files |
|----------------------|-------|
| `busy_poll_us`       | `src/network.cpp` |
| `rate_cap_pct`       | `src/stream.cpp` |
| `enet_4mib_buffer`   | `src/network.cpp` |
| `pipewire_latency_ms`| `src/platform/linux/pipewire.cpp` |
| `cpu_pinning`        | `src/platform/linux/misc.cpp` |
| `dscp_qos`           | `src/network.cpp` |
| `gpu_governor`       | `src/gpu_governor.cpp`, `src/video.cpp` |
| `headless_virtual_display` | `src/video.cpp` |
| `headless_width` / `headless_height` / `headless_refresh` | `src/process.cpp`, `src/video.cpp` |
| `idle_timeout_min` | `src/stream.cpp` |
| `skip_wayland_correlation` | `src/platform/linux/kmsgrab.cpp`, `src/platform/linux/misc.cpp` |
| `latency_mode`       | `src/stream.cpp`, `src/video.cpp`, `src/audio.cpp` |
| `webhook_secret` / `webhook_url_<n>` | `src/webhooks.cpp`, `src/stream.cpp` |
| `client_profile_<name>_*` | `src/client_profiles.cpp`, `src/nvhttp.cpp` |
| `sf_audio_*`         | `src/audio.cpp`, `src/config.cpp` |
| `sf_opus_*`          | `src/audio.cpp`, `src/config.cpp` |

## Webhooks

SolarFlare can notify external services when a streaming session starts or
ends. Each webhook URL is a numbered key:

```bash
webhook_url_0 = https://example.com/hooks/solarflare
webhook_url_1 = https://discord.com/api/webhooks/...
webhook_secret = change-me
```

On `stream.start` / `stream.end`, SolarFlare POSTs a JSON payload to every
configured URL (fire-and-forget, 5 s timeout):

```json
{
  "event": "stream.start",
  "app_name": "Cyberpunk 2077",
  "client_name": "Moonlight-PC",
  "client_address": "192.168.1.10",
  "codec": "hevc_vaapi",
  "width": 1920,
  "height": 1080,
  "fps": 60,
  "avg_bitrate_kbps": 20000.0,
  "avg_rtt_ms": 2.5,
  "avg_encode_ms": 1.2,
  "dropped_frames": 0,
  "error": ""
}
```

When `webhook_secret` is set, each request carries an
`X-Solarflare-Signature: sha256=<hex>` header (HMAC-SHA256 over the body) so
receivers can verify the payload came from this host. Session history is
also written to `<appdata>/session_history.jsonl` and exposed through
`GET /api/sessions`.

## Per-client streaming profiles

Clients on different devices often want different streaming settings. Set a
profile per client device name (the `uniqueid` Moonlight sends on launch):

```bash
client_profile_Phone_max_bitrate = 15000
client_profile_Phone_hevc_mode = 2
client_profile_Phone_av1_mode = 0
client_profile_Phone_latency_mode = aggressive

client_profile_PC_max_bitrate = 60000
client_profile_PC_hevc_mode = 5
```

Supported fields: `max_bitrate` (kbps), `hevc_mode`, `av1_mode`,
`latency_mode` (`safe` / `aggressive`). A value of `0` (or empty) means
"use the global config". Profiles are applied at `/launch` before encoder
probing and restored when the session ends, so a phone session can use
lower bitrate while the global config stays untouched.

## Idle session auto-stop

`idle_timeout_min` (default `0` = disabled) stops a streaming session after
the given number of minutes without any client input (mouse, keyboard,
gamepad), freeing the capture/encode pipeline and letting other clients
connect. The watchdog checks every few seconds and logs a distinct
"Idle timeout" reason. The key is hot-reloadable via the config watcher.

## NVENC tuning presets

`nvenc_tuning_preset` provides single-knob tuning for NVIDIA NVENC encoders without manually tweaking individual low-level parameters:

- `-1` (default): Manual / driver defaults.
- `0` (Latency): Configures low-latency rate control, disabled B-frames, and zerolatency tuning.
- `1` (Balanced): Balanced rate control and multi-pass encoding.
- `2` (Quality): High-quality 2-pass encoding with spatial adaptive quantization (`spatial_aq`).

## Trusted subnets and auto-pairing

Configure CIDR subnets allowed to connect and pair without requiring a PIN:

```bash
trusted_subnets = 192.168.1.0/24,10.0.0.0/8
trusted_subnet_auto_pairing = enabled
```

When `trusted_subnet_auto_pairing` is enabled, Moonlight clients originating from any address within `trusted_subnets` are automatically paired on first connection.

## Scoped API tokens

External scripts and automation tools can authenticate to the REST API using scoped
bearer tokens configured in `sunshine.conf` or minted via `POST /api/tokens`.
is shown exactly once when minted via `POST /api/tokens` or the Web UI. In
`sunshine.conf`, each token is a TOML-style object inside the `api_tokens` array:

```toml
api_tokens = [
  { name = "home-assistant", token_hash = "abc123...", salt = "deadbeef", scopes = ["config:get", "apps:launch"] },
  { name = "ci-bot", token_hash = "def456...", salt = "cafebabe", scopes = ["apps:get", "apps:close", "logs:get"] }
]
```

Authenticate with `Authorization: Bearer <plaintext_token>`. Basic Auth with the
admin username/password still grants the wildcard `*` scope (full access).

| Scope | Endpoint access |
|---|---|
| `config:get` | Read `/api/config` |
| `config:set` | Write `/api/config` |
| `apps:get` | List apps (`GET /api/apps`) |
| `apps:launch` | Launch an app (`POST /api/apps`) |
| `apps:close` | Stop a running app (`POST /api/apps/close`) |
| `clients:list` | List paired clients |
| `clients:pair` | Pair a new client |
| `clients:unpair` | Unpair one or all clients |
| `logs:get` | Read the log file |
| `display:reset` | Reset display-device persistence |
| `tokens:manage` | CRUD API tokens via `/api/tokens` |
| `*` | All of the above (admin path only) |

Grant the minimum scopes needed. A Home Assistant automation that only launches
games needs `apps:launch` - not `config:set` or `clients:unpair`.

## Headless compositor streaming

SolarFlare can route games into a private nested compositor instead of capturing
your desktop. This is controlled by the **Headless Stream** tab keys documented
in [configuration.md](configuration.md):

| Key | Purpose |
|---|---|
| `headless_mode` | Master switch - launch apps into the private compositor |
| `linux_use_cage_compositor` | Route into labwc nested Wayland compositor |
| `linux_prefer_gpu_native_capture` | Prefer DMA-BUF capture over X11 fallback |
| `compositor_backend` | `auto`, `labwc`, `krfb`, or `gamescope` |
| `headless_width` / `headless_height` / `headless_refresh` | Fixed virtual display mode (0 = follow client) |

**Backend selection (`compositor_backend`):**

- **`auto`** (default): On KWin, prefer `krfb-virtualmonitor`; elsewhere use `labwc`.
- **`labwc`**: Nested labwc compositor - best general-purpose Wayland headless path.
- **`krfb`**: KWin virtual monitor integration - lowest overhead on KDE Plasma 6.
- **`gamescope`**: Gamescope micro-compositor - useful for per-game resolution scaling,
  FSR, and sandboxing Proton titles away from the desktop session.

`headless_virtual_display` (file-only fork key) is a separate fallback for bare
headless servers with **no** physical outputs: it creates an `xrandr VIRTUAL1`
output. Use it when Sunshine refuses to start because no display is detected.
Combine with `headless_width` / `headless_height` / `headless_refresh` for a
fixed 1920×1080@120 virtual panel on a datacenter GPU.

## Tuning recipes

Copy-paste snippets for common scenarios. All keys go in `~/.config/sunshine/sunshine.conf`
unless noted. Restart Sunshine (or rely on the config watcher for hot-reloadable keys)
after editing.

### Recipe: Wired 4K60 LAN (NVIDIA, lowest latency)

Goal: sub-5 ms glass-to-glass on a dedicated 2.5 GbE link.

```bash
# Network fast path
busy_poll_us = 100
rate_cap_pct = 90
enet_4mib_buffer = true
dscp_qos = true
cpu_pinning = true
latency_mode = aggressive
pipewire_latency_ms = 4

# NVENC one-knob preset (overrides individual nvenc_* when set)
nvenc_tuning_preset = 0

# Optional ceiling if clients request too much bitrate
max_bitrate = 80000
packetsize = 0
```

Per-game override in `apps.json`: `"encoder-preset": 0` for competitive titles.

### Recipe: Wi-Fi 6/7 handheld (Steam Deck, phone)

Goal: stable 1080p60 over lossy wireless with graceful degradation.

```bash
busy_poll_us = 50
rate_cap_pct = 70
pipewire_latency_ms = 12
latency_mode = safe
fec_percentage = 30
packetsize = 1024

# Client profile (replace Phone with your Moonlight device name)
client_profile_Phone_max_bitrate = 15000
client_profile_Phone_hevc_mode = 2
client_profile_Phone_latency_mode = aggressive

# Opus hints for packet loss
sf_opus_fec = true
sf_opus_expected_loss_pct = 8
sf_opus_vbr = 1
```

### Recipe: AMD VA-API Linux desktop

Goal: reliable HEVC on Mesa/RADV without encoder stalls.

```bash
gpu_governor = true
encoder = vaapi
vaapi_rc_mode = 3
vaapi_quality = 4
vaapi_strict_rc_buffer = true
vaapi_async_depth = 2
max_bitrate = 50000
```

If you see VBV underruns, raise `vaapi_rc_buffer_frames` to `2`.

### Recipe: Headless homelab (no monitor)

Goal: stream a 1080p desktop from a GPU server in a rack.

```bash
headless_virtual_display = true
headless_width = 1920
headless_height = 1080
headless_refresh = 60
headless_mode = true
compositor_backend = labwc
linux_use_cage_compositor = true
```

Launch **Desktop** from Moonlight; Sunshine creates the virtual output before capture.

### Recipe: Couch co-op with voice chat (audio FX)

Goal: intelligible microphone over loud game audio without manual volume riding.

```bash
sf_audio_agc = true
sf_audio_agc_target_db = -18
sf_audio_vad = true
sf_audio_ducking = true
sf_audio_ducker_attenuation_db = -15
sf_audio_noise_gate = true
sf_audio_noise_gate_db = -50
sf_opus_application = 1
```

Requires `stream_audio = enabled` and a working `audio_sink` / `virtual_sink` pair.

### Recipe: Shared LAN (roommate torrenting)

Goal: prioritize stream packets when the link is congested.

```bash
rate_cap_pct = 55
dscp_qos = true
busy_poll_us = 0
adaptive_bitrate_enabled = true
adaptive_bitrate_min = 5000
adaptive_bitrate_max = 40000
```

Also enable **QoS / WMM** on your router and map DSCP CS3 to the highest queue.

### Recipe: Trusted living-room TV (auto-pair)

Goal: Moonlight on a fixed set-top box pairs without entering a PIN each reinstall.

```bash
trusted_subnets = 192.168.1.0/24
trusted_subnet_auto_pairing = true
origin_web_ui_allowed = lan
```

Restrict subnets to the smallest CIDR that covers only trusted devices. Never use
`0.0.0.0/0`.

### Recipe: Home automation webhooks

Goal: turn off lights when streaming starts, restore when it ends.

```bash
webhook_url_0 = http://192.168.1.50:8123/api/webhook/solarflare_stream
webhook_secret = your-long-random-secret
idle_timeout_min = 30
```

Verify signatures in your receiver:

```python
import hmac, hashlib
expected = "sha256=" + hmac.new(secret.encode(), body, hashlib.sha256).hexdigest()
assert request.headers["X-Solarflare-Signature"] == expected
```

### Recipe: Intel QuickSync laptop (battery vs plugged-in)

**Plugged in / performance:**

```bash
qsv_preset = 4
qsv_slow_hevc = false
nvenc_tuning_preset = -1
encoder = qsv
max_bitrate = 60000
```

**On battery (lower power):**

```bash
qsv_preset = 7
max_bitrate = 20000
minimum_fps_target = 30
idle_timeout_min = 10
```

### Recipe: macOS VideoToolbox remote workstation

```bash
encoder = videotoolbox
vt_realtime = enabled
vt_coder = cabac
max_bitrate = 80000
```

VideoToolbox ignores most `nvenc_*` / `vaapi_*` keys - set encoder-specific tabs only.

### Recipe: Disable all SolarFlare tuning (upstream parity)

Useful for A/B testing or bug isolation:

```bash
busy_poll_us = 0
enet_4mib_buffer = false
cpu_pinning = false
dscp_qos = false
gpu_governor = false
latency_mode = safe
pipewire_latency_ms = 8
# Leave sf_audio_* / sf_opus_* at defaults (all off / upstream-compatible)
```

`rate_cap_pct = 80` is already the historical hardcoded value and needs no change.

## Config hot reload

SolarFlare polls `sunshine.conf` every 2 seconds (`start_config_watcher()` in
`config.cpp`). These fork keys reload without restart:

- All `sf_audio_*` / `sf_opus_*` tunables (next session)
- `idle_timeout_min`, `latency_mode`, `busy_poll_us`, `rate_cap_pct`,
  `enet_4mib_buffer`, `pipewire_latency_ms`, `cpu_pinning`, `dscp_qos`,
  `gpu_governor`, `skip_wayland_correlation`

Keys that require a **new stream session** (applied at launch): `headless_*`,
`client_profile_*`, `nvenc_tuning_preset`, encoder/capture selection.

## A quick A/B test

To verify the fork keys are working, run:

```bash
cat > /tmp/sf-test.conf <<'EOF'
min_log_level = 1
busy_poll_us = 0
rate_cap_pct = 95
enet_4mib_buffer = false
pipewire_latency_ms = 1
cpu_pinning = false
dscp_qos = false
gpu_governor = false
headless_virtual_display = true
skip_wayland_correlation = true
latency_mode = aggressive
sf_audio_agc = true
sf_audio_vad = true
sf_audio_ducking = true
sf_audio_noise_gate = true
sf_audio_noise_gate_db = -50
sf_audio_agc_target_db = -18
sf_audio_agc_max_gain_db = 15
sf_audio_agc_min_gain_db = -10
sf_audio_agc_attack_ms = 8
sf_audio_agc_hold_ms = 150
sf_audio_agc_release_ms = 80
sf_audio_vad_threshold_db = -40
sf_audio_vad_hysteresis_db = 5
sf_audio_vad_min_speech_ms = 80
sf_audio_vad_min_silence_ms = 150
sf_audio_ducker_attenuation_db = -15
sf_audio_ducker_attack_ms = 40
sf_audio_ducker_release_ms = 400
sf_opus_application = 1
sf_opus_vbr = 1
sf_opus_complexity = 8
sf_opus_fec = true
sf_opus_expected_loss_pct = 5
sf_opus_bandwidth_extension = true
EOF

sunshine /tmp/sf-test.conf
# Look for these in the first ~20 log lines:
#   config: 'busy_poll_us' = 0
#   config: 'rate_cap_pct' = 95
#   ...
#   config: 'sf_audio_agc' = true
#   config: 'sf_opus_application' = 1
#
# If the fork keys appear with no "Unrecognized" warnings, the fork
# config plumbing is wired correctly.
```

## Verification on an existing install

After installing a new SolarFlare build:

1. Start `app-dev.lizardbyte.app.Sunshine.service` and inspect its user journal.
   The startup log should show a clean version/commit, SolarFlare publisher
   metadata, and no fatal configuration errors.
2. Confirm the installed binary contains the fork identity with
   `strings /usr/local/bin/sunshine | grep -m1 SolarFlare`.
3. Open `https://localhost:47990`. Audio FX and Opus controls should appear in
   the Audio/Video tab. The lower-level network, scheduling, and capture
   tunables remain file-only controls in `~/.config/sunshine/sunshine.conf`.
4. Run `tests/unit/test_config_fork_keys.cpp` through `test_sunshine` when
   changing these keys; the test owns the source/default/documentation
   consistency contract.

## See also

- [docs/configuration.md](configuration.md): the full upstream
  configuration reference.
- [docs/PORTING.md](PORTING.md): multi-distro build instructions.
