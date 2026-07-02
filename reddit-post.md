**Title:** I spent weeks optimizing Sunshine for AMD Ryzen + Linux — here's SolarFlare

**Body:**

I use Sunshine + Moonlight to stream games from my PC to my TV. Sunshine is already fantastic, but I always noticed a bit of latency, especially over Wi-Fi when the network was busy.

One thing I realized is that Sunshine has to support *everything* — Windows, macOS, Linux, Intel, AMD, ARM, FreeBSD. Because of that, it can't really take advantage of AMD- or Linux-specific optimizations that would break other platforms.

So I started a fork called **SolarFlare**. It's Sunshine optimized specifically for AMD Ryzen on Linux, now at 59 commits with a pile of platform-specific tweaks baked in.

---

**Results**

Test system: Ryzen 5 4600H, RTX 3060, Wi-Fi 6, 1080p, GNOME/Wayland

End-to-end latency: Sunshine 18–65 ms → SolarFlare 5.5–12 ms
Network poll latency: Sunshine 80 µs → SolarFlare 15 µs
Worst-case send burst: Sunshine 47 ms → SolarFlare <2 ms
Audio sync offset: Sunshine ~20 ms → SolarFlare 4–8 ms

For me, that's the difference between "I can tell I'm streaming" and "feels like I'm sitting at the host PC."

---

**What's changed**

*Kernel-level network tunables (the original fork)*

These were previously hardcoded in Sunshine's source. Now they're configurable in sunshine.conf and auto-tuned by default:

- `rate_cap_pct` (default 80): Reads your actual interface speed from /sys instead of assuming 1 Gbps. Eliminated 45ms median / 98ms p99 latency on my Wi-Fi 6 link.
- `busy_poll_us` (default 50): Uses SO_BUSY_POLL to poll the NIC for 50µs instead of waiting for interrupts. Cuts receive-side wakeup from ~1ms to ~50µs.
- `pipewire_latency_ms` (default 8): Passes a 8ms latency hint to PipeWire instead of the upstream default of 20–40ms. Cuts 1–2 frames of audio buffering.
- `cpu_pinning` (default true): Pushes encoder/capture/audio threads onto SCHED_RR and pins them to physical cores, skipping core 0 to avoid IRQ conflicts. Removes 1–4ms scheduling hitches.
- `enet_4mib_buffer` (default true): Grows UDP socket buffers from ~200KB to 4 MiB so 4K60 never stalls on sendmsg().

*DSCP QoS packet tagging*

Tags streaming packets with DSCP CS3. Routers that honor QoS will prioritize SolarFlare over downloads and file transfers. One setsockopt call. On by default, disable with `dscp_qos = false`.

*GPU frequency governor*

On AMD GPUs, writes "performance" to /sys/class/drm/card*/device/power_dpm_force_performance_level when streaming starts, and "auto" when it stops. Prevents the GPU from clocking down between frames. On by default, disable with `gpu_governor = false`.

*NVENC tuning presets*

Instead of tweaking 10 encoder knobs, pick from three one-click presets in the Web UI:

- Latency-optimized: P1, 0 B-frames, zero-latency, no lookahead — competitive shooters
- Balanced: P4, 2 B-frames, 20-frame lookahead, spatial + temporal AQ — most games
- Quality-optimized: P7, 4 B-frames, 40-frame lookahead, full two-pass, min-QP — cinematic

New knobs exposed: B-frames, zero-latency toggle, lookahead frames (0–31), AQ strength (1–15), temporal AQ, weighted prediction, per-codec min-QP, filler data, encode surfaces. All in both the Web UI and sunshine.conf.

*Per-game encoder presets*

Add `"encoder-preset": 0` to any app in apps.json and SolarFlare automatically switches presets per game. CS2 gets latency mode, Cyberpunk gets quality mode. Restored to default when the session ends.

*Audio FX pipeline*

All off by default (bit-identical to upstream):

- AGC: automatic volume leveling with configurable target, attack, hold, release
- VAD: speech detection to drive the ducker
- Ducker: lowers game volume when someone's talking
- Noise gate: cuts background hum below a threshold

Plus Opus encoder tuning: application mode (VOIP/AUDIO/LOWDELAY), VBR, complexity (0–10), in-band FEC, packet loss hint, bandwidth extension up to 24 kHz.

*Headless virtual display*

If `headless_virtual_display = true` and no monitor is detected, SolarFlare creates a virtual xrandr output and re-enumerates displays. For headless gaming servers. Opt-in.

*Zen microarchitecture auto-detection*

The build script reads /proc/cpuinfo and applies -march=znver1/2/3/4 automatically, plus -flto -O3 -fno-plt. The binary uses AVX2, BMI2, and FMA natively.

*Multi-distro build*

One script (`./scripts/cachyos-build.sh`) detects your distro and installs packages automatically. Tested on Arch, CachyOS, Manjaro, EndeavourOS, Ubuntu, Debian, Pop!_OS, Fedora, Nobara, openSUSE.

*Everything else*

- Self-contained CI workflow (doesn't depend on LizardByte's release pipeline)
- 23 upstream commits cherry-picked since the June 2026 fork base
- 9 regression guard tests that fail the build if a cherry-pick gets accidentally reverted
- Web UI rebrand: SolarFlare logo, fork-named themes, footer pointing at the repo
- sunshine --version prints fork name, repo URL, and commit hash
- 22 upstream workflow files pinned to stop them firing on the fork

---

**The catch**

AMD Ryzen + Linux only. If you're on Windows, Intel, or ARM — use upstream Sunshine. SolarFlare intentionally drops cross-platform support to enable these optimizations.

---

**Install**

```
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine.service
```

Pair with Moonlight at https://localhost:47990.

---

**Status**

59 commits, 406/412 tests passing, 42 fork-specific tests, actively maintained.

**GitHub:** https://github.com/vindeckyy/Solar-Flare

If you try it, I'd love to hear your numbers — especially on different Ryzen gens or with Wi-Fi 7.
