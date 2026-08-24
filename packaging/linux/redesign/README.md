# Solar-Flare fork redesign services

Boot-time **best-effort** systemd units that SolarFlare expects on a fresh
CachyOS / Arch / Manjaro / EndeavourOS install. Each unit probes hardware first
and exits cleanly when the relevant sysfs knob, NIC driver, or NVIDIA API is
absent - they must **never** block boot.

The maintained end-user path is [`scripts/linux-install.sh`](../../../scripts/linux-install.sh),
which runs this installer automatically (skipped on NixOS).

---

## What's included

| File | Purpose |
|------|---------|
| [`systemd/cpu-performance.service`](systemd/cpu-performance.service) + [`cpu-performance.sh`](systemd/cpu-performance.sh) | Sets every online CPU `scaling_governor` to `performance`. No-op if already performance or cpufreq missing. |
| [`systemd/nic-tuning.service`](systemd/nic-tuning.service) + [`nic-tuning.sh`](systemd/nic-tuning.sh) | `ethtool -C` (adaptive-rx off, `rx-usecs 0`) and `ethtool -G` (`rx 4096`) on common interface names. Skips drivers without these knobs (e.g. `r8169`). |
| [`systemd/nvidia-clock-lock.service`](systemd/nvidia-clock-lock.service) + [`nvidia-clock-lock.sh`](systemd/nvidia-clock-lock.sh) | Locks NVIDIA GPU clocks to reported boost via `nvidia-smi -lgc`. Skips when no GPU or `coolbits`/persistence unavailable. |
| [`install-redesign-services.sh`](install-redesign-services.sh) | Idempotent installer → `/etc/systemd/system` + `/usr/local/sbin`. Pass `--uninstall` to remove. |

Service files are thin wrappers: `ExecStart` calls the matching `.sh` helper so
systemd does not expand shell variables prematurely (an earlier inline
`/bin/sh -c` approach broke `${boost}` expansion).

Related **optional** post-install (also from `linux-install.sh`):

| Script | Purpose |
|--------|---------|
| [`install-hermes-kms.sh`](install-hermes-kms.sh) | Build + DKMS-install Hermes-KMS from `third-party/hermes-kms` |
| [`packaging/linux/solarflare-update-apply`](../../solarflare-update-apply) | Privileged Web UI self-update (polkit) |

---

## Install paths

### Developer / `linux-install.sh` (from clone)

```bash
sudo ./packaging/linux/redesign/install-redesign-services.sh
```

Source resolution order in `install-redesign-services.sh`:

1. `/usr/share/sunshine/redesign/systemd/` (if Arch packaging experiment installed files there)
2. `packaging/linux/redesign/systemd/` next to the script

### Arch packaging experiment (`sunshine.install`)

When built from in-tree Arch packaging, files may live under:

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

The `sunshine.install` post-install hook runs the installer, copying units to
`/etc/systemd/system/` and helpers to `/usr/local/sbin/` (alongside existing
`apply-tuned-undervolt.sh` / punktfunk helpers on reference hardware).

### Installed layout (after `install-redesign-services.sh`)

| Destination | Files |
|-------------|-------|
| `/etc/systemd/system/` | `cpu-performance.service`, `nic-tuning.service`, `nvidia-clock-lock.service` |
| `/usr/local/sbin/` | `cpu-performance.sh`, `nic-tuning.sh`, `nvidia-clock-lock.sh` |

Units are **`enable`d** for next boot, not started immediately.

Start now:

```bash
sudo systemctl start cpu-performance nic-tuning nvidia-clock-lock
```

---

## Uninstall

```bash
sudo ./packaging/linux/redesign/install-redesign-services.sh --uninstall
```

Disables units, removes unit files and `/usr/local/sbin/*.sh` helpers, runs
`systemctl daemon-reload`.

---

## Service behavior (detail)

### cpu-performance

- Iterates `/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`
- Writes `performance` when writable
- **Exit 0** always

### nic-tuning

Probes interfaces: `enp2s0`, `enp3s0`, `enp4s0`, `enp5s0`, `eno1`, `eth0`.

For each existing interface:

```bash
ethtool -C "${iface}" adaptive-rx off rx-usecs 0
ethtool -G "${iface}" rx 4096
```

Logs driver name; ignores ethtool errors.

