# SolarFlare fork configuration

The SolarFlare fork adds five Linux-only tunables that let you dial
the CachyOS local-LAN fast path in or out without rebuilding. All five
live under the `solarflare_t` struct in `src/config.h` and are read
from the same `~/.config/sunshine/sunshine.conf` file as the upstream
options. They appear in `sunshine --version` (with
`min_log_level = 1`) as `config: '...' = ...` lines.

This document describes only the **fork-specific** keys. The full
upstream config documentation lives at
[docs/configuration.md](configuration.md); every option there is
still supported.

## The five tunables at a glance

| Key | Type | Default | Range | What it does |
|---|---|---|---|---|
| `busy_poll_us`        | int    | 50   | 0–10000 | `SO_BUSY_POLL` on the ENet UDP socket, in microseconds. 0 disables. |
| `rate_cap_pct`        | int    | 80   | 50–95   | Percent of the negotiated link speed used as the rate-control pacer. |
| `enet_4mib_buffer`    | bool   | true | -       | Grow ENet UDP send/recv buffers to 4 MiB (Linux only). |
| `pipewire_latency_ms` | int    | 8    | 1–40    | `PW_KEY_NODE_LATENCY` hint passed to the PipeWire compositor. |
| `cpu_pinning`         | bool   | true | -       | Push the capture thread onto `SCHED_RR` and pin it to a non-IRQ, non-SMT core. |
| `dscp_qos`            | bool   | true | -       | Tag ENet packets with DSCP CS3 so routers prioritize streaming over bulk traffic (Linux only). |
| `gpu_governor`        | bool   | true | -       | Set AMD GPU to `performance` power profile during stream, restore `auto` on disconnect (Linux only). |
| `headless_virtual_display` | bool | false | -    | If no displays detected, try creating a virtual xrandr output (Linux only, opt-in). |

Each one is opt-out — setting it back to its "fall back to upstream"
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

`SO_RCVBUFFORCE` / `SO_SNDBUFFORCE` are tried first — these let us
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

Values outside 1-40 are silently rejected. The value is converted to
nanoseconds (`ms * 1'000'000`) and formatted as `Ns/1000`, which is
the natural ratio for `pw-top` / `pw-dump` output.

### `cpu_pinning`

If `true` (default), on `adjust_thread_priority(critical)` the thread
is also pushed onto `SCHED_RR` priority 10 and pinned to a physical
core (round-robin across cores 1..N/2, skipping core 0 to avoid the
default IRQ affinity shadow and skipping SMT siblings).

Removes the 5-15 ms CFS tail-latency spikes that show up as frame
jitter under load, and keeps the thread's L1/L2 cache warm
frame-to-frame. The calls fail silently under containers, systemd-run
units, or non-`CAP_SYS_NICE` users — the upstream nice-only path
still applies.

If `false`, only the upstream `nice -15` is applied. Use this if:

- You're running under `systemd-run --user --scope` and the SCHED_RR
  call is producing noisy warnings in `journalctl --user -u sunshine`.
- You're on a Zen 1 / Bulldozer-era CPU where pinning to a single
  physical core actually hurts throughput more than it helps.

## Where these are used

| Tunable              | Files |
|----------------------|-------|
| `busy_poll_us`       | `src/network.cpp` |
| `rate_cap_pct`       | `src/stream.cpp` |
| `enet_4mib_buffer`   | `src/network.cpp` |
| `pipewire_latency_ms`| `src/platform/linux/pipewire.cpp` |
| `cpu_pinning`        | `src/platform/linux/misc.cpp` |

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
EOF

sunshine /tmp/sf-test.conf
# Look for these in the first ~20 log lines:
#   config: 'busy_poll_us' = 0
#   config: 'rate_cap_pct' = 95
#   config: 'enet_4mib_buffer' = false
#   config: 'pipewire_latency_ms' = 1
#   config: 'cpu_pinning' = false
#
# If all five appear with no "Unrecognized" warnings, the fork
# config plumbing is wired correctly.
```

## Verification on an existing install

After pulling a new SolarFlare build:

1. `sunshine --version` — should still exit 0 and show
   `Sunshine version: ... commit: ...` plus the publisher metadata.
   No `FATAL` lines.
2. `grep -c solarflare_t src/config.h` — should print `1` (the struct
   definition) plus `5` field declarations. `grep -c
   config::solarflare src/network.cpp src/stream.cpp
   src/platform/linux/misc.cpp src/platform/linux/pipewire.cpp` should
   total at least `5`.
3. The web UI at `https://localhost:47990` should NOT show the five
   fork tunables (they're intentionally not exposed; edit
   `~/.config/sunshine/sunshine.conf` directly if you want to change
   them). Everything else in the Configuration tab should look
   identical to upstream.

## See also

- [docs/configuration.md](configuration.md) — the full upstream
  configuration reference.
- [docs/PORTING.md](PORTING.md) — multi-distro build instructions.
- `README.md` (root) — quick-start and the build script entry point.