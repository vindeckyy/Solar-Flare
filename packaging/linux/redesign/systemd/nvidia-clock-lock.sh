#!/usr/bin/env bash
# Helper invoked by nvidia-clock-lock.service. Probes the GPU's boost clock
# via nvidia-smi and locks min/max to that value so the encoder doesn't wait
# for boost transitions during a streaming session. Exits 0 in every case —
# this script is best-effort, never a hard boot dependency.
set -u
if ! command -v nvidia-smi >/dev/null 2>&1; then
  echo "nvidia-clock-lock: nvidia-smi not found, skipping"
  exit 0
fi
if ! nvidia-smi -L >/dev/null 2>&1; then
  echo "nvidia-clock-lock: no NVIDIA GPU detected, skipping"
  exit 0
fi
sf_boost="$(nvidia-smi --query-gpu=clocks.max.graphics --format=csv,noheader,nounits 2>/dev/null | head -1 | tr -d ' ')"
if [ -z "${sf_boost}" ] || ! [ "${sf_boost}" -gt 0 ] 2>/dev/null; then
  echo "nvidia-clock-lock: could not read boost clock, skipping"
  exit 0
fi
echo "nvidia-clock-lock: locking clocks to ${sf_boost},${sf_boost} MHz"
nvidia-smi -pm 1 >/dev/null 2>&1 || true
if ! nvidia-smi -lgc "${sf_boost},${sf_boost}" 2>/dev/null; then
  echo "nvidia-clock-lock: nvidia-smi -lgc failed (coolbits/persistence?), continuing"
fi
exit 0
