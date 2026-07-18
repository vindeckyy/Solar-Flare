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
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/changelog-SolarFlare-orange?style=flat-square" alt="Changelog"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/releases"><img src="https://img.shields.io/github/v/release/vindeckyy/Solar-Flare?style=flat-square" alt="Release"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/stargazers"><img src="https://img.shields.io/github/stars/vindeckyy/Solar-Flare?style=flat-square" alt="Stars"></a>
</p>

<br>

<div align="center">
  <table>
    <tr>
      <td align="center"><strong>End-to-end latency</strong><br><sub>button press to screen update</sub></td>
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

Moonlight + Sunshine works fine until your network gets busy or your CPU has something else to do. Regular Sunshine targets every platform (Windows, Mac, Linux, Intel, AMD, ARM), so it can't use the Linux-only tricks.

SolarFlare does one thing: low latency game streaming on Linux. Same Moonlight app, same web interface, same config files. It just talks to your hardware directly in ways upstream Sunshine can't. The result is 3 to 5 times less delay on the same PC.

What that means: with regular Sunshine there's a noticeable gap between pressing a key and seeing the result. Like watching a YouTube video of your game. With SolarFlare, that gap shrinks to the point where it feels like you're sitting at the computer.

---

## SolarFlare Web UI

The host observatory keeps pairing, application mapping, configuration, and
diagnostics in one responsive control surface. It uses a persistent desktop
rail, a compact mobile bar, keyboard command search, and interaction-driven
feedback without ambient looping animation.

<table>
  <tr>
    <td width="50%"><strong>Host overview</strong><br><img src="docs/images/web-ui-home.png" alt="SolarFlare host overview"></td>
    <td width="50%"><strong>Pair a device</strong><br><img src="docs/images/web-ui-pin.png" alt="SolarFlare PIN pairing"></td>
  </tr>
  <tr>
    <td width="50%"><strong>Applications</strong><br><img src="docs/images/web-ui-applications.png" alt="SolarFlare application catalog"></td>
    <td width="50%"><strong>Featured apps</strong><br><img src="docs/images/web-ui-featured.png" alt="SolarFlare featured applications"></td>
  </tr>
  <tr>
    <td width="50%"><strong>Configuration</strong><br><img src="docs/images/web-ui-configuration.png" alt="SolarFlare configuration console"></td>
    <td width="50%"><strong>Troubleshooting</strong><br><img src="docs/images/web-ui-troubleshooting.png" alt="SolarFlare troubleshooting console"></td>
  </tr>
</table>

---

## Quick start

Build from source or grab the prebuilt binary:

```bash
# Option A: build from source
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh
sudo cmake --install build
sudo setcap 'cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine
```

```bash
# Option B: prebuilt binary from a release
sudo apt install -y libopus0 libva2 libdrm2 libevdev2 libgbm1 libvulkan1 libwayland-client0 libpulse0 libcurl4 libnotify4 libcap2-bin   # runtime deps on Debian/Ubuntu
sudo curl -L -o /usr/local/bin/sunshine https://github.com/vindeckyy/Solar-Flare/releases/latest/download/sunshine-x86_64
sudo chmod +x /usr/local/bin/sunshine
sudo setcap 'cap_sys_admin,cap_net_bind_service,cap_net_raw,cap_dac_read_search+ep' /usr/local/bin/sunshine
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
sudo setcap 'cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user restart sunshine
```
</details>

---

## Every feature explained

### 1. Smarter network speed detection

Regular Sunshine assumes every connection is 1 Gbps. If your PC has 2.5 GbE or fast Wi-Fi, Sunshine treats it the same and paces your stream way below what the network can handle.

SolarFlare reads your actual network card speed and uses that number. On a fast Wi-Fi 6 connection, this cuts average delay by 45 ms and worst-case spikes by 98 ms.

*Config: `rate_cap_pct` (default: 80, range: 50–95)*

### 2. Instant network responsiveness

