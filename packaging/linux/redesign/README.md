# Solar-Flare fork redesign services

Boot-time tuning systemd units that the Solar-Flare fork expects on a
fresh CachyOS / Arch / Manjaro / EndeavourOS install. They are
**best-effort** — each unit probes hardware first and skips cleanly if
the hardware doesn't expose the relevant sysfs / driver / GPU API.

## What's in here

| File | Purpose |
|---|---|
| `systemd/cpu-performance.service` + `cpu-performance.sh` | Sets every online CPU's `scaling_governor` to `performance`. No-op on already-performance systems. |
| `systemd/nic-tuning.service` + `nic-tuning.sh` | Applies `ethtool -C` (adaptive-rx / rx-usecs) and `ethtool -G` (rx ring) to a list of common interface names. Skips NICs whose driver doesn't expose these knobs (e.g. `r8169`). |
| `systemd/nvidia-clock-lock.service` + `nvidia-clock-lock.sh` | Locks the NVIDIA GPU's clocks to its reported boost value via `nvidia-smi -lgc`. Probes first; skips cleanly on boxes without an NVIDIA GPU or without `coolbits`. |
| `install-redesign-services.sh` | Idempotent installer. Drops the units into `/etc/systemd/system` and the helper scripts into `/usr/local/sbin`. Pass `--uninstall` to remove. |

The service files are thin wrappers — each `ExecStart` just calls the
matching `.sh` helper. This keeps systemd's own specifier expansion from
clobbering shell variables inside complex commands (the earlier
inline-`/bin/sh -c` approach hit this: `${boost}` got expanded to
empty by systemd before the shell ever saw it).

## Why these live in the repo

Previously these units existed only on the live box as config drift. A
fresh `cachyos-build.sh` install would not get them, leaving the fork
in a state where upstream Sunshine boots cleanly but the fork-specific
tuning was missing — and a hard-coded clock value in an untracked unit
would also break on any hardware change (different NIC, different
NVIDIA card).

Tracking them here makes a clean install reproducible.

## Install paths after a pacman install

```
/usr/share/sunshine/redesign/systemd/cpu-performance.service
/usr/share/sunshine/redesign/systemd/cpu-performance.sh
/usr/share/sunshine/redesign/systemd/nic-tuning.service
/usr/share/sunshine/redesign/systemd/nic-tuning.sh
/usr/share/sunshine/redesign/systemd/nvidia-clock-lock.service
/usr/share/sunshine/redesign/systemd/nvidia-clock-lock.sh
/usr/share/sunshine/redesign/install-redesign-services.sh
/usr/share/sunshine/redesign/README.md
```

The `sunshine.install` post-install hook runs the installer, which
copies units to `/etc/systemd/system/` and helper scripts to
`/usr/local/sbin/` (matching where the live box already keeps the
existing `apply-tuned-undervolt.sh` and punktfunk helpers).

## Install on an existing box (developer working from a clone)

```sh
sudo ./packaging/linux/redesign/install-redesign-services.sh
```

The script auto-detects whether files live at `/usr/share/sunshine/redesign/systemd/`
(pacman install) or in the repo clone (developer path).

## What's deliberately not in here

These services exist on the live box but are personal / CPU-specific
tooling, not part of the fork:

- `punktfunk-*.service` — punktfunk latency-tuner (separate project)
- `ryzenadj-tuned.service` — Ryzen 5 4600H undervolt profile
- `bpftune.service`, `pci-latency.service`, `cachyos-iw-set-regdomain.service` — CachyOS distro defaults
- `nvidia-*.service` (hibernate/suspend/persistenced/powerd) — NVIDIA driver upstream

If the fork ever needs to ship general Ryzen tuning or per-distro
defaults, that goes in a separate directory.

## Verifying after install

```sh
systemctl status cpu-performance nic-tuning nvidia-clock-lock
```

All three should report `active (exited)` with status 0/SUCCESS.
A `failed` result on any of them is a bug — open an issue with the
output of `journalctl -b -u <unit>.service`.

Quick post-boot sanity:

```sh
nvidia-smi --query-gpu=clocks.current.graphics,clocks.max.graphics --format=csv
```

If the current clock is at max after a few seconds of streaming, the
clock lock is doing its job. If current stays at idle (300 MHz or so)
even during encode, the lock silently failed — check the journal.
