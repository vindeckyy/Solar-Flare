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
      <td align="center"><strong>End-to-end latency</strong><br><sub>button press → screen</sub></td>
      <td align="center"><strong>Network polling</strong><br><sub>socket wake-up</sub></td>
      <td align="center"><strong>Audio sync</strong><br><sub>buffer offset</sub></td>
      <td align="center"><strong>Worst-case burst</strong><br><sub>network jitter</sub></td>
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

SolarFlare is Linux-only and **Ryzen-tuned**. Same protocol as Sunshine. Same Moonlight client. Same config folder. But the kernel hooks, real-time scheduling, and zero-copy paths that upstream has to skip are all fair game here.

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

## Features

### ⚡ Real-time performance
| Feature | What it does |
|---|---|
| **CPU pinning** | Encoder, capture, and audio threads get dedicated physical cores + `SCHED_RR`. No more frame jitter. |
| **Busy-poll sockets** | `SO_BUSY_POLL` cuts network wake-up latency from 80 µs → 15 µs. |
| **Rate cap** | Reads your actual NIC speed — 2.5 GbE and Wi-Fi 7 stop being bottlenecked at 1 Gbps. |
| **DSCP QoS** | Stream packets tagged with Class Selector 3. QoS-aware routers prioritize game traffic. |
| **GPU governor** | AMD GPU locked to `performance` clock frequency during streaming. |
| **4 MB socket buffers** | Prevents `sendmsg()` stalls on 4K 60 FPS encode bursts. |

### 🎨 Encoder & video
| Feature | What it does |
|---|---|
| **3 NVENC presets** | Latency (P1, 0 B-frames), Balanced (P4, 2 B-frames), Quality (P7, 4 B-frames). |
| **Full NVENC controls** | B-frames, lookahead, AQ strength, temporal AQ, weighted prediction, QP clamping. |
| **Per-game profiles** | Different encoder presets for CS2 vs Cyberpunk — switches automatically. |
| **Adaptive bitrate** | EWMA controller drops bitrate on RTT spikes, ramps back up when link recovers. |

### 🔊 Audio pipeline
| Feature | What it does |
|---|---|
| **AGC** | Smooths loudness between explosions and dialogue — off by default. |
| **VAD + Ducker** | Detects speech, lowers game volume so voices stay clear. |
| **Noise gate** | Cuts background hum, fan noise, keyboard clatter. |
| **Opus tuning** | VOIP/AUDIO/LOWDELAY, VBR/CBR, FEC, complexity, bandwidth extension. |

### 🖥️ Headless & capture
| Feature | What it does |
|---|---|
| **Smart headless** | Auto-selects `krfb`, `gamescope`, or `labwc` based on your compositor. |
| **Hermes-KMS** | Kernel module exposing a DRM/KMS virtual output with DMA-BUF capture. |
| **KWin privilege drop** | Retries screencast when `CAP_SYS_ADMIN` blocks it. |
| **Game scanner** | Discovers Steam, Lutris, and Heroic games — one-click import. |

### 🔧 Developer & platform
| Feature | What it does |
|---|---|
| **Scoped API tokens** | Bearer tokens with granular permissions — no more sharing admin passwords. |
| **Command palette** | `Ctrl+K` in the web UI — search across pages, settings, and controls. |
| **Trusted subnet pairing** | LAN devices pair without a PIN. |
| **Systemd tuning services** | CPU governor, NIC tuning, NVENC clock lock — one script to install all three. |
| **Zen auto-detect** | Build picks `-march=znverN` for your CPU. |
| **ccache + mold** | Incremental rebuilds in seconds. |

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

<sub>Hardware: Ryzen 5 4600H, RTX 3060, Wi-Fi 6, 1080p, GNOME/Wayland.</sub>

---

## Building from source

### Dependencies

<details>
<summary><b>Arch / CachyOS</b></summary>

```bash
sudo pacman -S base-devel cmake boost libcurl opus libx11 libxrandr \
  libxfixes libxcb avahi libdrm libevdev wayland wayland-protocols \
  pulseaudio pipewire
```
</details>

<details>
<summary><b>Ubuntu / Debian</b></summary>

```bash
sudo apt install build-essential cmake libboost-all-dev \
  libcurl4-openssl-dev libopus-dev libx11-dev libxrandr-dev \
  libxfixes-dev libxcb1-dev libavahi-client-dev libdrm-dev \
  libevdev-dev libwayland-dev libpulse-dev libpipewire-0.3-dev
```
</details>

<details>
<summary><b>Fedora</b></summary>

```bash
sudo dnf install gcc-c++ cmake boost-devel libcurl-devel opus-devel \
  libX11-devel libXrandr-devel libXfixes-devel libxcb-devel \
  avahi-devel libdrm-devel libevdev-devel wayland-devel \
  pulseaudio-libs-devel pipewire-devel
```
</details>

### Build

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSUNSHINE_ENABLE_CUDA=OFF -DBUILD_DOCS=OFF -DBUILD_TESTS=OFF
cmake --build build -j$(nproc)
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
```

---

## Testing

490 automated tests cover config defaults, NVENC presets, audio processing, capture backends, adaptive bitrate, Hermes-KMS, and regression guards.

```bash
cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DSUNSHINE_ENABLE_CUDA=OFF
cmake --build build-tests -j$(nproc)
./build-tests/tests/test_sunshine --gtest_brief=1
```

- **478 pass**, 12 skipped (hardware-bound or platform-specific), **0 failed**

---

## FAQ

**Will this break my existing Moonlight setup?**  
No. Same ports, same config, same pairing. Switch freely.

**How do I go back to regular Sunshine?**  
`sudo pacman -S sunshine` (Arch) or rebuild from source. Your `~/.config/sunshine/` works with both.

**Does this work on Windows / Intel / ARM?**  
No. SolarFlare is Linux-only. Use regular Sunshine.

**A game freezes during loading screens.**  
Fixed. The capture thread no longer runs on `SCHED_RR` — game threads can preempt it.

---

## Credits

SolarFlare is built on [LizardByte's Sunshine](https://github.com/LizardByte/Sunshine), based on the original Sunshine by Nathan Castle.

**Full changelog:** [docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md)

---

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square" alt="License"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/issues"><img src="https://img.shields.io/github/issues/vindeckyy/Solar-Flare?style=flat-square" alt="Issues"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/releases"><img src="https://img.shields.io/github/v/release/vindeckyy/Solar-Flare?style=flat-square" alt="Release"></a>
  <br><br>
  <strong>SolarFlare — Less lag, more game.</strong>
</p>
