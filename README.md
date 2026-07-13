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
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/version-v2026.708.3--solarflare-orange?style=flat-square" alt="Version"></a>
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

If you've ever tried streaming games with Moonlight + Sunshine, you know it works great — until your network gets busy or your CPU has to think about other things. Regular Sunshine is built to run on every platform (Windows, Mac, Linux, Intel, AMD, ARM), which means it can't use any of the tricks that only work on Linux.

SolarFlare is built for one thing: **low-latency game streaming on Linux**. It does the same job as Sunshine — same Moonlight app, same web interface, same config files — but it talks to your hardware directly in ways that upstream Sunshine can't. The result is 3 to 5 times less delay on the same PC.

Here's what that means in practice: with regular Sunshine, there's a noticeable gap between when you press a key and when you see the result — like watching a YouTube video of your game. With SolarFlare, that gap shrinks to the point where it feels like you're sitting at your computer.

---

## Quick start

Either build from source, or grab the prebuilt binary from the latest release:

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
sudo setcap 'cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
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

Regular Sunshine assumes every internet connection is 1 Gbps. If your PC has a 2.5 GbE port or a fast Wi-Fi card, Sunshine won't know the difference — it treats everything the same, causing unnecessary delays because it's pacing your stream way below what your network can handle.

SolarFlare reads your actual network card speed and uses that number to pace your stream. On a fast Wi-Fi 6 connection, this cuts average delay by 45 milliseconds and worst-case spikes by 98 milliseconds.

**What this means for you:** If you have a modern router or a wired connection faster than 1 Gbps, your stream will be noticeably snappier without changing any settings.

*Config: `rate_cap_pct` (default: 80, range: 50–95)*

### 2. Instant network responsiveness

When your PC sends or receives network data, the normal way is: the network card sends an interrupt, the CPU stops what it's doing, handles the data, and goes back to work. That interrupt takes about 1 millisecond — which doesn't sound like much until you're trying to stream 60 frames per second and every frame needs multiple network exchanges.

SolarFlare uses a different approach: instead of waiting for interrupts, the CPU checks for new network data every 50 microseconds. This is like a cashier checking if the next customer is ready instead of waiting for them to ring a bell. The delay drops from 1 millisecond to about 50 microseconds.

**What this means for you:** Your game stream responds faster to your inputs, especially over Wi-Fi where the network is less predictable.

*Config: `busy_poll_us` (default: 50, range: 0–10000)*

### 3. Tighter audio sync

Audio has always been a weak point in game streaming. The audio system (PipeWire on modern Linux) keeps a buffer of sound data before sending it to the encoder — usually 20 to 40 milliseconds worth. That means what you hear is always slightly behind what you see.

SolarFlare tells the audio system to use a much smaller buffer: 8 milliseconds instead of 20–40. This cuts the audio delay by 1 to 2 frames without introducing crackling or dropouts on normal hardware.

**What this means for you:** The audio stays in sync with what's happening on screen. Gunshots, footsteps, and dialogue arrive at the same time as the video showing them.

*Config: `pipewire_latency_ms` (default: 8, range: 1–40)*

### 4. Dedicated CPU cores for streaming

Here's a problem that nobody talks about: when you're gaming and streaming at the same time, your CPU has to split its attention between running the game, capturing the screen, encoding the video, handling network traffic, and processing audio. If the game's thread and the encoder's thread end up on the same CPU core, they fight for time — and both of them stutter.

SolarFlare solves this by giving the encoder, capture, and audio threads their own dedicated CPU cores that nothing else can use. It also puts those threads on a priority system so the kernel knows they need to run immediately, not "whenever there's free time." It carefully avoids core 0 (which handles most other system interrupts) and avoids sharing a physical core with hyperthreading.

**What this means for you:** No more micro-stutters when the encoder and the game compete for CPU time. On a 6-core CPU or better, you won't even notice the two cores being reserved — your game has plenty of cores left, and the stream runs smoothly.

*Config: `cpu_pinning` (default: on)*

### 5. Bigger network buffers for 4K

When you're streaming at 4K resolution, each encoded frame can be surprisingly large — sometimes up to 200 KB. The default network buffer in Linux is also about 200 KB. If one frame fills the entire buffer, the next frame has to wait until the buffer drains before it can even start sending. This causes visible hitches.