Normally, the network card sends an interrupt, the CPU stops what it's doing, handles data, and goes back to work. That interrupt takes about 1 ms. Fine for most things, but at 60 fps every frame needs multiple network exchanges.

SolarFlare checks for new network data every 50 microseconds instead of waiting for interrupts. Like a cashier checking if the next customer is ready rather than waiting for a bell. The delay drops from 1 ms to about 50 µs.

*Config: `busy_poll_us` (default: 50, range: 0–10000)*

### 3. Tighter audio sync

PipeWire keeps a buffer of sound data before sending it to the encoder. Usually 20 to 40 ms worth. That means what you hear is always slightly behind what you see.

SolarFlare tells the audio system to use an 8 ms buffer instead of 20–40. This cuts audio delay by 1 to 2 frames. No crackling or dropouts on normal hardware.

Gunshots, footsteps, and dialogue arrive at the same time as the video showing them.

*Config: `pipewire_latency_ms` (default: 8, range: 1–40)*

### 4. Dedicated CPU cores for streaming

When you're gaming and streaming at the same time, the CPU splits attention between the game, capture, encoding, network, and audio. If the game thread and encoder thread land on the same core, they fight for time. Both stutter.

SolarFlare gives the encoder, capture, and audio threads their own dedicated CPU cores that nothing else can use. It also puts those threads on a priority system so the kernel runs them immediately. It avoids core 0 (which handles most system interrupts) and avoids sharing a physical core with hyperthreading.

On a 6-core CPU or better, you won't notice the two reserved cores. Plenty left for the game, and the stream runs smooth.

*Config: `cpu_pinning` (default: on)*

### 5. Bigger network buffers for 4K

At 4K, each encoded frame can be 200 KB. The default Linux network buffer is also about 200 KB. One frame fills the entire buffer, and the next frame waits until it drains. Visible hitches.

SolarFlare grows those buffers to 4 MB. Twenty times the default. The encoder can queue several frames without ever blocking.

*Config: `enet_4mib_buffer` (default: on)*

### 6. Smarter video encoding (NVENC presets)

NVENC is the dedicated video encoder chip on NVIDIA GPUs. It has about 10 settings that all interact, and nobody wants to spend hours tuning them.

SolarFlare replaces those 10 knobs with 3 one-click presets:

- **Latency mode** for competitive games (CS2, VALORANT, fighting games). Fastest encoding, zero extra frames of delay.
- **Balanced mode** (default). Good quality, low latency. Covers most single player games.
- **Quality mode** for slow, gorgeous games (Cyberpunk, Red Dead 2). The encoder takes more time per frame to produce the best possible image.

Every individual NVENC setting is still available in the web interface and config file if you want full control.

### 7. Cleaner, clearer game audio

SolarFlare can process game audio before sending it to the client. Everything here is off by default. If you don't touch these settings, audio sounds exactly like regular Sunshine.

- **Auto volume** (AGC). Keeps loud explosions and quiet dialogue at a similar level.
- **Voice detection** (VAD). Detects when someone is speaking versus game noise.
- **Auto-ducking**. When someone speaks, game volume automatically lowers. Like a radio DJ turning down the music.
- **Background noise removal**. Mutes audio below a threshold. Cuts fan hum, keyboard clatter, background noise.
- **Opus encoder tuning**. Fine tune how the audio compressor works: lowest delay (VOIP mode), highest quality (AUDIO mode), or somewhere in between. Control variable bitrate, error correction for spotty Wi-Fi, and bandwidth extension for crisp high frequencies.

### 8. Per-game encoder settings

Counter-Strike needs lowest possible latency. Cyberpunk needs best possible quality. Switching manually is annoying, and forgetting to switch back means one of your games looks or feels worse.

SolarFlare lets you set a per-game encoder preset in your apps config. Launch that game through Moonlight and it automatically switches to the right preset. Game ends, it switches back to your default.

### 9. Network priority tagging

When your network is busy (Netflix, Steam downloads, photo uploads), your game stream competes for bandwidth. Without priority, stream packets can get stuck behind a download.

