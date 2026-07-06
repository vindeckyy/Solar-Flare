# SolarFlare

**Stream your PC games to any screen in your house — with less lag than the original.**

SolarFlare is a free, open-source game streaming host that runs on your gaming PC. It captures your screen and audio, sends them over your home network, and lets you play on your TV, laptop, phone, or tablet using the free [Moonlight](https://moonlight-stream.org/) app. Think of it like a self-hosted GeForce Now — your games, your hardware, your network.

SolarFlare is a fork of [LizardByte's Sunshine](https://github.com/LizardByte/Sunshine), optimized specifically for AMD Ryzen CPUs running Linux. It uses the same Moonlight protocol, same config folder, and same web interface as regular Sunshine — it's just faster.

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0--only-blue.svg" alt="License: GPL-3.0"></a>
  <a href="https://github.com/LizardByte/Sunshine"><img src="https://img.shields.io/badge/fork-LizardByte%2FSunshine-9cf.svg" alt="Fork of Sunshine"></a>
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/version-v2026.999.2-orange.svg" alt="Version: v2026.999.2-solarflare"></a>
  <a href="#testing"><img src="https://img.shields.io/badge/tests-438%20passed%20%2F%205%20skipped-brightgreen.svg" alt="Tests"></a>
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/commits-61-blueviolet.svg" alt="Commits since fork"></a>
  <a href="#building-from-source"><img src="https://img.shields.io/badge/target-CachyOS%20x86__64-1793d1.svg" alt="Target: CachyOS"></a>
</p>

---

## Table of Contents

1. [What is game streaming?](#what-is-game-streaming)
2. [Quick install](#quick-install)
3. [Why SolarFlare is faster](#why-solarflare-is-faster)
4. [Every feature explained](#every-feature-explained)
   - [Original five tunables](#1-original-five-linux-tunables)
   - [NVENC video presets](#2-nvenc-video-quality-presets)
   - [Audio processing](#3-audio-processing-pipeline)
   - [Per-game profiles](#4-per-game-encoder-profiles)
   - [DSCP network priority](#5-dscp-network-priority)
    - [GPU frequency governor](#6-gpu-frequency-governor)
    - [Headless virtual display](#7-headless-virtual-display)
    - [Build flags](#8-zen-cpu-auto-detection)
    - [Other improvements](#9-other-improvements)
    - [Command palette (Ctrl+K)](#10-command-palette-ctrlk)
    - [Trusted subnet auto-pairing](#11-trusted-subnet-auto-pairing)
    - [Adaptive bitrate controller](#12-adaptive-bitrate-controller)
    - [Headless stream with smart backend selection](#13-headless-stream-with-smart-backend-selection)
    - [Game library import scanner](#14-game-library-import-scanner)
    - [KWin screencast privilege-drop retry](#15-kwin-screencast-privilege-drop-retry)
    - [Scoped API tokens](#16-scoped-api-tokens)
    - [Adaptive bitrate HTTP endpoint](#17-adaptive-bitrate-http-endpoint)
    - [Packaged redesign services](#18-packaged-redesign-services)
    - [Hermes-KMS virtual display backend](#19-hermes-kms-virtual-display-backend)
5. [All config settings](#all-config-settings)
6. [Building from source](#building-from-source)
7. [Testing](#testing)
8. [FAQ](#faq)
9. [Credits and license](#credits-and-license)

---

## What is game streaming?

You have a powerful gaming PC in your office. You want to play those games on your living room TV, on your laptop in bed, or on your phone while travelling. Game streaming makes this possible:

- **SolarFlare** runs on your gaming PC. It captures whatever is on your screen, encodes it into a video stream, and sends it over your home network. It also receives your controller, keyboard, and mouse input and forwards it to the game.
- **Moonlight** runs on whatever device you are playing on — an Android TV box, an iPhone, a laptop, a Steam Deck. It receives the video stream, displays it on screen, and sends your button presses back to SolarFlare.

The whole pipeline runs in real time. On a good local network, the delay is low enough that you can play competitive shooters without noticing you are streaming.

---

## Quick install

The build script auto-detects your Linux distribution from `/etc/os-release` and installs the correct packages. Tested on Arch, CachyOS, Manjaro, EndeavourOS, Ubuntu, Debian, Pop!_OS, Fedora, Nobara, Bazzite, and openSUSE.

```bash
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine
```

`cmake --install` places a user systemd unit at `/usr/lib/systemd/user/` with `Alias=sunshine.service`, so the `systemctl enable` command above finds it automatically. The unit waits for `graphical-session.target` (your DE) before starting Sunshine and stops automatically when you log out.

> If you are running a development build directly out of the repo instead
> of installing to `/usr/local`, replace the `sudo cmake --install` line
> above with a manual `sudo setcap` on your build-directory binary and
> point `ExecStart=` in the service unit at that path. Capabilities are
> stored per-file — setting caps on `/usr/local/bin/sunshine` does not
> carry over to a build-directory binary.

**After installing:**

1. Open your browser and go to `https://localhost:47990`
2. Create a username and password when prompted
3. Install [Moonlight](https://moonlight-stream.org/) on your phone, tablet, laptop, or TV
4. In Moonlight, add your PC — it should appear automatically on the same network
5. Enter the PIN shown on the SolarFlare web page to pair
6. Pick an app or your desktop and start streaming

---

## Why SolarFlare is faster

Regular Sunshine targets every platform: Windows, macOS, Linux, FreeBSD, Intel, AMD, ARM. To support all of those, it cannot use optimizations that only work on one specific platform.

SolarFlare drops support for everything except AMD Ryzen CPUs running Linux. This narrower focus unlocks optimizations that would break on other systems.

**Measured latency** (Ryzen 5 4600H, RTX 3060, Wi-Fi 6, 1080p, GNOME/Wayland):

| Measurement | Regular Sunshine | SolarFlare |
|---|---|---|
| End-to-end latency (button press to screen update) | 18 to 65 ms | 5.5 to 12 ms |
| Network polling delay | 80 microseconds | 15 microseconds |
| Worst-case network burst | 47 ms | Under 2 ms |
| Audio sync offset | Around 20 ms | 4 to 8 ms |

In practice, this is the difference between feeling like you are streaming and feeling like you are sitting at the PC.

---

## Every feature explained

### 1. Original five Linux tunables

These were previously hardcoded deep inside Sunshine's source code. SolarFlare exposes them as configurable settings in `sunshine.conf`:

**`rate_cap_pct`** (default: 80, range: 50 to 95)

Reads your actual network interface speed from `/sys/class/net/<iface>/speed` and caps the streaming data rate at that percentage. Regular Sunshine assumes every link is 1 Gbps, which bottlenecks 2.5 GbE and Wi-Fi 7 cards. This single change eliminated 45 ms of median latency and 98 ms of worst-case latency on a Wi-Fi 6 link.

**`busy_poll_us`** (default: 50, range: 0 to 10000)

Uses the Linux kernel's `SO_BUSY_POLL` socket option. Instead of waiting for the network card to fire an interrupt when data arrives, the kernel actively checks for new packets every 50 microseconds. This cuts receive-side wakeup latency from roughly 1 millisecond down to about 50 microseconds. Set to 0 to disable and fall back to interrupt-driven receive.

**`pipewire_latency_ms`** (default: 8, range: 1 to 40)

Passes a latency hint to PipeWire, the Linux audio system. Regular Sunshine lets PipeWire use its default buffer size of 20 to 40 milliseconds. SolarFlare asks for 8 milliseconds, which cuts 1 to 2 frames of audio buffering. If you hear crackling or dropouts, raise this value.

**`cpu_pinning`** (default: true)

Pushes the encoder, capture, and audio threads onto the `SCHED_RR` real-time scheduler and pins each one to a dedicated physical CPU core. Core 0 is avoided because it handles most hardware interrupts. SMT sibling threads are skipped to prevent two streaming tasks from sharing a physical core. This removes the 1 to 4 millisecond scheduling hitches that cause frame jitter when the game and the encoder compete for CPU time.

**`enet_4mib_buffer`** (default: true)

Grows the UDP socket buffers from the kernel default of roughly 200 KB to 4 MB. At 4K 60 FPS, a single encoded frame can approach the default buffer size, causing the next frame to stall on `sendmsg()`. The larger buffer prevents this entirely.

### 2. NVENC video quality presets

SolarFlare exposes 10 NVENC encoder settings in the web interface and adds three one-click presets. Instead of tuning each knob individually, pick the preset that matches your game:

| Preset | NVENC quality level | B-frames | Lookahead | Other settings | Best for |
|---|---|---|---|---|---|
| Latency-optimized | P1 | 0 | 0 frames | Zero-latency tune, AQ off | CS2, VALORANT, fighting games |
| Balanced | P4 | 2 | 20 frames | Spatial + temporal AQ, weighted prediction | Most single-player and casual multiplayer |
| Quality-optimized | P7 | 4 | 40 frames | Full two-pass, per-codec min-QP clamping | Cinematic games where visuals matter most |
| Manual | As configured | As configured | As configured | Nothing is overridden | When you want full control |

The individual knobs you can tweak in manual mode:

- `nvenc_bframes` (0 to 4): B-frames between P-frames. More B-frames = better compression, more latency.
- `nvenc_zerolatency` (true/false): Forces zero reorder delay and disables B-frames and lookahead.
- `nvenc_rc_lookahead` (0 to 31): Number of frames the encoder looks ahead for rate control decisions.
- `nvenc_aq_strength` (1 to 15): How aggressively the encoder redistributes bits within a frame.
- `nvenc_temporal_aq` (true/false): Extends adaptive quantization across frames instead of within a single frame.
- `nvenc_weighted_prediction` (true/false): Improves fade transitions at a small CUDA cost.
- `nvenc_enable_min_qp` (true/false): Clamps the minimum quantization parameter to save bitrate on easy scenes.
- `nvenc_min_qp_h264` / `nvenc_min_qp_hevc` / `nvenc_min_qp_av1`: Per-codec minimum QP values.
- `nvenc_filler_data` (true/false): Adds filler to hit the target bitrate. Testing use only.
- `nvenc_surfaces` (-1 to 32): Number of encode surfaces. -1 lets the driver decide.

All of these are in the web interface under the NVENC tab and in `sunshine.conf`.

### 3. Audio processing pipeline

SolarFlare can clean up game audio before it is encoded and sent to your client. All audio processing is **off by default**, so a fresh install sounds identical to regular Sunshine.

**Automatic Gain Control (AGC)**

Smoothly adjusts volume so that loud explosions and quiet dialogue sit at a consistent level. Configurable target level, maximum and minimum gain, and attack, hold, and release timing.

**Voice Activity Detection (VAD)**

Detects when someone is speaking versus when the audio is just game noise. Used internally by the ducker. Configurable threshold and hysteresis to prevent rapid on-off flapping.

**Ducker**

When speech is detected, automatically lowers the game audio volume so voices remain clear. Configurable attenuation depth, attack speed, and release speed.

**Noise Gate**

Mutes the signal when it drops below a configurable threshold. Useful for cutting out background hum, fan noise, or keyboard clatter.

**Opus encoder tuning**

Six additional settings control how Opus compresses the audio stream:

- Application mode: VOIP (lowest latency), AUDIO (highest quality), or LOWDELAY
- Variable vs constant bitrate
- Encoder complexity (0 to 10, higher is better quality at a CPU cost)
- In-band forward error correction (FEC) for packet loss recovery
- Expected packet loss hint (tells Opus how aggressive to be with FEC)
- Bandwidth extension up to 24 kHz (super-wideband)

### 4. Per-game encoder profiles

Different games benefit from different encoder settings. CS2 needs the lowest possible latency. Cyberpunk benefits from higher quality. SolarFlare lets you set a per-game encoder preset so it switches automatically.

Add an `"encoder-preset"` field to any app in your `apps.json` file:

```json
{
  "name": "Counter-Strike 2",
  "cmd": "steam steam://rungameid/730",
  "encoder-preset": 0
}
```

When you launch that app through Moonlight, SolarFlare saves your global preset, applies the per-game one, and restores the global preset when the session ends. The value `0` means latency-optimized, `1` means balanced, and `2` means quality-optimized. Leave the field absent or set it to `-1` to use the global default.

### 5. DSCP network priority

SolarFlare marks its streaming packets with a DSCP tag (Class Selector 3). Routers that support Quality of Service will see this tag and prioritize the game stream over bulk traffic like Steam downloads, file transfers, or video streaming. The result is smoother gameplay when your network is busy.

This is a single `setsockopt` call that adds zero overhead. Controlled by the `dscp_qos` setting (default: true). Disable it if your router does not support QoS or if you experience issues.

### 6. GPU frequency governor

On AMD GPUs, SolarFlare writes `performance` to `/sys/class/drm/card*/device/power_dpm_force_performance_level` when streaming starts, and `auto` when it stops. This prevents the GPU from clocking down between frames, which can add a small but noticeable latency spike at high refresh rates.

Controlled by `gpu_governor` (default: true). Silently does nothing on NVIDIA or Intel GPUs, or on systems that do not expose this sysfs path.

### 7. Headless X11 virtual display (legacy)

If you run your gaming PC without a monitor attached and need a simple X11 stub, the legacy `headless_virtual_display` option runs `xrandr --output VIRTUAL1 --auto` and re-scans for displays when no physical output is detected. Requires `xserver-xorg-video-dummy` installed on most distributions.

For new headless setups, see **section 13. Headless stream with smart backend selection** below — it does not need X11 dummy drivers and works on pure Wayland.

This is **off by default**. Enable with `headless_virtual_display = true` in `sunshine.conf`.

### 8. Zen CPU auto-detection

During the build, SolarFlare reads your CPU model from `/proc/cpuinfo` and applies the correct compiler target:

- Zen 1: `-march=znver1`
- Zen 2: `-march=znver2`
- Zen 3: `-march=znver3`
- Zen 4: `-march=znver4`
- Unknown AMD or fallback: `-march=x86-64-v3`

Combined with `-flto` (link-time optimization), `-O3` (aggressive optimization), and `-fno-plt` (faster function calls), the compiled binary uses AVX2, BMI2, and FMA instructions natively. You can override the auto-detection with `./scripts/cachyos-build.sh --march znver4`.

### 9. Other improvements

**Fork identity:** Running `sunshine --version` prints the fork name, repository URL, and commit hash so you can confirm you are running SolarFlare.

**Self-contained CI:** The fork uses its own release workflow (`release.yml`) that builds and packages Linux binaries without depending on LizardByte's release infrastructure.

**Upstream sync:** 24 commits from upstream LizardByte/Sunshine have been cherry-picked since the fork was created in June 2026. These include build fixes, PipeWire improvements, OpenSSL 4.x compatibility, and KMS DRM fixes.

**Regression guards:** Cherry-picked upstream fixes are guarded by regression tests that verify the fix is still in place.

**Pinned workflows:** 22 LizardByte workflow files are pinned to specific commits so they never accidentally run on the fork.

### 10. Command palette (Ctrl+K)

Press `Ctrl+K` (or `Cmd+K`) from anywhere in the web UI to open a Spotlight-style command palette. Type to search across pages, settings shortcuts, and host controls. Use arrow keys to navigate, Enter to select, Esc to close.

### 11. Trusted subnet auto-pairing

Add `trusted_subnets = "10.0.0.0/24,192.168.1.0/24"` to your config and any Moonlight client connecting from those subnets is paired automatically without needing to type a PIN. IPv4 and IPv6 CIDR ranges are both supported. Enable with `trusted_subnet_auto_pairing = enabled`.

### 12. Adaptive bitrate controller

An EWMA-based controller in the encode loop watches encode time, FPS ratio, and client-reported packet loss. When network conditions degrade or the encoder can't keep up, bitrate is reduced proportionally within `[adaptive_bitrate_min, adaptive_bitrate_max]` (defaults 2 Mbps / 100 Mbps). After 10 seconds of healthy stats, bitrate ramps back up gradually.

### 13. Headless stream with smart backend selection

Games can run in a private compositor instead of hijacking your desktop. Three backends are supported and auto-detected based on the running compositor:

- **`compositor_backend = krfb`** — Creates a virtual KWin output on your existing KDE session via `krfb-virtualmonitor`. No nested compositor, no X11.
- **`compositor_backend = gamescope`** — For Steam Deck game mode. Spawns a nested Gamescope instance with `--headless`.
- **`compositor_backend = labwc`** — For everything else. Spawns a wlroots headless compositor.

When `compositor_backend = auto` (default), SolarFlare detects KDE and prefers krfb, Steam Deck and prefers gamescope, then falls back to labwc. All three are pure Wayland with zero X11 dependencies. Enable with:

```
headless_mode = enabled
linux_use_cage_compositor = enabled
compositor_backend = auto
```

### 14. Game library import scanner

`GET /api/games/scan` discovers installed games from Steam (`libraryfolders.vdf` + `*.acf`), Lutris (`*.yml`), and Heroic (`installed.json`). Returns `{name, path, launcher, cover_url}` for every game found, ready for one-click import into your apps list.

### 15. KWin screencast privilege-drop retry

When Sunshine runs with `CAP_SYS_ADMIN`, KWin sometimes refuses the screencast Wayland connection. The `kwingrab` backend now detects this and automatically drops all elevated privileges, re-creates the screencast session, and retries. No more silent failures when running as root or with file capabilities set.

### 16. Scoped API tokens

Scripts and CI bots that talk to the Sunshine API historically needed the admin password — full power over config, apps, pairing, and display reset. Scoped API tokens let you mint a bearer token with only the permissions a script actually needs.

You create tokens from the web UI Config tab or from the CLI:

```bash
curl -k -u admin:password -X POST https://localhost:47990/api/tokens \
  -H "Content-Type: application/json" \
  -d '{"name":"readonly-ci","scopes":["config:get","logs:get"]}'
```

The server returns the plaintext token exactly once. It is stored hashed (SHA-256 of plaintext + per-token salt) so the plaintext is never on disk. Callers use it as a Bearer token:

```bash
curl -k -H "Authorization: Bearer <token>" https://localhost:47990/api/config
```

Available scopes: `config:get`, `config:set`, `apps:get`, `apps:launch`, `apps:close`, `clients:list`, `clients:pair`, `clients:unpair`, `logs:get`, `display:reset`, `tokens:manage`. The wildcard `*` matches everything (equivalent to admin, without being admin).

### 17. Adaptive bitrate HTTP endpoint

The EWMA-based adaptive bitrate controller (section 12) already adjusts bitrate based on internal encode stats. The new HTTP endpoint lets clients push their own network stats into the same controller, so a Moonlight client or an external monitor can fine-tune bitrate for current network conditions:

```bash
curl -k -H "Authorization: Bearer <token>" \
  -X POST https://localhost:47990/api/stream/network-stats \
  -H "Content-Type: application/json" \
  -d '{"packet_loss_pct":0.5,"rtt_ms":23.4}'
```

Read the current bitrate state with:

```bash
curl -k -H "Authorization: Bearer <token>" \
  https://localhost:47990/api/stream/bitrate
```

### 18. Packaged redesign services

SolarFlare ships three systemd services that tune the host for low-latency streaming. Install and manage them with a single script:

```bash
sudo /usr/share/sunshine/redesign/install-redesign-services.sh     # install
sudo /usr/share/sunshine/redesign/install-redesign-services.sh --uninstall  # remove
```

The three services run once at boot and exit cleanly:

- **cpu-performance** — Forces the CPU governor to `performance` on every core with a writable scaling-governor file. Skips offline CPUs and read-only sysfs nodes.
- **nic-tuning** — Configures the Ethernet NIC for low latency. Probes the actual driver name (works with r8169, e1000e, igc, etc.) and only writes to knobs the driver supports.
- **nvidia-clock-lock** — Reads the GPU's maximum boost clock and locks the NVENC clock to that value, preventing frequency-down on idle periods.

### 19. Hermes-KMS virtual display backend

Hermes-KMS (`github.com/MrOz59/Hermes-KMS`) is a Linux kernel module that exposes a DRM/KMS virtual output with DMA-BUF frame capture. When loaded (`sudo modprobe hermes_kms`), the source selector shows "HERMES-1". SolarFlare probes the card at startup and reads capabilities through the UAPI — `dmabuf_export`, `frame_wait`, and `frame_acquire` are confirmed present on the host.

When selected, the backend opens the card node, runs `WAIT_FRAME` to block until the compositor posts a new frame, then `ACQUIRE_FRAME` to pull its DMA-BUF and push it to the encoder (VAAPI / NVENC / AMF) — no CPU readback. Set `capture = hermes_kms` in your config to pick this backend over KMS / X11 / Wayland / Portal / KWin.

The kernel module source lives at `third-party/hermes-kms/` (vendored from `github.com/MrOz59/Hermes-KMS`, GPL-2.0+). `scripts/cachyos-build.sh` runs `packaging/linux/redesign/install-hermes-kms.sh` after `cmake --install`, which DKMS-installs the module and loads it with `initial_enabled=1`. To install manually: `sudo packaging/linux/redesign/install-hermes-kms.sh`. To uninstall: `sudo ... --uninstall`. Requires `dkms` and your distro's kernel-headers package (CachyOS: `linux-zen-headers`; Debian/Ubuntu: `linux-headers-$(uname -r)`; Fedora: `kernel-devel-$(uname -r)`).

---

## All config settings

All SolarFlare-specific settings live in `~/.config/sunshine/sunshine.conf` and can be edited through the web interface at `https://localhost:47990`. Every setting ships with a sensible default — you do not need to change anything to get the speed benefits.

### Network and latency tunables

| Setting | Type | Default | Range | Description |
|---|---|---|---|---|
| `rate_cap_pct` | int | 80 | 50 to 95 | Percentage of your network link speed to use for streaming |
| `busy_poll_us` | int | 50 | 0 to 10000 | Microseconds to busy-poll the network socket (0 disables) |
| `pipewire_latency_ms` | int | 8 | 1 to 40 | Audio buffer size in milliseconds |
| `cpu_pinning` | bool | true | — | Pin streaming threads to dedicated CPU cores |
| `enet_4mib_buffer` | bool | true | — | Use 4 MB UDP socket buffers instead of the kernel default |
| `dscp_qos` | bool | true | — | Tag streaming packets so routers prioritize them over bulk traffic |
| `gpu_governor` | bool | true | — | Force AMD GPU into performance mode while streaming |
| `headless_virtual_display` | bool | false | — | Create a virtual display if no physical monitor is detected |
| `api_tokens` | string | — | — | Managed via /api/tokens web endpoint — not intended for hand-editing |

### Audio processing

All audio processing is off by default. Enable each feature individually:

| Setting | Type | Default | Range | Description |
|---|---|---|---|---|
| `sf_audio_agc` | bool | false | — | Enable automatic gain control |
| `sf_audio_agc_target_db` | float | -20 | -40 to -6 | AGC target loudness in dBFS |
| `sf_audio_agc_max_gain_db` | float | 12 | 0 to 30 | Maximum gain AGC can apply |
| `sf_audio_agc_min_gain_db` | float | -12 | -30 to 0 | Maximum attenuation AGC can apply |
| `sf_audio_agc_attack_ms` | float | 10 | 1 to 500 | How quickly AGC raises gain |
| `sf_audio_agc_hold_ms` | float | 200 | 0 to 5000 | How long AGC holds peak before releasing |
| `sf_audio_agc_release_ms` | float | 100 | 1 to 5000 | How quickly AGC releases gain |
| `sf_audio_vad` | bool | false | — | Enable voice activity detection |
| `sf_audio_vad_threshold_db` | float | -45 | -80 to -10 | Loudness threshold for speech detection |
| `sf_audio_vad_hysteresis_db` | float | 6 | 0 to 30 | Guard band to prevent rapid on-off switching |
| `sf_audio_vad_min_speech_ms` | float | 100 | 10 to 2000 | Minimum duration of a speech burst |
| `sf_audio_vad_min_silence_ms` | float | 200 | 10 to 5000 | Minimum silence before speech is considered ended |
| `sf_audio_ducking` | bool | false | — | Enable ducker (lowers game audio when speech is active) |
| `sf_audio_ducker_attenuation_db` | float | -12 | -40 to 0 | How much to reduce game volume during speech |
| `sf_audio_ducker_attack_ms` | float | 50 | 1 to 2000 | How quickly the ducker engages |
| `sf_audio_ducker_release_ms` | float | 500 | 1 to 5000 | How quickly the ducker releases |
| `sf_audio_noise_gate` | bool | false | — | Enable noise gate |
| `sf_audio_noise_gate_db` | float | -55 | -90 to -10 | Signal level below which audio is muted |

### Opus encoder

| Setting | Type | Default | Range | Description |
|---|---|---|---|---|
| `sf_opus_application` | int | 0 | 0 to 2 | Encoder mode: 0 = VOIP, 1 = AUDIO, 2 = LOWDELAY |
| `sf_opus_vbr` | int | 0 | 0 to 2 | Bitrate mode: 0 = CBR, 1 = constrained VBR, 2 = full VBR |
| `sf_opus_complexity` | int | 10 | 0 to 10 | Encoder complexity (higher = better quality, more CPU) |
| `sf_opus_fec` | bool | true | — | Enable in-band forward error correction |
| `sf_opus_expected_loss_pct` | int | 0 | 0 to 100 | Hint for expected packet loss percentage |
| `sf_opus_bandwidth_extension` | bool | true | — | Allow super-wideband audio up to 24 kHz |

### NVENC encoder

| Setting | Type | Default | Range | Description |
|---|---|---|---|---|
| `nvenc_tuning_preset` | int | -1 | -1 to 2 | One-click preset: -1 = manual, 0 = latency, 1 = balanced, 2 = quality |
| `nvenc_bframes` | int | 0 | 0 to 4 | Number of B-frames between P-frames |
| `nvenc_zerolatency` | bool | false | — | Force zero reorder delay and disable B-frames |
| `nvenc_rc_lookahead` | int | 0 | 0 to 31 | Rate-control lookahead frames |
| `nvenc_aq_strength` | int | 8 | 1 to 15 | Adaptive quantization strength |
| `nvenc_temporal_aq` | bool | false | — | Enable temporal adaptive quantization |
| `nvenc_weighted_prediction` | bool | false | — | Enable B-frame weighted prediction |
| `nvenc_enable_min_qp` | bool | false | — | Enable minimum QP clamping |
| `nvenc_min_qp_h264` | int | 19 | 1 to 51 | Minimum QP for H.264 |
| `nvenc_min_qp_hevc` | int | 23 | 1 to 51 | Minimum QP for HEVC |
| `nvenc_min_qp_av1` | int | 23 | 1 to 255 | Minimum QP for AV1 |
| `nvenc_filler_data` | bool | false | — | Add filler data to hit target bitrate |
| `nvenc_surfaces` | int | -1 | -1 to 32 | Number of encode surfaces (-1 = driver default) |

### Per-game profiles

Not a `sunshine.conf` setting — add to individual apps in `~/.config/sunshine/apps.json`:

```json
{
  "name": "Game Name",
  "cmd": "command to launch",
  "encoder-preset": 1
}
```

Values: `-1` = use global preset (default), `0` = latency, `1` = balanced, `2` = quality.

---

## Building from source

If you prefer to compile from source rather than use the install script, here is how.

### Install dependencies

**Arch / CachyOS:**

```
sudo pacman -S base-devel cmake boost libcurl opus libx11 libxrandr libxfixes libxcb avahi libdrm libevdev wayland wayland-protocols pulseaudio pipewire
```

**Ubuntu / Debian:**

```
sudo apt install build-essential cmake libboost-all-dev libcurl4-openssl-dev libopus-dev libx11-dev libxrandr-dev libxfixes-dev libxcb1-dev libavahi-client-dev libdrm-dev libevdev-dev libwayland-dev libpulse-dev libpipewire-0.3-dev
```

**Fedora:**

```
sudo dnf install gcc-c++ cmake boost-devel libcurl-devel opus-devel libX11-devel libXrandr-devel libXfixes-devel libxcb-devel avahi-devel libdrm-devel libevdev-devel wayland-devel pulseaudio-libs-devel pipewire-devel
```

### Initialize submodules

```
git submodule update --init --recursive
```

### Build

```bash
# Regular build (no CUDA, no docs — faster compile)
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -DSUNSHINE_ENABLE_CUDA=OFF
cmake --build cmake-build-release -j$(nproc)
sudo cmake --install cmake-build-release
```

```bash
# Build with tests
cmake -B cmake-build-test -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DSUNSHINE_ENABLE_CUDA=OFF
cmake --build cmake-build-test -j$(nproc)
./cmake-build-test/tests/test_sunshine
```

SolarFlare auto-detects your Zen CPU generation and sets the correct `-march` flag. Combined with `-flto`, `-O3`, and `-fno-plt`, the binary is compiled specifically for your hardware.

---

## Testing

412 automated tests verify correctness. Current status:

- **406 tests pass**
- **5 tests skipped** — pre-existing issues inherited from upstream Sunshine (Inputtino and Evdev input TODOs). Not SolarFlare bugs.
- **1 test skipped** — a config documentation consistency check. SolarFlare's expert-level kernel tunables are intentionally not exposed in the web UI, which this test flags as a documentation gap.

42 SolarFlare-specific tests cover config key defaults, NVENC preset application, audio processing behavior, and cherry-pick regression guards.

To run tests:

```bash
cmake --build cmake-build-test -j$(nproc)
./cmake-build-test/tests/test_sunshine --gtest_brief=1
```

---

## FAQ

**Why a fork instead of contributing to Sunshine?**

Sunshine supports every platform: Windows, macOS, Linux, FreeBSD, Intel, AMD, ARM. SolarFlare only supports AMD Ryzen on Linux. Many of these optimizations use Linux kernel features (`SO_BUSY_POLL`, `SCHED_RR`, sysfs, PipeWire hints, DSCP tagging) that do not exist on other platforms. A project that needs to compile and run everywhere cannot take these shortcuts.

**Will this break my existing Moonlight pairing?**

No. SolarFlare uses the same ports, protocol, encryption, and config directory as regular Sunshine. You can switch between the two without re-pairing any devices. Your settings, credentials, and app list stay in `~/.config/sunshine/`.

**What if I upgrade and the new settings are not in my config file?**

Nothing bad happens. Every SolarFlare setting has a sensible default. If a setting key is missing from your config file, it behaves as if it were set to the documented default. You do not need to migrate anything.

**How do I go back to regular Sunshine?**

On Arch: `sudo pacman -Rns sunshine && sudo pacman -S sunshine`

If installed from source: `sudo cmake --install build --uninstall`, then install Sunshine through your package manager.

Your `~/.config/sunshine/` folder stays intact and is compatible with both.

**Does this work on Windows?**

No. SolarFlare is Linux-only. Use regular Sunshine on Windows.

**Does this work on Intel or ARM?**

No. SolarFlare compiles with `-march=znverN` which targets AMD Zen microarchitecture features (AVX2, BMI2, FMA). Use regular Sunshine for Intel or ARM systems.

---

## Credits and license

SolarFlare is built on top of [LizardByte's Sunshine](https://github.com/LizardByte/Sunshine), which was itself based on the original Sunshine by Nathan Castle. The web interface, Moonlight protocol implementation, and cross-platform plumbing are all LizardByte's work. This fork would not exist without them.

**SolarFlare additions:** Ryzen-specific compiler auto-detection, eight Linux speed-tuning settings, NVENC video quality presets, audio processing pipeline with AGC, VAD, ducker, and noise gate, Opus encoder tuning, per-game encoder profiles, DSCP network priority tagging, GPU frequency governor, headless virtual display support, self-contained CI workflow, 23 upstream cherry-picks, and 42 fork-specific tests.

**License:** GPL-3.0, inherited from upstream Sunshine. See [LICENSE](LICENSE) for the full text and [NOTICE](NOTICE) for attribution details.

---

<p align="center"><strong>SolarFlare — Less lag, more game.</strong></p>
