# Guides

Curated how-tos for SolarFlare operators. Start with the
[SolarFlare README](../README.md), [fork configuration](CONFIGURATION.md), and
[porting guide](PORTING.md). For corrections to this repository, follow
[CONTRIBUTING](../CONTRIBUTING.md).

> [!IMPORTANT]
> **SolarFlare vs upstream:** Guides marked *SolarFlare* apply to this fork's
> Linux install path (`./scripts/linux-install.sh`). Guides marked *Upstream*
> come from the LizardByte Sunshine community and may reference packaging, UI
> labels, or platform support that differs from SolarFlare.

---

## Quick navigation

| I want to… | Read |
|---|---|
| Install and pair on Linux | [Getting started - first run](getting_started.md#solarflare-on-linux-first-run) |
| Open firewall ports | [Getting started - firewall](getting_started.md#firewall-rules-by-distribution) |
| Migrate from GameStream | [GameStream migration](gamestream_migration.md) |
| Tune LAN latency | [Performance tuning](performance_tuning.md) + [CONFIGURATION.md](CONFIGURATION.md) |
| Fix stream problems | [Troubleshooting](troubleshooting.md) |
| Add Steam / Proton games | [App examples](app_examples.md) |
| Automate the host | [API](api.md) |

---

## SolarFlare guides

### LAN streaming baseline

**Goal:** Stable 1080p120 or 4K60 on a wired LAN with default fork settings.

1. Install with `./scripts/linux-install.sh` and enable the user service.
2. Wire the host with Ethernet; use Wi-Fi 6/7 on the client only if wired is impossible.
3. Leave fork defaults enabled (`cpu_pinning`, `enet_4mib_buffer`, `busy_poll_us`, `dscp_qos`).
4. In Moonlight, set bitrate to ~80% of measured iPerf throughput.
5. Run `iperf3` as described in [Troubleshooting - network test](troubleshooting.md).

> [!TIP]
> If the host NIC is 2.5 GbE but the client is 1 GbE, set `rate_cap_pct = 80`
> or lower to avoid buffer overruns - see [CONFIGURATION.md](CONFIGURATION.md#rate_cap_pct).

### Competitive / low-latency profile

**Goal:** Minimize end-to-end latency for fast-paced games.

Add to `~/.config/sunshine/sunshine.conf`:

```ini
latency_mode = aggressive
busy_poll_us = 75
pipewire_latency_ms = 4
cpu_pinning = true
nvenc_tuning_preset = 0
```

Per-game NVENC override in `apps.json`:

```json
"encoder-preset": 0
```

Disable V-Sync in-game; cap Moonlight bitrate only if packet loss appears.

> [!WARNING]
> `latency_mode = aggressive` trades some visual quality on software scaling
> paths. Test before using in single-player titles where fidelity matters.

### Headless and SSH access

**Goal:** Stream from a server or workstation without a local monitor.

**Path 1 - Virtual display (SolarFlare fork):**

```ini
headless_virtual_display = true
headless_width = 1920
headless_height = 1080
headless_refresh = 120
```

**Path 2 - Dummy plug (NVIDIA / stable modes):** Attach an HDMI dummy emulator;
log in via normal graphical session (GDM, SDDM, etc.).

**Path 3 - SSH + existing X11 session:**

```bash
ssh user@host 'export DISPLAY=:0; sunshine'
```

For headless X11 without autologin, see the upstream walkthrough
[Remote SSH Headless Setup](https://app.lizardbyte.dev/2023-09-14-remote-ssh-headless-sunshine-setup)
(*Upstream* - X11 + NVIDIA focus; adapt paths for SolarFlare's `sunshine` binary).

> [!CAUTION]
> Do not run two `sunshine` instances. Stop the systemd user service before
> foreground debugging: `systemctl --user stop app-dev.lizardbyte.app.Sunshine.service`.

### Multi-GPU workstations

**Goal:** Capture and encode from the correct GPU.

| Step | Action |
|---|---|
| 1 | Identify GPUs: `lspci \| grep -E 'VGA\|3D'` and `nvidia-smi` |
| 2 | Plug the monitor (or dummy) into the GPU you want to capture |
| 3 | Launch games on that GPU (`DRI_PRIME=1`, `prime-run`, or BIOS mux) |
| 4 | In Web UI, set display adapter / output if multiple heads are visible |
| 5 | Enable `gpu_governor = true` for AMD; install redesign `nvidia-clock-lock` for NVIDIA |

**eGPU:** The external card must drive the display being captured. Internal
panel-only laptops may need a mux switch or external monitor on the eGPU.

### HDR on Linux (experimental)

**Goal:** Stream HDR10 content to a capable Moonlight client.

Requirements:

- KMS capture backend (not X11-only or NvFBC for HDR)
- Compositor with HDR (KDE Plasma 6, Gamescope)
- HEVC Main 10 or AV1 10-bit encoder (VAAPI on AMD/Intel)
- HDR enabled in host OS and Moonlight client settings
- EDID emulator or HDR-capable display on host

See [Getting started - HDR](getting_started.md#hdr-support) for inherited
platform notes and Arch wiki links.

### Per-client household profiles

**Goal:** Different bitrate/resolution for a phone vs. a living-room TV.

Use `client_profile_<identifier>` in `sunshine.conf`. The identifier matches the
paired client name from the Web UI **PIN** / clients list.

```ini
client_profile_living-room-tv = {"max_bitrate": 150000, "hevc_mode": 1}
client_profile_phone = {"max_bitrate": 20000, "width": 1280, "height": 720}
```

Full field list: [CONFIGURATION.md - per-client profiles](CONFIGURATION.md#per-client-streaming-profiles).

### Webhooks and automation

**Goal:** Notify Home Assistant, Discord, or a custom service on stream start/stop.

```ini
webhook_secret = your-hmac-secret
webhook_url_0 = https://example.com/hooks/solarflare
```

Verify signatures with the `X-Solarflare-Signature` header. API tokens for
read-only stats: [API.md](api.md).

### Structured logging for observability

```bash
SUNSHINE_LOG_JSON=1 systemctl --user restart app-dev.lizardbyte.app.Sunshine.service
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -f
```

Each log line is JSON: `{"ts":"...","level":"...","msg":"..."}`. See
[Troubleshooting](troubleshooting.md#structured-json-logging).

---

## Migration and upgrade guides

| Guide | Document |
|---|---|
| GameStream → SolarFlare | [gamestream_migration.md](gamestream_migration.md) |
| Sunshine → SolarFlare (same PC) | [gamestream_migration.md#migrating-from-upstream-sunshine](gamestream_migration.md#migrating-from-upstream-sunshine) |
| In-place release update | [README - update](../README.md#update-an-existing-installation) |
| Distro port / manual build | [PORTING.md](PORTING.md) |
| GSMS app import | [gamestream_migration.md#automated-migration-with-gsms](gamestream_migration.md#automated-migration-with-gsms) |

---

## Application and launcher guides

| Topic | Location |
|---|---|
| Steam Big Picture, detached launches | [app_examples.md](app_examples.md) |
| Epic / URI launches | [app_examples.md - Epic](app_examples.md#epic-game-store-game) |
| Lutris, Heroic (scanner) | Web UI **Applications** → scan |
| Flatpak games on host | Prep commands may need `flatpak-spawn --host` (*Upstream* Flatpak note in [getting_started](getting_started.md)) |
| Gamescope nested session | [app_examples.md](app_examples.md) (compositor examples) |

---

## Upstream community guides

The [LizardByte community blog](https://app.lizardbyte.dev/blog) publishes
Sunshine guides that often apply to SolarFlare because the protocol and
`sunshine.conf` schema are shared. Treat packaging, service names, and screenshots
as *Upstream* unless they match this repository's README.

| Resource | Notes |
|---|---|
| [LizardByte blog](https://app.lizardbyte.dev/blog) | Tutorials, release notes |
| [YouTube playlist](https://www.youtube.com/playlist?list=PLMYr5_xSeuXAbhxYHz86hA1eCDugoxXY0) | Community videos (*Upstream*) |
| [Awesome Sunshine](awesome_sunshine.md) | Third-party tools ecosystem |
| [Moonlight wiki](https://github.com/moonlight-stream/moonlight-docs/wiki) | Client-side setup |

> [!NOTE]
> Third-party tools (Playnite plugins, custom frontends, etc.) target the
> GameStream API surface. They generally work with SolarFlare but are not
> maintained by the SolarFlare project.

---

## Platform-specific inherited guides

Detailed install instructions for FreeBSD, macOS, Windows, Docker, Flatpak, and
distro packages are in [Getting started](getting_started.md). Those paths install
**upstream Sunshine**, not SolarFlare, unless you build this repository yourself
on that platform.

| Platform | SolarFlare support |
|---|---|
| **Linux x86-64** | Primary; `./scripts/linux-install.sh` |
| **Linux (other arch)** | Source may build; not release-tested |
| **Windows / macOS** | Inherited code; use upstream Sunshine releases |
| **FreeBSD** | Inherited upstream packages only |

---

## Contributing a guide

Community guides are welcome. To add or fix documentation in this repository:

1. Read [CONTRIBUTING.md](../CONTRIBUTING.md) and [contributing.md](contributing.md).
2. Mark SolarFlare-only steps clearly when they differ from upstream.
3. Include verification steps (commands, expected output).
4. Open a pull request against `docs/` - do not create issues in the LizardByte org.

> [!TIP]
> Prefer linking to `CONFIGURATION.md` and `configuration.md` instead of
> duplicating full option tables in guide prose.

---

<div class="section_buttons">

| Previous                                |                                        Next |
|:----------------------------------------|--------------------------------------------:|
| [Awesome-Sunshine](awesome_sunshine.md) | [Performance Tuning](performance_tuning.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
