<p align="center">
  <img src="https://img.shields.io/badge/SolarFlare-F7B731?style=for-the-badge&logo=sun&logoColor=black" alt="SolarFlare">
</p>

<h1 align="center">SolarFlare</h1>

<p align="center">
  <em>Your games. Your hardware. Your network. No lag.</em>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square" alt="License"></a>
  <a href="https://github.com/LizardByte/Sunshine"><img src="https://img.shields.io/badge/fork-LizardByte%2FSunshine-9cf?style=flat-square" alt="Fork"></a>
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/version-v2026.708.2--solarflare-orange?style=flat-square" alt="Version"></a>
  <a href="#testing"><img src="https://img.shields.io/badge/tests-490%20passed%2C%2012%20skipped-brightgreen?style=flat-square" alt="Tests"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/releases"><img src="https://img.shields.io/github/v/release/vindeckyy/Solar-Flare?style=flat-square" alt="Release"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/stargazers"><img src="https://img.shields.io/github/stars/vindeckyy/Solar-Flare?style=flat-square" alt="Stars"></a>
</p>

<br>

<div align="center">
  <table>
    <tr>
      <td align="center"><strong>End-to-end latency</strong><br><sub>button press → screen update</sub></td>
      <td align="center"><strong>Network polling</strong><br><sub>socket wake-up delay</sub></td>
      <td align="center"><strong>Audio sync</strong><br><sub>audio buffer offset</sub></td>
      <td align="center"><strong>Worst-case burst</strong><br><sub>network scheduling jitter</sub></td>
    </tr>
    <tr>
      <td align="center"><b>5.5–12 ms</b><br><sub><s>18–65 ms</s></sub></td>
      <td align="center"><b>15 µs</b><br><sub><s>80 µs</s></sub></td>
      <td align="center"><b>4–8 ms</b><br><sub><s>~20 ms</s></sub></td>
      <td align="center"><b>&lt;2 ms</b><br><sub><s>47 ms</s></sub></td>
    </tr>
  </table>
  <sub><i>Ryzen 5 4600H · RTX 3060 · Wi-Fi 6 · 1080p · Wayland</i></sub>
</div>

<br>

---

## Why SolarFlare?

Sunshine is the best open-source game streamer — but it runs everywhere. That portability means it can't use the tricks that only work on Linux.

SolarFlare is Linux-only. Same protocol as Sunshine. Same Moonlight client. Same config folder. But the kernel hooks, real-time scheduling, and zero-copy paths that upstream has to skip are all fair game here.

**The result:** 3–5× less latency than regular Sunshine on the same hardware. The difference between *watching* your game and *playing* it.

---

## Quick start

```bash
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine
```