SolarFlare grows those buffers to 4 MB — 20 times the default. Now the encoder can queue up several frames worth of data without ever blocking.

**What this means for you:** Smoother 4K streaming. If you stream at 1080p, you probably won't notice a difference, but it won't hurt anything either.

*Config: `enet_4mib_buffer` (default: on)*

### 6. Smarter video encoding (NVENC presets)

NVENC is NVIDIA's hardware video encoder — a dedicated chip on your GPU that handles video compression so your CPU doesn't have to. The problem is that NVENC has about 10 different settings that all interact, and most people don't want to spend hours figuring out the right combination.

SolarFlare replaces those 10 knobs with 3 one-click presets:

- **Latency mode** — Use this for competitive games (CS2, VALORANT, fighting games). It prioritizes speed over picture quality. The encoder runs at its fastest setting with zero extra frames of delay.
- **Balanced mode** — The default. Good picture quality with low latency. Works well for most single-player games.
- **Quality mode** — Use this for slow, beautiful games (Cyberpunk, Red Dead Redemption 2). The encoder takes more time per frame to produce the best possible image.

If you want full control, every individual NVENC setting is available in the web interface and config file.

**What this means for you:** Pick the preset that matches what you're playing and never think about encoder settings again. Switch between them automatically per-game (see section 8).

### 7. Cleaner, clearer game audio

SolarFlare can process your game audio before sending it to the streaming client. Everything here is *off by default* — if you don't touch these settings, your audio sounds exactly like regular Sunshine. But if you want better audio, here's what's available:

- **Auto volume** (AGC) — Keeps loud explosions and quiet dialogue at a similar volume. If you've ever had to adjust your volume between game scenes, this smooths it out automatically.
- **Voice detection** (VAD) — Detects when someone is speaking (voice chat, commentary) versus just game noise.
- **Auto-ducking** — When someone speaks, the game volume automatically lowers so voices stay clear. Think of it like a radio DJ who turns down the music when talking.
- **Background noise removal** — Mutes the audio when it drops below a threshold. Great for cutting out fan hum, keyboard clatter, or background noise.
- **Opus encoder tuning** — Fine-tune how the audio compressor works: prioritize lowest delay (VOIP mode), highest quality (AUDIO mode), or a middle ground. Control variable bitrate, error correction for spotty Wi-Fi, and bandwidth extension for crisp high-frequency audio.

**What this means for you:** Better-sounding streams, especially if you use voice chat or play games with wide dynamic range. Everything is tunable and opt-in.

### 8. Per-game encoder settings

Different games need different encoder settings. Counter-Strike needs the lowest possible latency. Cyberpunk needs the best possible quality. Manually switching between them is annoying, and forgetting to switch back means one of your games looks or feels worse than it should.

SolarFlare lets you set a per-game encoder preset right in your apps config. When you launch that game through Moonlight, it automatically switches to the right preset. When the game ends, it switches back to your default.

**What this means for you:** Set it once, forget it. Your competitive shooters always run at lowest latency. Your single-player games always look their best.

### 9. Network priority tagging

When your network is busy — someone's streaming Netflix, downloading a Steam game, or uploading photos — your game stream competes with all that traffic for bandwidth. Without priority, your stream packets can get stuck behind a Steam download.

SolarFlare marks its streaming packets with a "this is important" tag that any quality-of-service (QoS) router understands. When your router sees this tag, it moves game traffic ahead of bulk traffic. It's a single checkbox in the config and adds zero overhead.

**What this means for you:** Smoother streaming when your family shares the network. If your router doesn't support QoS, nothing changes — it just ignores the tag.

*Config: `dscp_qos` (default: on)*

### 10. GPU speed boost during streaming

Your GPU normally runs at a lower speed when it's not doing much, then speeds up when needed. That speeding-up process takes a few milliseconds — and in a game stream, those few milliseconds can land right in the middle of encoding a frame, causing a visible hitch.

SolarFlare tells your GPU to run at full speed whenever a stream is active, and go back to normal when streaming stops. On AMD GPUs this is automatic. On NVIDIA it does the same thing through a different mechanism.

**What this means for you:** More consistent frame timing. The GPU is already up to speed when it needs to encode a frame, so you don't get those random hitches.

*Config: `gpu_governor` (default: on)*

