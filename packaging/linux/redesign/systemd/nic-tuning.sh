#!/usr/bin/env bash
# Helper invoked by nic-tuning.service. Applies ethtool coalescing + ring
# tuning to a list of common interface names. ethtool exits non-zero on
# drivers that don't expose these knobs (e.g. r8169) — we log and skip.
# Best-effort: never a hard boot dependency.
set -u
for sf_iface in enp2s0 enp3s0 enp4s0 enp5s0 eno1 eth0; do
  [ -d "/sys/class/net/${sf_iface}" ] || continue
  sf_drv="$(basename "$(readlink /sys/class/net/${sf_iface}/device/driver 2>/dev/null)" 2>/dev/null)"
  echo "nic-tuning: ${sf_iface} driver=${sf_drv}"
  ethtool -C "${sf_iface}" adaptive-rx off rx-usecs 0 2>/dev/null || true
  ethtool -G "${sf_iface}" rx 4096 2>/dev/null || true
done
exit 0