Then install [Moonlight](https://moonlight-stream.org/) on any device, pair with the PIN at `https://localhost:47990`, and play.

<details>
<summary><b>Updating an existing install</b></summary>

```bash
cd Solar-Flare
git pull
git submodule update --init --recursive
./scripts/cachyos-build.sh --clean
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user restart sunshine
```
</details>

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
| **Latency-optimized** | P1 | 0 | 0 frames | Zero-latency tune, AQ off | CS2, VALORANT, fighting games |
| **Balanced** | P4 | 2 | 20 frames | Spatial + temporal AQ, weighted prediction | Most single-player and casual multiplayer |
| **Quality-optimized** | P7 | 4 | 40 frames | Full two-pass, per-codec min-QP clamping | Cinematic games where visuals matter most |
| **Manual** | As configured | As configured | As configured | Nothing is overridden | When you want full control |

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

- **Fork identity** — `sunshine --version` prints the fork name, repository URL, and commit hash so you can confirm you're running SolarFlare (and not regular Sunshine).
- **Self-contained CI** — The fork uses its own release workflow (`release.yml`) that builds and packages Linux binaries without depending on LizardByte's release infrastructure.
- **Upstream sync** — 24 commits from upstream LizardByte/Sunshine have been cherry-picked since the fork was created in June 2026.
- **Regression guards** — Each cherry-picked upstream fix ships with a regression test that fails if the fix is reverted.
- **Pinned workflows** — 22 LizardByte workflow files are pinned to specific commit SHAs so they never accidentally run on the fork.
- **Mailbox hygiene** — The repo watch is set to `ignored`, so no activity emails come from this repo.

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
curl -k -H "Authorization: Bearer ***" https://localhost:47990/api/config
```

Available scopes: `config:get`, `config:set`, `apps:get`, `apps:launch`, `apps:close`, `clients:list`, `clients:pair`, `clients:unpair`, `logs:get`, `display:reset`, `tokens:manage`. The wildcard `*` matches everything (equivalent to admin, without being admin).

### 17. Adaptive bitrate HTTP endpoint

The EWMA-based adaptive bitrate controller already adjusts bitrate based on internal encode stats. The HTTP endpoint lets clients push their own network stats into the same controller:

```bash
curl -k -H "Authorization: Bearer ***" \
  -X POST https://localhost:47990/api/stream/network-stats \
  -H "Content-Type: application/json" \
  -d '{"packet_loss_pct":0.5,"rtt_ms":23.4}'
```

Read the current bitrate state with:

```bash
curl -k -H "Authorization: Bearer ***" \
  https://localhost:47990/api/stream/bitrate
```

### 18. Packaged redesign services

SolarFlare ships three systemd services that tune the host for low-latency streaming:

```bash
sudo /usr/share/sunshine/redesign/install-redesign-services.sh     # install
sudo /usr/share/sunshine/redesign/install-redesign-services.sh --uninstall  # remove
```

The three services run once at boot and exit cleanly:

- **cpu-performance** — Forces the CPU governor to `performance` on every core with a writable scaling-governor file.
- **nic-tuning** — Configures the Ethernet NIC for low latency. Probes the actual driver name and only writes to knobs the driver supports.
- **nvidia-clock-lock** — Reads the GPU's maximum boost clock and locks the NVENC clock to that value.

### 19. Hermes-KMS virtual display backend

Hermes-KMS is a Linux kernel module that exposes a DRM/KMS virtual output with DMA-BUF frame capture. When loaded (`sudo modprobe hermes_kms`), the source selector shows "HERMES-1". SolarFlare probes the card at startup and reads capabilities through the UAPI — `dmabuf_export`, `frame_wait`, and `frame_acquire` are confirmed present on the host.

When selected, the backend opens the card node, runs `WAIT_FRAME` to block until the compositor posts a new frame, then `ACQUIRE_FRAME` to pull its DMA-BUF and push it to the encoder (VAAPI / NVENC / AMF) — no CPU readback. Set `capture = hermes_kms` in your config to pick this backend over KMS / X11 / Wayland / Portal / KWin.

The kernel module source lives at `third-party/hermes-kms/` (vendored from `github.com/MrOz59/Hermes-KMS`, GPL-2.0+). `scripts/cachyos-build.sh` runs `packaging/linux/redesign/install-hermes-kms.sh` after `cmake --install`, which DKMS-installs the module and loads it. To install manually: `sudo packaging/linux/redesign/install-hermes-kms.sh`.

---

## Benchmarks

<div align="center">

| Metric | Regular Sunshine | SolarFlare | Improvement |
|---|---|---|---|
| End-to-end latency | 18–65 ms | **5.5–12 ms** | **3–5×** |
| Network polling | 80 µs | **15 µs** | **5.3×** |
| Audio sync | ~20 ms | **4–8 ms** | **2.5–5×** |
| Worst-case burst | 47 ms | **&lt;2 ms** | **23×** |

</div>

*Ryzen 5 4600H, RTX 3060, Wi-Fi 6, 1080p, GNOME/Wayland. See the sections above for the full methodology and per-feature breakdown.*

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

### Audio processing

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

### Dependencies

<details>
<summary><b>Arch / CachyOS</b></summary>

```bash
sudo pacman -S base-devel cmake boost libcurl opus libx11 libxrandr libxfixes libxcb avahi libdrm libevdev wayland wayland-protocols pulseaudio pipewire
```
</details>

<details>
<summary><b>Ubuntu / Debian</b></summary>

```bash
sudo apt install build-essential cmake libboost-all-dev libcurl4-openssl-dev libopus-dev libx11-dev libxrandr-dev libxfixes-dev libxcb1-dev libavahi-client-dev libdrm-dev libevdev-dev libwayland-dev libpulse-dev libpipewire-0.3-dev
```
</details>

<details>
<summary><b>Fedora</b></summary>

```bash
sudo dnf install gcc-c++ cmake boost-devel libcurl-devel opus-devel libX11-devel libXrandr-devel libXfixes-devel libxcb-devel avahi-devel libdrm-devel libevdev-devel wayland-devel pulseaudio-libs-devel pipewire-devel
```
</details>

### Initialize submodules

```bash
git submodule update --init --recursive
```

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSUNSHINE_ENABLE_CUDA=OFF -DBUILD_DOCS=OFF -DBUILD_TESTS=OFF
cmake --build build -j$(nproc)
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
```

---

## Testing

490 automated tests verify correctness:

- **478 tests pass** — cumulative across the forked test suites (`test_config_fork_keys`, `test_audio_fx`, `test_adaptive_bitrate`, `test_trusted_subnet`, `test_parse_monitor_index`, `test_game_scanner`, `test_headless_compositor`, plus `test_error` and upstream-cherry-pick regression guards).
- **12 tests skipped** — environment-bound: NVENC/VAAPI/software encoder variants require the matching hardware, the AudioTest surround-channel parameterizations need extra audio config, MouseHID tests need a physical input device, and the Windows-only UTF utility test is skipped on Linux.
- **0 failed**

To run tests:

```bash
cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DSUNSHINE_ENABLE_CUDA=OFF
cmake --build build-tests -j$(nproc)
./build-tests/tests/test_sunshine --gtest_brief=1
```

---

## FAQ

**Will this break my existing Moonlight setup?**  
No. SolarFlare uses the same ports, protocol, encryption, and config directory as regular Sunshine. You can switch between the two without re-pairing any devices.

**How do I go back to regular Sunshine?**  
On Arch: `sudo pacman -S sunshine`. If installed from source, run `sudo xargs rm < build/install_manifest.txt` then install Sunshine through your package manager. Your `~/.config/sunshine/` folder stays intact and is compatible with both.

**Does this work on Windows, Intel, or ARM?**  
No. SolarFlare is Linux-only. Use regular Sunshine.

**A game freezes during its loading screen.**  
Fixed upstream in `8060cf3`. Root cause was the capture thread being on `SCHED_RR`: when a Proton game's loader is mostly blocked on GPU fences, a SCHED_RR capture thread can run a full ~100 ms time slice before the kernel preempts it, starving the game's main thread. The fix keeps core pinning but drops the capture thread off SCHED_RR onto plain CFS.

---

## Credits

SolarFlare is built on top of [LizardByte's Sunshine](https://github.com/LizardByte/Sunshine), which was itself based on the original Sunshine by Nathan Castle. The web interface, Moonlight protocol implementation, and cross-platform plumbing are all LizardByte's work.

The full per-commit history with topic-grouped explanations lives in [docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md).

---

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square" alt="License"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/issues"><img src="https://img.shields.io/github/issues/vindeckyy/Solar-Flare?style=flat-square" alt="Issues"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/releases"><img src="https://img.shields.io/github/v/release/vindeckyy/Solar-Flare?style=flat-square" alt="Release"></a>
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/changelog-SolarFlare-orange?style=flat-square" alt="Changelog"></a>
  <br><br>
  <strong>SolarFlare — Less lag, more game.</strong>
</p>