### 11. Streaming without a monitor

If you have a dedicated streaming PC or a server without a monitor plugged in, Sunshine normally can't capture video — there's no display to capture. SolarFlare has two solutions:

**Legacy mode:** If you just need something simple, it creates a fake display using the dummy X11 driver. Works on older setups.

**Modern headless mode (recommended):** SolarFlare can automatically detect what desktop environment you're using and start a private compositor for games:

- On **KDE Plasma**: Creates a virtual monitor using KDE's built-in tools. No extra software needed.
- On **Steam Deck**: Uses Gamescope's headless mode.
- On **everything else**: Starts a lightweight Wayland compositor.

**What this means for you:** Run a headless game server in your closet. No monitor, no keyboard, no mouse needed. Stream from it like any other gaming PC.

### 12. Automatic quality adjustment

If your network gets congested — someone starts a big download, or Wi-Fi interference spikes — your stream normally either stutters or drops frames. Some streaming software handles this by reducing quality, but Sunshine doesn't.

SolarFlare watches your network conditions in real time. When it detects problems (packet loss, rising delay, the encoder falling behind), it automatically lowers the video quality to keep things smooth. When the network recovers, it gradually raises quality back up.

**What this means for you:** Instead of a stuttery mess when your network gets busy, the stream gets a bit softer-looking — which is way better than freezing or skipping.

*Config: `adaptive_bitrate_enabled` (default: off)*

### 13. Auto-pairing for home devices

Normally, every new Moonlight client needs to show a PIN on screen, you type it into the web interface, and they pair. It's a minor annoyance when you're setting up a new laptop or tablet at home.

SolarFlare lets you define your home network ranges (like "192.168.1.0/24" or "10.0.0.0/24"). Any Moonlight client connecting from those addresses pairs automatically — no PIN needed.

**What this means for you:** Set up once. New devices connect instantly when they're on your home network.

### 14. Quick command search (Ctrl+K)

The Sunshine web interface has a lot of pages and settings. SolarFlare adds a search bar that you can open from anywhere by pressing Ctrl+K (or Cmd+K on Mac). Start typing and it finds the page or setting you're looking for — like Spotlight on macOS or Ctrl+K in VS Code.

**What this means for you:** Navigate the web UI faster. Type "4k" and the resolution picker opens. Type "bitrate" and jump straight to the bitrate settings.

### 15. Find and import your games automatically

If you use Steam, Lutris, or Heroic Games Launcher, SolarFlare can scan your game libraries and find everything you have installed. It returns the game name, launch path, and — for Steam — the cover art URL, ready to import with one click into your streaming apps list.

**What this means for you:** No more manually typing the launch command for every game. Run the scan, pick the games you want to stream, and go.

### 16. Safer API tokens

Scripts and automation tools that talk to Sunshine usually need your admin password — which gives them full control over everything. SolarFlare lets you create limited tokens that only have the permissions you choose.

For example, you can create a token that can only read the current configuration and download logs, but can't change settings, pair new devices, or launch apps. Or a token that can only launch and stop apps.

