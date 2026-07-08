#!/usr/bin/env bash
# Helper invoked by cpu-performance.service. Sets every online CPU's
# scaling_governor to performance. Tolerates offline CPUs and CPUs without
# cpufreq exposed. Best-effort.
set -u
for sf_f in /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor; do
  [ -w "${sf_f}" ] && echo performance > "${sf_f}" 2>/dev/null || true
done
exit 0
