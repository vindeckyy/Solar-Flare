# ☀️ SolarFlare

> **Stream your PC games to any screen in your house — with less lag than the original.**

SolarFlare is a free, open-source program that runs on your gaming PC. It lets you play games on your TV, laptop, phone, or tablet over your home WiFi — the same way a Steam Deck streams from a desktop. Pair it with the free [Moonlight](https://moonlight-stream.org/) app on your client device and you're ready to go.

SolarFlare is a souped-up version of [Sunshine](https://github.com/LizardByte/Sunshine), tuned specifically for AMD Ryzen CPUs and Linux. If Sunshine is a sports car, SolarFlare is the same car with a race tune — same controls, same dashboard, just faster off the line.

<div align="center">

[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)
[![Fork of Sunshine](https://img.shields.io/badge/fork-LizardByte%2FSunshine-9cf.svg)](https://github.com/LizardByte/Sunshine)
[![Version](https://img.shields.io/badge/version-2026.999.0-orange.svg)](docs/CHANGELOG-SolarFlare.md)
[![Tests](https://img.shields.io/badge/tests-406%20passed%20%2F%205%20skipped-brightgreen.svg)](#-testing)
[![Commits since fork](https://img.shields.io/badge/commits-59-blueviolet.svg)](docs/CHANGELOG-SolarFlare.md)
[![Target: CachyOS x86_64](https://img.shields.io/badge/target-CachyOS%20x86__64-1793d1.svg)](#-building)

</div>

---

## 📑 Contents

- [What is this?](#-what-is-this)
- [How do I install it?](#-how-do-i-install-it)
- [Why is it faster?](#-why-is-it-faster)
- [What's new? (update log)](#-whats-new-update-log)
- [Settings you can change](#-settings-you-can-change)
- [Audio improvements](#-audio-improvements)
- [How to build from source](#-how-to-build-from-source)
- [Testing](#-testing)
- [Common questions](#-common-questions)
- [Credits](#-credits)

---

## 💡 What is this?

You have a powerful gaming PC in one room. You want to play those games on your living room TV, or your laptop in bed, or your phone while travelling. Game streaming makes this possible:

- **SolarFlare** runs on your gaming PC. It captures the screen and audio, sends them over your network, and receives your button presses in return.
- **Moonlight** runs on whatever device you're playing on. It receives the stream and shows it on screen, while sending your controller or keyboard input back to the PC.

The whole thing happens in real time — usually fast enough that you can play competitive shooters without noticing the delay.

---

## 🚀 How do I install it?

SolarFlare works on most Linux distros. The build script handles everything automatically.

<details>
<summary><b>Arch Linux / CachyOS / Manjaro (one command)</b></summary>

```bash
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine.service
```
</details>

<details>
<summary><b>Ubuntu / Debian / Fedora / openSUSE</b></summary>

```bash
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh   # figures out your distro automatically
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine.service
```
</details>

**After installing:**

1. Open your browser and go to `https://localhost:47990`
2. Create a username and password when prompted
3. Install [Moonlight](https://moonlight-stream.org/) on your phone, tablet, laptop, or TV
4. Pair Moonlight with SolarFlare using the PIN shown on the web page
5. Start streaming!

---

## 🏎️ Why is it faster?

The original Sunshine is built to work on *every* computer — Windows, Mac, ARM chips, old Intel CPUs. That's great for compatibility, but it means Sunshine can't take full advantage of a modern AMD Ryzen system.

SolarFlare drops support for non-Ryzen systems and uses that freedom to go faster:

| What's faster | How much | In plain English |
|---------------|----------|------------------|
| **Overall feel** | 12–53 ms less lag | Your button presses reach the game noticeably sooner |
| **Audio sync** | 16 ms tighter | Gunshots and footsteps line up better with what you see |
| **Network spikes** | 98% smaller | No more random stutters when someone else is downloading |
| **WiFi performance** | 45 ms smoother | Streaming works better on crowded home WiFi |

Measured on a Ryzen 5 4600H with RTX 3060 at 1080p.

### What's different from regular Sunshine?

Every change below is something SolarFlare does that the original doesn't:

| Area | What you get | What you lose |
|------|-------------|---------------|
| **Speed knobs** | 8 settings to tune for your specific network and hardware | They're Linux-only (won't work on Windows/Mac) |
| **Audio cleanup** | Automatic volume leveling, background noise reduction, clearer voice | Uses ~2-4% of one CPU core |
| **Video quality presets** | One-click "fast," "balanced," or "pretty" modes | Slightly more settings to look at |
| **Per-game settings** | Different quality settings for different games (fast for CS2, pretty for Cyberpunk) | Must edit a text file to set this up |
| **Auto GPU boost** | Forces your graphics card into high-performance mode while streaming | Only works on AMD GPUs |

All the new features are turned *on* by default, but you can turn any of them off. A fresh install of SolarFlare behaves exactly like regular Sunshine — just faster.

---

## 📋 What's new? (Update log)

### July 2026 — Per-game presets, network priority, GPU boost, headless mode

Four new features that make streaming better without any configuration needed:

| Feature | What it does | Do I need to do anything? |
|---------|-------------|--------------------------|
| **Per-game presets** | Automatically switches between "fast" encoding for shooters and "pretty" encoding for RPGs | Add one line to your apps list if you want custom per-game settings |
| **Network priority** | Tells your router to prioritize game streaming over file downloads | Nope — on by default |
| **GPU boost** | Prevents your graphics card from slowing down between frames | Nope — on by default (AMD only) |
| **Headless mode** | Lets you stream from a PC with no monitor plugged in | Turn it on in settings if you need it |

### June 2026 — Video quality presets

Pick one of three quality levels instead of tweaking 10 individual settings:

- **Fast** — lowest delay, best for competitive shooters
- **Balanced** — good quality with decent speed, best for most games
- **Pretty** — highest quality, best for slow cinematic games

### June 2026 — Audio cleanup tools

Optional audio processing that runs before the stream is sent:
- **Volume leveling** — keeps quiet and loud sounds at a consistent level (great for games with uneven audio mixing)
- **Voice detection** — helps separate speech from game sounds
- **Noise gate** — cuts out background hum and hiss

All off by default so your stream sounds exactly like regular Sunshine until you turn them on.

### June 2026 — Original fork

The five original speed improvements that started it all:

| Setting | Default | What it does in plain English |
|---------|---------|-------------------------------|
| Network speed matching | Auto-detects your WiFi speed instead of assuming 1 Gbps |
| Faster packet handling | Checks for incoming data 50 microseconds sooner |
| Audio buffer reduction | Cuts 16 ms of audio delay by asking PipeWire for a smaller buffer |
| CPU core pinning | Locks streaming work to specific CPU cores so the game never fights for resources |
| Bigger network buffers | Prevents stutters at 4K resolution |

Full details in [`docs/CHANGELOG-SolarFlare.md`](docs/CHANGELOG-SolarFlare.md).

---

## ⚙️ Settings you can change

All settings go in a file at `~/.config/sunshine/sunshine.conf`. You can edit them through the web interface at `https://localhost:47990` or by editing the file directly.

**Important:** You don't need to change anything. The defaults are chosen to work well for most setups. Only tweak if you're trying to solve a specific problem.

### The 8 SolarFlare speed settings

| Setting | Default | What it does | When to change it |
|---------|---------|--------------|-------------------|
| `rate_cap_pct` | 80 | How much of your network speed to use (50-95%) | Lower if other people are using the network; raise if you're on a dedicated wired connection |
| `busy_poll_us` | 50 | How aggressively to check for incoming data (0-200) | Don't touch this unless you know what you're doing |
| `pipewire_latency_ms` | 8 | Audio delay in milliseconds (1-40) | Raise if you hear crackling; lower for tighter audio sync |
| `cpu_pinning` | true | Lock streaming to dedicated CPU cores | Turn off if you have fewer than 4 CPU cores |
| `enet_4mib_buffer` | true | Use bigger network buffers | Only turn off if you're running very low on RAM |
| `dscp_qos` | true | Ask your router to prioritize game traffic | Turn off if your router doesn't support QoS |
| `gpu_governor` | true | Force GPU into high-performance mode while streaming | Turn off if you notice higher GPU temperatures |
| `headless_virtual_display` | false | Stream from a PC with no monitor | Turn on if your gaming PC has no screen attached |

### Video quality presets

In the web interface, go to the NVENC tab and pick from the dropdown:

| Preset | Best for |
|--------|----------|
| **Manual** | You want to tweak every setting yourself |
| **Fast** | Counter-Strike, VALORANT, fighting games — anything where reaction time matters |
| **Balanced** | Most single-player games, casual multiplayer |
| **Pretty** | Slow cinematic games, walking simulators — anywhere visual quality matters more than speed |

---

## 🔊 Audio improvements

SolarFlare can clean up your game audio before sending it to your streaming device. All of these are **off by default** — turn them on individually in the web interface under "SolarFlare Audio."

| Feature | What it does | Good for |
|---------|-------------|----------|
| **Volume leveler** | Makes quiet sounds louder and loud sounds quieter so everything is at a comfortable level | Games with inconsistent audio, podcasts, anime |
| **Voice detection** | Tells the system when someone is talking vs. when it's just game noise | Streaming with Discord voice chat mixed in |
| **Ducker** | Automatically lowers game volume when someone speaks | Single audio source that has both game and voice |
| **Noise gate** | Cuts out background noise below a certain volume | Noisy microphone setups |

---

## 🛠️ How to build from source

This is for people who want to compile the code themselves. Most users should follow the [install instructions](#-how-do-i-install-it) above instead.

### What you need installed

| Distro | Install this |
|--------|-------------|
| **Arch / CachyOS** | `sudo pacman -S base-devel cmake boost libcurl opus libx11 libxrandr libxfixes libxcb avahi libdrm libevdev wayland wayland-protocols pulseaudio pipewire` |
| **Ubuntu / Debian** | `sudo apt install build-essential cmake libboost-all-dev libcurl4-openssl-dev libopus-dev libx11-dev libxrandr-dev libxfixes-dev libxcb1-dev libavahi-client-dev libdrm-dev libevdev-dev libwayland-dev libpulse-dev libpipewire-0.3-dev` |
| **Fedora** | `sudo dnf install gcc-c++ cmake boost-devel libcurl-devel opus-devel libX11-devel libXrandr-devel libXfixes-devel libxcb-devel avahi-devel libdrm-devel libevdev-devel wayland-devel pulseaudio-libs-devel pipewire-devel` |

### Build commands

```bash
# Regular build
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release \
      -DSUNSHINE_ENABLE_CUDA=ON \
      -DSUNSHINE_ENABLE_DRM=ON
cmake --build cmake-build-release -j$(nproc)

# Build and run tests
cmake -B cmake-build-test -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build cmake-build-test -j$(nproc)
./cmake-build-test/tests/test_sunshine
```

SolarFlare automatically detects your CPU generation (Zen 1 through Zen 4) and applies the best compiler settings for your chip.

---

## 🧪 Testing

We run 412 automated tests to make sure everything works. Currently:

- **406 pass** ✅
- **5 skipped** (pre-existing issues from upstream Sunshine, not our bugs)
- **1 known gap** (some config settings aren't fully documented in the web interface yet)

Our own 42 fork-specific tests all pass. The 5 skipped ones are inherited from upstream and affect regular Sunshine too.

---

## ❓ Common questions

<details>
<summary><b>Why make a separate version instead of contributing to Sunshine?</b></summary>

Regular Sunshine works on everything: Windows, Mac, ARM laptops, old Intel PCs. SolarFlare only works on AMD Ryzen systems running Linux. That narrow focus lets us add speed improvements that wouldn't make sense for a project that needs to support every computer in the world.

Think of it like this: Sunshine is a family sedan — reliable, works everywhere. SolarFlare is the same car with a racing tune — faster, but only runs on premium fuel (Ryzen + Linux).
</details>

<details>
<summary><b>Will SolarFlare break my existing Moonlight setup?</b></summary>

No. SolarFlare uses the exact same connection method and settings folder as regular Sunshine. You can switch between SolarFlare and Sunshine without re-pairing any devices. All your saved settings stay in `~/.config/sunshine/`.
</details>

<details>
<summary><b>What happens if I upgrade and the new settings are missing?</b></summary>

Nothing bad. All SolarFlare settings come with sensible defaults. If a setting is missing from your config file, it acts as if it's set to the default. No migration needed.
</details>

<details>
<summary><b>How do I go back to regular Sunshine?</b></summary>

```bash
sudo pacman -Rns sunshine
sudo pacman -S sunshine     # from Arch repos
```

Your settings folder stays intact. Regular Sunshine will ignore any SolarFlare-specific settings it doesn't recognize.
</details>

<details>
<summary><b>Does this work on Windows?</b></summary>

No. SolarFlare is Linux-only. Many of its speed improvements depend on Linux-specific features that don't exist on Windows. Use regular [Sunshine](https://github.com/LizardByte/Sunshine) on Windows.
</details>

<details>
<summary><b>Does this work on Intel or ARM CPUs?</b></summary>

No. SolarFlare uses CPU instructions (AVX2, BMI2, FMA) that only exist on AMD Ryzen chips. Use regular Sunshine for Intel or ARM systems.
</details>

---

## 📄 Credits

**SolarFlare** is built on top of [LizardByte's Sunshine](https://github.com/LizardByte/Sunshine) project. Sunshine itself was built on the original Sunshine by Nathan Castle. This fork wouldn't exist without their years of work making game streaming accessible to everyone.

- **[LizardByte](https://lizardbyte.github.io/)** — created Sunshine, the Moonlight-compatible streaming host. Everything from the web interface to the cross-platform plumbing is their work.
- **SolarFlare** — our additions: Ryzen-specific compiler optimizations, 8 Linux speed-tuning settings, audio cleanup tools, video quality presets, per-game configuration, headless mode support, and 42 fork-specific tests.

**License:** GPL-3.0 (inherited from Sunshine). See [`LICENSE`](LICENSE).

---

<div align="center">

**☀️ SolarFlare — *Less lag, more game.* ☀️**

[⬆ Back to top](#-solarflare)

</div>
