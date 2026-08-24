# Performance Tuning

SolarFlare is tuned for low-latency game streaming on trusted local networks.
This guide explains **every** performance lever: fork-specific configuration
keys, inherited upstream options, Linux host tuning, measurement methodology,
and ready-made profiles you can copy into `~/.config/sunshine/sunshine.conf`.

> [!IMPORTANT]
> Fork-specific keys are documented in [SolarFlare Configuration](CONFIGURATION.md).
> Inherited upstream keys live in the [complete configuration reference](configuration.md).
> This guide focuses on **how to combine** those options for real-world outcomes.

## Quick reference: where to tune what

| Goal | Primary docs | Key settings | Host-level tools |
|---|---|---|---|
| Lower end-to-end latency | [Latency mode](CONFIGURATION.md#latency_mode), [PipeWire](CONFIGURATION.md#pipewire_latency_ms), encoder presets | `latency_mode`, `pipewire_latency_ms`, `nvenc_tuning_preset`, `encoder` tab options | CPU governor, `SCHED_RR`, NVIDIA `nvidia-smi -lgc` |
| Smoother video on Wi-Fi | [Network tunables](CONFIGURATION.md#busy_poll_us) | `busy_poll_us`, `enet_4mib_buffer`, `rate_cap_pct`, `dscp_qos` | Router QoS, `sysctl` socket buffers |
| Stable 4K60 without drops | [Buffers & pacing](CONFIGURATION.md#enet_4mib_buffer) | `enet_4mib_buffer`, `rate_cap_pct`, bitrate caps in client | Link speed matching, `tc` shaping |
| Consistent frame times under load | [CPU pinning](CONFIGURATION.md#cpu_pinning) | `cpu_pinning`, capture backend selection | Isolate cores, disable SMT sibling contention |
| Better audio sync | [Audio FX](CONFIGURATION.md#audio-fx-pre-encoder-processing) | `pipewire_latency_ms`, `sf_audio_*`, `sf_opus_*` | PipeWire quantum, `pactl`/`pw-top` |
| Headless / no physical display | [Headless display](CONFIGURATION.md#headless_virtual_display) | `headless_virtual_display`, `headless_*` | Virtual outputs, compositor setup |

---

## Measurement before you tune

Blindly changing settings wastes time. Establish a baseline first.

### Moonlight client overlay

Enable the performance overlay in Moonlight (Settings → Debug → Performance overlay).
Record for each test session:

| Metric | What it tells you | Target (wired LAN) |
|---|---|---|
| Decode latency | Client decode + display path | Platform-dependent; lower is better |
| Network latency | RTT-style network component | < 5 ms on Gigabit LAN |
| Host processing latency | Capture + encode on host | < 10 ms for competitive profiles |
| Frame drops / frame rate | Transport stability | 0 drops at target FPS |
| Bitrate | Encoder + network throughput | Stable; spikes may indicate pacing issues |

### SolarFlare logs

```bash
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -f
```

Look for:

- `rate_cap_pct: detected link` - confirms link-speed detection
- Encoder init lines (NVENC, VA-API, software)
- Capture backend selection (KMS, X11, portal)
- Warnings about capabilities (`setcap`, `SCHED_RR`, buffer growth)

Set `min_log_level = 1` in `sunshine.conf` for verbose config dumps at startup.
Fork keys appear as `config: 'busy_poll_us' = 50` style lines when logging is verbose.

### Structured JSON logging

For automated analysis, enable JSON log lines:

```bash
export SUNSHINE_LOG_JSON=1
./sunshine 2>&1 | head -5
```

Each line is `{"ts":"...","level":"...","msg":"..."}`. Useful for correlating
frame events with network warnings in scripts.

### Network path testing

Before tuning SolarFlare, validate the path with iPerf3 (see
[Troubleshooting - Network performance test](troubleshooting.md#network-performance-test)):

```bash
# On host
iperf3 -s

# On client (example: 50 Mbps UDP reverse test)
iperf3 -c <host-ip> -t 60 -u -R -b 50M
```

Aim for **< 5% packet loss** and **< 1 ms jitter** at your target bitrate.
If the path fails here, fix networking before encoder tuning.

### API stream telemetry

When authenticated, SolarFlare exposes stream and latency endpoints documented in
[API](api.md). Use these to verify host-side state during a live session without
parsing logs manually.

---

## Tuning profiles (copy-paste starting points)

Add or merge these blocks into `~/.config/sunshine/sunshine.conf`.
Adjust bitrates in the Moonlight client to match your link.

### Profile A - Competitive latency (wired, NVIDIA NVENC)

Best for: FPS titles, 1080p120 or 1440p120 on a dedicated Gigabit+ link.

```ini
# Fork network & scheduling
busy_poll_us = 50
rate_cap_pct = 90
enet_4mib_buffer = true
dscp_qos = true
cpu_pinning = true
gpu_governor = true
latency_mode = aggressive
pipewire_latency_ms = 4
nvenc_tuning_preset = 0

# Inherited: prefer NVENC in Web UI Video tab; disable extra B-frames / lookahead
```

**Host extras:**

```bash
# Lock NVIDIA GPU clocks (replace <max> with your card's boost clock)
sudo nvidia-smi -lgc <max>,<max>
```

Revert with `sudo nvidia-smi -rgc` after testing.

### Profile B - Balanced quality (wired, any GPU)

Best for: single-player, 4K60 HEVC, visually rich titles.

```ini
busy_poll_us = 50
rate_cap_pct = 80
enet_4mib_buffer = true
dscp_qos = true
cpu_pinning = true
gpu_governor = true
latency_mode = safe
pipewire_latency_ms = 8
nvenc_tuning_preset = 2
```

Pair with Moonlight HEVC and a bitrate appropriate for 4K (typically 50–80 Mbps wired).

### Profile C - Wi-Fi friendly

Best for: Wi-Fi 6/7 clients where CPU wakeups and pacing matter more than peak bitrate.

```ini
busy_poll_us = 100
rate_cap_pct = 70
enet_4mib_buffer = true
dscp_qos = true
cpu_pinning = true
latency_mode = safe
pipewire_latency_ms = 8
```

**Router:** Enable WMM/QoS if available. `dscp_qos = true` tags streaming UDP with DSCP CS3;
benefit depends on router support.

**Client:** Cap Moonlight bitrate below the sustained Wi-Fi throughput from iPerf3.

### Profile D - Shared LAN (other heavy traffic)

Best for: household links with NAS backups, downloads, or multiple streams.

```ini
busy_poll_us = 0
rate_cap_pct = 60
enet_4mib_buffer = true
dscp_qos = true
cpu_pinning = true
latency_mode = safe
pipewire_latency_ms = 12
```

Lower `rate_cap_pct` leaves headroom for competing flows. Disable `busy_poll_us` if CPU is constrained.

### Profile E - AMD Mesa / VA-API

Best for: AMD GPUs using VA-API encode.

```ini
gpu_governor = true
cpu_pinning = true
latency_mode = safe
pipewire_latency_ms = 8
```

SolarFlare sets `AMD_DEBUG=lowlatencyenc` automatically when appropriate.
Verify with `amdgpu_top` - VCLK and DCLK should stay high during encode.
See [Troubleshooting - AMD encoding latency](troubleshooting.md#amd-encoding-latency-issues).

### Profile F - Headless server (no monitor)

```ini
headless_virtual_display = true
headless_width = 1920
headless_height = 1080
headless_refresh = 120
capture = kms
```

Requires KMS capture and proper capabilities. See
[CONFIGURATION - headless display](CONFIGURATION.md#headless_virtual_display) and
[Troubleshooting - KMS streaming fails](troubleshooting.md#kms-streaming-fails).

---

## Linux host tuning (beyond `sunshine.conf`)

SolarFlare applies several optimizations automatically when fork keys are enabled.
These sections cover **manual** host changes when you need more control.

### CPU governor and frequency scaling

For desktop hosts, set performance governor during streaming tests:

```bash
# AMD / Intel (example - sysfs path varies)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

SolarFlare's `cpu_pinning = true` elevates the capture thread to `SCHED_RR` and pins it
to a non-IRQ, non-SMT core. This removes 5–15 ms CFS tail-latency spikes that appear as
frame jitter under load.

**When to disable `cpu_pinning`:**

- Running under `systemd-run --user --scope` with noisy `SCHED_RR` warnings
- Containers without `CAP_SYS_NICE`
- Laptops on battery where core pinning fights power management

### IRQ affinity and isolated cores

On high-core-count hosts, consider isolating a core from the general scheduler
(`isolcpus` kernel parameter) and letting SolarFlare pin capture there.
This is advanced; most users achieve sufficient results with default `cpu_pinning`.

### GPU clocks and governors

#### AMD (`gpu_governor = true`)

When enabled, SolarFlare raises AMD DRM cards to the `performance` power state during
capture and restores `auto` on teardown (RAII guard in `src/video.cpp`).

#### NVIDIA

Use persistent clock locking for stable encode latency:

```bash
nvidia-smi -q -d CLOCK | grep -i max
sudo nvidia-smi -lgc <max_graphics_clock>,<max_graphics_clock>
```

See also [`packaging/linux/redesign`](../packaging/linux/redesign/README.md) for
optional boot-time performance services.

### PipeWire audio latency

`pipewire_latency_ms` (default `8`, range `1–40`) sets `PW_KEY_NODE_LATENCY` on the
capture node. Lower values reduce pre-encoder buffering at the risk of compositor underruns.

| Value | Use when |
|---|---|
| 1–3 ms | Wired LAN, strong CPU/GPU, competitive audio sync |
| 4–8 ms | Default sweet spot for 1080p120 / 4K60 |
| 12–20 ms | Crackling, underruns, or heavy desktop audio load |
| > 20 ms | Rarely useful for streaming; defeats fork tuning purpose |

Verify the active quantum with `pw-top` or `pactl list sources` while streaming.

### Socket buffering and low-latency polling

| Key | Mechanism | Tuning notes |
|---|---|---|
| `enet_4mib_buffer` | Grows UDP send/recv buffers to 4 MiB | Essential for 4K60 bursts; disable only for low-res tests |
| `busy_poll_us` | `SO_BUSY_POLL` on ENet socket | 50 µs default; 100–200 µs on Wi-Fi; 0 to save CPU |
| `dscp_qos` | DSCP CS3 on streaming packets | Requires router QoS/WMM; harmless if ignored |
| `rate_cap_pct` | Paces to % of detected link speed | Auto-reads `/sys/class/net/<iface>/speed`; falls back to 1 Gbps |

**Sysctl (optional, system-wide):**

```bash
# /etc/sysctl.d/99-solarflare.conf - example only; tune to your NIC
net.core.rmem_max = 4194304
net.core.wmem_max = 4194304
net.core.busy_read = 50
net.core.busy_poll = 50
```

Apply with `sudo sysctl --system`. SolarFlare attempts `SO_RCVBUFFORCE`/`SO_SNDBUFFORCE` first,
then falls back to `SO_RCVBUF`/`SO_SNDBUF` within `rmem_max` limits.

### Link-speed mismatch mitigation

If the host NIC is faster than the client path (e.g. 2.5 GbE host → Wi-Fi client),
bursts every ~16 ms at 60 FPS can overrun downstream buffers. Mitigations:

1. Lower Moonlight bitrate
2. Reduce `rate_cap_pct`
3. Use Linux `tc` HTB shaping on UDP port 47998 (see
   [Troubleshooting - Packet loss from buffer overruns](troubleshooting.md#packet-loss-from-buffer-overruns))
4. SolarFlare's post-0.23.1 network improvements may reduce the need for NIC throttling

---

## Encoder-specific tuning

### NVENC (`nvenc_tuning_preset`)

| Value | Profile | Trade-off |
|---|---|---|
| -1 | Manual - use Video tab / per-app overrides | Full control |
| 0 | Latency | Fewest B-frames, minimal lookahead |
| 1 | Balanced | Default fork recommendation for most titles |
| 2 | Quality | Higher quality, more GPU time |

Per-application overrides in `apps.json`:

```json
{
  "name": "Competitive",
  "cmd": "steam steam://rungameid/730",
  "encoder-preset": 0
}
```

See [Application examples](app_examples.md) for more patterns.

### VA-API / AMD

- Ensure Mesa ≥ 24.2 for low-latency encode (`AMD_DEBUG=lowlatencyenc`)
- Patent-restricted distro Mesa builds may lack encoders - see
  [Troubleshooting - Hardware encoding fails](troubleshooting.md#hardware-encoding-fails)

### Software encoding

`latency_mode = aggressive` selects `SWS_FAST_BILINEAR` for software scale paths.
Use only when hardware encode is unavailable; CPU load will be high at 1080p120+.

---

## Capture backend selection

Capture path dominates latency and compatibility.

| Backend | Best for | Caveats |
|---|---|---|
| KMS | Full-screen Linux gaming, headless | Requires `cap_sys_admin`; see setcap |
| X11 | Legacy Xorg sessions | Higher latency than KMS on some setups |
| Portal (PipeWire) | Wayland compositors | Correlation issues - see `skip_wayland_correlation` |
| NVFBC | NVIDIA on X11 (legacy) | CUDA/driver requirements |

Set `capture` in `sunshine.conf` or the Web UI. Wrong backend symptoms are covered in
[Troubleshooting](troubleshooting.md).

---

## Audio FX and Opus (pre-network)

Fork audio keys (`sf_audio_*`, `sf_opus_*`) process audio before Opus encode.
They trade CPU for clarity:

| Feature | Keys | When to enable |
|---|---|---|
| AGC | `sf_audio_agc_*` | Quiet microphones, variable input levels |
| Noise gate | `sf_audio_noise_gate_*` | Keyboard/mechanical noise |
| VAD / ducking | `sf_audio_vad_*`, `sf_audio_duck_*` | Voice chat alongside game audio |
| Opus tuning | `sf_opus_*` | Bitrate, complexity, FEC, DTX |

Start with defaults; enable one feature at a time and re-test latency in Moonlight.
Full key reference: [CONFIGURATION - Audio FX](CONFIGURATION.md#audio-fx-pre-encoder-processing).

---

## Windows GPU settings (inherited platforms)

SolarFlare's primary release target is Linux. These notes apply when running
inherited Windows builds.

### AMD

Enabling **Enhanced Sync** in AMD settings may reduce latency by roughly one frame
for `amfenc` and `libx264` encoders.

### NVIDIA

Enabling **Fast Sync** in NVIDIA Control Panel may reduce display-side latency.
For host-side encode stability, prefer NVIDIA driver-level frame pacing settings
consistent with your Moonlight client VSync choice.

---

## Per-client streaming profiles

SolarFlare supports per-client overrides via `client_profile_*` keys (see
[CONFIGURATION - Per-client streaming profiles](CONFIGURATION.md#per-client-streaming-profiles)).
Use these when one Moonlight device needs Wi-Fi-safe settings while another uses
competitive wired settings on the same host.

Example pattern:

```ini
# Default host profile in sunshine.conf
rate_cap_pct = 80

# Laptop on Wi-Fi (client UUID from logs or Web UI)
client_profile_<uuid>_rate_cap_pct = 65
```

---

## Webhooks and automation

Stream lifecycle webhooks (`webhook_url_*`, `webhook_secret`) do not affect stream
performance directly but enable external automation (LED indicators, AV receivers,
logging pipelines). See [API](api.md) and [CONFIGURATION - Webhooks](CONFIGURATION.md#webhooks).

---

## Troubleshooting tuning regressions

| Symptom | Likely cause | Fix |
|---|---|---|
| Higher latency after fork update | `latency_mode = aggressive` + quality expectations | Set `latency_mode = safe` or adjust encoder preset |
| CPU spikes on idle stream | `busy_poll_us` too high | Reduce to 50 or 0 |
| Audio crackling | `pipewire_latency_ms` too low | Raise to 8–12 ms |
| 4K stutter, no network loss | Buffers too small | Ensure `enet_4mib_buffer = true` |
| Pacing warnings in logs | Link speed detection failed | Set conservative `rate_cap_pct`; check `/sys/class/net/*/speed` |
| SCHED_RR warnings | Container or missing caps | Set `cpu_pinning = false` |

Full diagnostic flows: [Troubleshooting](troubleshooting.md).

---

## See also

- [SolarFlare Configuration](CONFIGURATION.md) - every fork key in depth
- [Complete configuration reference](configuration.md) - inherited upstream options
- [Application examples](app_examples.md) - per-title `apps.json` overrides
- [API](api.md) - automation and stream telemetry
- [Building](building.md) - compile-time options affecting capture/encode
- [Troubleshooting](troubleshooting.md) - symptom → cause → fix

<div class="section_buttons">

| Previous            |          Next |
|:--------------------|--------------:|
| [Guides](guides.md) | [API](api.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