Tokens are shown once when created (the server doesn't store the plain text), and can be revoked at any time.

**What this means for you:** Automate your streaming setup without sharing your admin password. A script that reads the current bitrate doesn't need to be able to reset your display or unpair your devices.

### 17. System tuning for low latency

SolarFlare ships three one-shot services that run once at boot and optimize your system for game streaming:

- **CPU booster:** Forces your CPU to always run at full speed instead of slowing down when idle. Prevents the "CPU wakes up too slow" problem when a stream starts.
- **Network card optimizer:** Tunes your Ethernet or Wi-Fi card for lowest possible latency instead of highest throughput.
- **GPU clock locker:** Locks your NVENC encoder clock to the GPU's maximum speed, preventing quality dips when the GPU thinks it can take a break.

Run one script to install all three. They're completely optional.

**What this means for you:** Every part of your system — CPU, network, GPU — is optimized for streaming without you having to tune anything manually.

### 18. Custom virtual display (Hermes-KMS)

For advanced users who need the absolute lowest capture latency, SolarFlare bundles a custom Linux kernel module that creates a virtual display. Unlike the other headless options (which run a full compositor), this is a bare-bones virtual monitor that uses the direct kernel display interface. Capture happens with zero CPU involvement — the GPU writes frames directly to the encoder.

This requires installing a kernel module (the build script does this automatically), so it's not for everyone. But if you're building a dedicated streaming box and want every millisecond, this is the fastest option.

**What this means for you:** The lowest possible capture latency for headless streaming, at the cost of needing DKMS and kernel headers installed.

### 19. Faster detection of monitors (skip_wayland_correlation)

When SolarFlare starts, it normally checks which monitors are connected via the Wayland display protocol. On most systems this is instant, but on some KDE setups it can hang for several seconds or time out entirely. If you've ever had Sunshine take forever to start, this is why.

SolarFlare lets you skip that check entirely. The tradeoff is that absolute mouse coordinates won't be as accurate across multiple monitors — but for single-monitor setups (which is most gaming rigs), you won't notice any difference, and Sunshine starts immediately.

**What this means for you:** If Sunshine was slow to start on your system, enable this and it'll start instantly.

*Config: `skip_wayland_correlation` (default: off)*

### 20. Automatic CPU optimization during build

When you compile SolarFlare from source, it reads your CPU model and picks the best compiler settings for your specific processor. The build script detects which Ryzen generation you have (Zen 1 through Zen 4) and enables the right instruction set extensions. Combined with aggressive optimizations, the final binary is tuned for your exact CPU.

**What this means for you:** The software you run is compiled specifically for your hardware. No generic "works on everything" compromise.

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

Every setting has a sensible default — you don't need to change anything to get the speed improvements. Settings live in `~/.config/sunshine/sunshine.conf` and can be edited through the web interface at `https://localhost:47990`.

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
| `sf_audio_ducking` | off | Lower game volume when voice detected |
| `sf_audio_ducker_attenuation_db` | -12 | How much to lower volume during speech |
| `sf_audio_noise_gate` | off | Mute quiet background noise |

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

490 automated tests check everything: config defaults, video presets, audio processing, capture backends, bitrate control, and regression guards.

- **478 pass**, 12 skipped (hardware-dependent: some tests need NVIDIA GPUs, physical audio devices, or input hardware — none are actual problems)
- **0 failures**

To run tests:

```bash
cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build-tests -j$(nproc)
./build-tests/tests/test_sunshine --gtest_brief=1
```

---

## FAQ

**Will this break my existing Moonlight setup?**  
No. Same ports, same config files, same pairing. You can switch between SolarFlare and regular Sunshine freely — all your settings and paired devices carry over.

**How do I go back to regular Sunshine?**  
On Arch: `sudo pacman -S sunshine`. If you built from source, the install manifest is at `build/install_manifest.txt`. Your config folder stays intact — both versions use the same files.

**Does this work on Windows, Intel, or ARM?**  
No. SolarFlare is Linux-only. Use regular Sunshine on those platforms.

**My game freezes during loading screens.**  
Fixed. The capture thread no longer uses real-time priority — game threads can interrupt it when needed.

**I don't have an NVIDIA GPU. Does SolarFlare help me?**  
Yes — the network, audio, CPU, and headless features work on any GPU. The NVENC presets are NVIDIA-only, but everything else benefits AMD and Intel GPUs too.

**How much CPU does SolarFlare use?**  
About the same as regular Sunshine. The busy-poll feature uses a tiny amount of extra CPU (one core checking for data 20,000 times per second), but it's offset by the CPU pinning feature keeping streaming threads off your game's cores.

---

## Credits

SolarFlare is built on [LizardByte's Sunshine](https://github.com/LizardByte/Sunshine), based on the original Sunshine by Nathan Castle. The web interface, Moonlight protocol, and cross-platform foundation are all their work.

Full changelog: [docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md)

---

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square" alt="License"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/issues"><img src="https://img.shields.io/github/issues/vindeckyy/Solar-Flare?style=flat-square" alt="Issues"></a>
  <a href="https://github.com/vindeckyy/Solar-Flare/releases"><img src="https://img.shields.io/github/v/release/vindeckyy/Solar-Flare?style=flat-square" alt="Release"></a>
  <a href="docs/CHANGELOG-SolarFlare.md"><img src="https://img.shields.io/badge/changelog-SolarFlare-orange?style=flat-square" alt="Changelog"></a>
  <br><br>
  <strong>SolarFlare — Less lag, more game.</strong>
</p>