SolarFlare marks its streaming packets with an "important" tag that QoS routers understand. The router moves game traffic ahead of bulk traffic. Single checkbox in the config, zero overhead.

*Config: `dscp_qos` (default: on)*

### 10. GPU speed boost during streaming

Your GPU runs slower when idle, then speeds up when needed. That ramp-up takes a few milliseconds. In a game stream, those milliseconds can land in the middle of encoding a frame. Visible hitch.

SolarFlare tells your GPU to run at full speed while streaming, then go back to normal afterward. Automatic on AMD. Same mechanism on NVIDIA.

*Config: `gpu_governor` (default: on)*

### 11. Streaming without a monitor

Sunshine normally can't capture video without a monitor. SolarFlare has two solutions:

**Legacy mode:** Creates a fake display using the dummy X11 driver. Works on older setups.

**Modern headless mode (recommended):** Auto detects your desktop environment and starts a private compositor for games:

- **KDE Plasma**: Creates a virtual monitor using KDE's built-in tools. No extra software.
- **Steam Deck**: Uses Gamescope's headless mode.
- **Everything else**: Starts a lightweight Wayland compositor.

Run a headless game server in a closet. No monitor, keyboard, or mouse needed. Stream from it like any other gaming PC.

### 12. Automatic quality adjustment

If your network gets congested (big download starts, Wi-Fi interference), your stream normally stutters or drops frames.

SolarFlare watches network conditions in real time. When it detects problems (packet loss, rising delay, encoder falling behind), it lowers video quality to keep things smooth. When the network recovers, quality goes back up.

A softer-looking stream beats freezing or skipping.

*Config: `adaptive_bitrate_enabled` (default: off)*

### 13. Auto-pairing for home devices

Normally, every new Moonlight client shows a PIN, you type it into the web interface, and they pair. Minor annoyance when setting up a new laptop or tablet.

SolarFlare lets you define your home network ranges (like "192.168.1.0/24" or "10.0.0.0/24"). Any Moonlight client connecting from those addresses pairs automatically. No PIN.

### 14. Quick command search (Ctrl+K)

The Sunshine web interface has a lot of pages. SolarFlare adds a search bar you can open from anywhere with Ctrl+K (Cmd+K on Mac). Start typing and it finds the page or setting. Like Spotlight on macOS or Ctrl+K in VS Code.

Type "4k" and the resolution picker opens. Type "bitrate" and jump to bitrate settings.

### 15. Find and import your games automatically

SolarFlare scans Steam, Lutris, and Heroic Games Launcher for everything you have installed. Returns game name, launch path, and (for Steam) cover art URL. Import with one click into your streaming apps list.

### 16. Safer API tokens

Scripts that talk to Sunshine usually need your admin password. Full control over everything. SolarFlare lets you create limited tokens with only the permissions you choose.

For example: a token that can read config and download logs but can't change settings, pair devices, or launch apps. Or a token that can only launch and stop apps.

