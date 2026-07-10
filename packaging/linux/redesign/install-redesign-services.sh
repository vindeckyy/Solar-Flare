#!/usr/bin/env bash
#
# packaging/linux/redesign/install-redesign-services.sh
#
# Install the Solar-Flare fork's boot-time tuning systemd units + helper
# scripts. Idempotent. Safe to re-run.
#
# What it does:
#   1. Copy each *.sh helper script from this directory's systemd/ to
#      /usr/local/sbin/. (Matches where ryzenadj-tuned.service and the
#      punktfunk helpers already live on this box.)
#   2. Copy each *.service file from this directory's systemd/ to
#      /etc/systemd/system.
#   3. Run `systemctl daemon-reload`.
#   4. `systemctl enable` each unit (no `start` — they fire at next boot).
#
# Usage:
#   sudo ./packaging/linux/redesign/install-redesign-services.sh
#
# To uninstall:
#   sudo ./packaging/linux/redesign/install-redesign-services.sh --uninstall
#
# When run from a system that already has the fork installed via pacman,
# the redesign files live at /usr/share/sunshine/redesign/systemd/ instead.
# That path is checked first if it exists.

set -euo pipefail

# Resolve our source directory even when called via absolute path.
if [[ -d /usr/share/sunshine/redesign/systemd ]]; then
    SRC_DIR="/usr/share/sunshine/redesign/systemd"
else
    SRC_DIR="$(cd "$(dirname "$0")/systemd" && pwd)"
fi

DST_SCRIPT_DIR="/usr/local/sbin"
DST_UNIT_DIR="/etc/systemd/system"
SERVICES=(cpu-performance nic-tuning nvidia-clock-lock)

action="install"
if [[ "${1:-}" == "--uninstall" ]]; then
    action="uninstall"
fi

if [[ "$action" == "install" ]]; then
    for svc in "${SERVICES[@]}"; do
        sh_src="$SRC_DIR/${svc}.sh"
        svc_src="$SRC_DIR/${svc}.service"
        sh_dst="$DST_SCRIPT_DIR/${svc}.sh"
        svc_dst="$DST_UNIT_DIR/${svc}.service"
        if [[ -f "$sh_src" ]]; then
            install -m 755 "$sh_src" "$sh_dst"
            echo "installed script: $sh_dst"
        fi
        if [[ -f "$svc_src" ]]; then
            install -m 644 "$svc_src" "$svc_dst"
            echo "installed unit:   $svc_dst"
            systemctl enable "${svc}.service" 2>&1 || echo "  enable failed (will retry next boot)"
        fi
    done
    systemctl daemon-reload
    echo
    echo "All redesign services installed and enabled."
    echo "They will start at next boot. To start them now:"
    echo "  sudo systemctl start cpu-performance nic-tuning nvidia-clock-lock"
else
    for svc in "${SERVICES[@]}"; do
        svc_dst="$DST_UNIT_DIR/${svc}.service"
        if [[ -f "$svc_dst" ]]; then
            systemctl disable "$svc.service" 2>/dev/null || true
            rm -f "$svc_dst"
            echo "removed unit: $svc_dst"
        fi
        sh_dst="$DST_SCRIPT_DIR/${svc}.sh"
        if [[ -f "$sh_dst" ]]; then
            rm -f "$sh_dst"
            echo "removed script: $sh_dst"
        fi
    done
    systemctl daemon-reload
    echo "All redesign services removed."
fi