**Customize:** edit `nic-tuning.sh` interface list for your NIC naming scheme
(systemd networkd predictable names vary by board).

### nvidia-clock-lock

1. Requires `nvidia-smi` and a visible GPU
2. Reads `clocks.max.graphics` as boost MHz
3. `nvidia-smi -pm 1` then `nvidia-smi -lgc "${boost},${boost}"`
4. Fails softly if CoolBits / persistence mode disallows locking

Verify after streaming:

```bash
nvidia-smi --query-gpu=clocks.current.graphics,clocks.max.graphics --format=csv
```

Current should approach max under load. Idle ~300 MHz with failed lock → check
journal (see below).

---

## Verifying after install

```bash
systemctl status cpu-performance nic-tuning nvidia-clock-lock
```

Expected: `active (exited)` and `status=0/SUCCESS` for all three.

Per-unit logs:

```bash
journalctl -b -u cpu-performance.service --no-pager
journalctl -b -u nic-tuning.service --no-pager
journalctl -b -u nvidia-clock-lock.service --no-pager
```

Quick post-boot sanity:

```bash
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor   # performance
nvidia-smi --query-gpu=clocks.current.graphics,clocks.max.graphics --format=csv
ethtool -c enp3s0 2>/dev/null | head   # rx-usecs if driver supports it
```

---

## Failure recovery

| Symptom | Cause | Fix |
|---------|-------|-----|
| Unit `failed` (non-zero) | Script bug or missing helper | Check `journalctl -b -u <unit>`; ensure `/usr/local/sbin/<name>.sh` is executable |
| `enable failed` during install | systemd not running (chroot) | Re-run installer on booted system |
| CPU still `powersave` | Intel P-state / amd-pstate ignores governor | Expected on some laptops; unit is best-effort |
| ethtool errors in log | Realtek `r8169`, Wi-Fi, or virtual NIC | Harmless; customize interface list or ignore |
| NVIDIA lock skipped | No discrete GPU | Expected on iGPU-only systems |
| `nvidia-smi -lgc failed` | CoolBits unset or driver policy | Enable persistence / CoolBits in `nvidia-settings`; or ignore if encode works |
| Services missing after pacman upgrade | Overwritten units | Re-run `install-redesign-services.sh` |
| Duplicate units from old manual install | Config drift | `--uninstall` then reinstall |

A `failed` result on any unit is unexpected - file an issue at
[vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare/issues) with
`journalctl -b -u <unit>.service` output.

---

## Integration with `linux-install.sh`

After `cmake --install`, the installer runs (non-NixOS):

```text
post-install  fork redesign services
  → sudo packaging/linux/redesign/install-redesign-services.sh
post-install  Hermes-KMS kernel module
  → sudo packaging/linux/redesign/install-hermes-kms.sh  (non-fatal on failure)
post-install  self-update helper
  → solarflare-update-apply + polkit policy
```

Hermes-KMS requires `dkms` and kernel headers (`linux-headers` / `linux-zen-headers`).
Failure is **non-fatal**; rerun manually when headers are installed.

---

## What's deliberately not in this directory

Personal or machine-specific units that exist on reference hardware but are **not**
part of the fork distribution:

| Unit / tool | Reason excluded |
|-------------|-----------------|
| `punktfunk-*.service` | Separate punktfunk latency-tuner project |
| `ryzenadj-tuned.service` | CPU-specific undervolt (Ryzen 5 4600H) |
| `bpftune.service`, `pci-latency.service`, `cachyos-iw-set-regdomain.service` | CachyOS distro defaults |
| `nvidia-*.service` (hibernate/suspend/persistenced/powerd) | NVIDIA driver / distro upstream |

General Ryzen tuning or per-distro defaults belong in a separate packaging path if
added later.

---

## NixOS

`linux-install.sh` **skips** these imperative units on NixOS. Express equivalent
tuning declaratively in your NixOS configuration (CPU governor, ethtool, NVIDIA
settings modules). See [Porting SolarFlare](../../../docs/PORTING.md#nixos).

---

## Security note

Units run as **root** at boot (`multi-user.target`). Scripts only write sysfs,
call `ethtool`, or invoke `nvidia-smi` - they do not fetch network content.
Review scripts before deploying on shared/administrated systems.