Tokens are shown once when created (the server doesn't store the plain text). Revocable at any time.

### 17. System tuning for low latency

Three one-shot services that run at boot and optimize your system for streaming:

- **CPU booster:** Forces CPU to run at full speed instead of slowing down when idle. Prevents the "wakes up too slow" problem when a stream starts.
- **Network card optimizer:** Tunes Ethernet or Wi-Fi for lowest latency instead of highest throughput.
- **GPU clock locker:** Locks NVENC encoder clock to GPU maximum speed. Prevents quality dips when the GPU thinks it can rest.

One script installs all three. Completely optional.

### 18. Custom virtual display (Hermes-KMS)

A custom Linux kernel module that creates a bare virtual monitor using the direct kernel display interface. Capture happens with zero CPU involvement. The GPU writes frames directly to the encoder.

This requires installing a kernel module (the build script handles it). Not for everyone. But if you're building a dedicated streaming box and want every millisecond, this is the fastest option.

### 19. Faster monitor detection (skip_wayland_correlation)

On startup, SolarFlare normally checks connected monitors via Wayland. On most systems this is instant. On some KDE setups it hangs for seconds or times out entirely.

SolarFlare lets you skip that check. The tradeoff: absolute mouse coordinates won't be as accurate across multiple monitors. For single monitor setups (most gaming rigs), you won't notice any difference.

*Config: `skip_wayland_correlation` (default: off)*

### 20. Automatic CPU optimization during build

When you compile from source, SolarFlare reads your CPU model and picks the best compiler settings for your processor. The build script detects which Ryzen generation you have (Zen 1 through Zen 4) and enables the right instruction set extensions.

The binary is tuned for your exact CPU. No generic "works everywhere" compromise.

---

## Benchmarks

*All measurements on Ryzen 5 4600H, RTX 3060, Wi-Fi 6, 1080p, GNOME/Wayland.*

<div align="center">

| What's being measured | Regular Sunshine | SolarFlare | How much better |
|---|---|---|---|
| Button press to screen update | 18–65 ms | **5.5–12 ms** | **3–5× faster** |
| Network check delay | 80 µs | **15 µs** | **5× faster** |
| Audio/video sync offset | ~20 ms | **4–8 ms** | **2.5–5× tighter** |
| Worst random network spike | 47 ms | **&lt;2 ms** | **23× smaller** |

</div>

---

## All config settings

Every setting has a sensible default. You don't need to change anything to get the speed improvements. Settings live in `~/.config/sunshine/sunshine.conf` and can be edited through the web interface at `https://localhost:47990`.

### Network and speed

| Setting | Default | What it controls |
|---|---|---|
| `rate_cap_pct` | 80 | % of your network speed to use (50–95) |
| `busy_poll_us` | 50 | How often to check for network data in microseconds (0 = off) |
| `pipewire_latency_ms` | 8 | Audio buffer size in milliseconds (1–40) |
| `cpu_pinning` | on | Give streaming threads their own CPU cores |
| `enet_4mib_buffer` | on | Increase network buffer to 4 MB for 4K |
| `dscp_qos` | on | Tag network packets for QoS routers |
| `gpu_governor` | on | Keep GPU at full speed during streaming |
| `headless_virtual_display` | off | Create virtual display for headless streaming |
| `skip_wayland_correlation` | off | Skip monitor detection at startup |

### Audio processing

| Setting | Default | What it controls |
|---|---|---|
| `sf_audio_agc` | off | Auto volume leveling |
| `sf_audio_agc_target_db` | -20 | Target loudness level |
| `sf_audio_agc_max_gain_db` | 12 | Maximum volume boost |
| `sf_audio_agc_min_gain_db` | -12 | Maximum volume reduction |
| `sf_audio_agc_attack_ms` | 10 | How fast volume adjusts |
| `sf_audio_agc_hold_ms` | 200 | How long to hold before releasing |
| `sf_audio_agc_release_ms` | 100 | How fast to return to normal |
| `sf_audio_vad` | off | Voice activity detection |
| `sf_audio_vad_threshold_db` | -45 | How quiet before considered silence |
| `sf_audio_vad_hysteresis_db` | 6 | Hysteresis around the VAD threshold |
| `sf_audio_vad_min_speech_ms` | 100 | Min duration to trigger voice detection |
| `sf_audio_vad_min_silence_ms` | 200 | Min duration to release voice detection |
| `sf_audio_ducking` | off | Lower game volume when voice detected |
| `sf_audio_ducker_attenuation_db` | -12 | How much to lower volume during speech |
| `sf_audio_ducker_attack_ms` | 50 | How fast to lower volume when speech starts |
| `sf_audio_ducker_release_ms` | 500 | How fast to raise volume when speech ends |
| `sf_audio_noise_gate` | off | Mute quiet background noise |
| `sf_audio_noise_gate_db` | -55 | Noise gate threshold |

### Audio encoder (Opus)

| Setting | Default | What it controls |
|---|---|---|
| `sf_opus_application` | 0 | 0 = low delay, 1 = voice, 2 = music |
| `sf_opus_vbr` | 0 | 0 = constant quality, 1–2 = variable |
| `sf_opus_complexity` | 10 | Quality vs CPU usage (0–10) |
| `sf_opus_fec` | on | Error correction for spotty Wi-Fi |
| `sf_opus_expected_loss_pct` | 0 | Expected packet loss % (0–100) |
| `sf_opus_bandwidth_extension` | on | Allow higher audio frequencies |

### Video encoder (NVENC)

| Setting | Default | What it controls |
|---|---|---|
| `nvenc_tuning_preset` | -1 | -1 = manual, 0 = latency, 1 = balanced, 2 = quality |
| `nvenc_bframes` | 0 | Extra reference frames (0–4, more = better compression) |
| `nvenc_zerolatency` | off | Remove all encoder delay |
| `nvenc_rc_lookahead` | 0 | How many frames to look ahead (0–31) |
| `nvenc_aq_strength` | 8 | How hard to optimize dark/light areas (1–15) |
| `nvenc_temporal_aq` | off | Optimize across frames, not just within one |
| `nvenc_enable_min_qp` | off | Don't let quality go below a minimum |

### Per-game profiles

Add to `~/.config/sunshine/apps.json`:

```json
{
  "name": "Counter-Strike 2",
  "cmd": "steam steam://rungameid/730",
  "encoder-preset": 0
}
```

Values: `-1` = use your default, `0` = lowest latency, `1` = balanced, `2` = best quality.

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

### Build

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_DOCS=OFF -DBUILD_TESTS=OFF
cmake --build build -j$(nproc)
sudo cmake --install build
sudo setcap 'cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
```

---

## Testing

The automated suite covers config defaults, video presets, audio processing,
capture backends, bitrate control, and regression guards. Run it locally before
building and uploading a release. Some hardware-dependent tests are skipped
when the machine has no supported GPU, audio device, or input hardware.

```bash
cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build-tests -j$(nproc)
./build-tests/tests/test_sunshine --gtest_brief=1
```

---

## FAQ

**Will this break my existing Moonlight setup?**  
No. Same ports, same config files, same pairing. Switch between SolarFlare and regular Sunshine freely. All your settings and paired devices carry over.

**How do I go back to regular Sunshine?**  
On Arch: `sudo pacman -S sunshine`. If you built from source, the install manifest is at `build/install_manifest.txt`. Your config folder stays intact. Both versions use the same files.

**Does this work on Windows, Intel, or ARM?**  
No. SolarFlare is Linux only. Use regular Sunshine on those platforms.

**My game freezes during loading screens.**  
Fixed. The capture thread no longer uses real-time priority. Game threads can interrupt it when needed.

**I don't have an NVIDIA GPU. Does SolarFlare help me?**  
Yes. The network, audio, CPU, and headless features work on any GPU. The NVENC presets are NVIDIA only, but everything else benefits AMD and Intel GPUs too.

**How much CPU does SolarFlare use?**  
About the same as regular Sunshine. Busy-poll uses a tiny amount of extra CPU (one core checking for data 20,000 times per second), but it's offset by CPU pinning keeping streaming threads off your game's cores.

---

## Credits

SolarFlare is built on [LizardByte's Sunshine](https://github.com/LizardByte/Sunshine), based on the original Sunshine by Nathan Castle. The web interface, Moonlight protocol, and cross platform foundation are all their work.

Full changelog: docs/CHANGELOG-SolarFlare.md

---

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square" alt="License"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/issues"><img src="https://img.shields.io/github/issues/vindeckyy/Solar-Flare?style=flat-square" alt="Issues"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/releases"><img src="https://img.shields.io/github/v/release/vindeckyy/Solar-Flare?style=flat-square" alt="Release"></a>
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/changelog-SolarFlare-orange?style=flat-square" alt="Changelog"></a>
  <br><br>
  <strong>SolarFlare. Less lag, more game.</strong>
</p>
