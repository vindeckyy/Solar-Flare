#!/usr/bin/env bash
#
# packaging/linux/redesign/install-hermes-kms.sh
#
# Build and load the Hermes-KMS kernel module from the SolarFlare vendored
# source tree (third-party/hermes-kms). DKMS-installs it so it survives
# kernel upgrades, then optionally loads it.
#
# This script is invoked by scripts/cachyos-build.sh after `cmake --install`.
# It is safe to re-run. It refuses to build if the kernel-headers package is
# missing and prints the exact pacman / apt / dnf command to install it.
#
# Why DKMS and not "make modules" once: EVDI uses the same pattern. Without
# DKMS, every kernel update requires a manual rebuild, and `modprobe hermes_kms`
# on the new kernel fails silently with "module not found". DKMS hooks into
# the kernel post-install and rebuilds automatically.
#
# Usage:
#   sudo ./packaging/linux/redesign/install-hermes-kms.sh
#   sudo ./packaging/linux/redesign/install-hermes-kms.sh --load
#   sudo ./packaging/linux/redesign/install-hermes-kms.sh --uninstall

set -euo pipefail

# Resolve repo root from this script's location, regardless of cwd.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
HK_SRC="$REPO_ROOT/third-party/hermes-kms"

if [[ ! -d "$HK_SRC" ]]; then
    echo "fatal: $HK_SRC not present." >&2
    echo "       Run: git submodule update --init --recursive" >&2
    exit 1
fi

if [[ $EUID -ne 0 ]]; then
    echo "fatal: this script must run as root (needs to call dkms and modprobe)." >&2
    exit 1
fi

action="install"
if [[ "${1:-}" == "--uninstall" ]]; then
    action="uninstall"
elif [[ "${1:-}" == "--load" ]]; then
    action="load"
fi

detect_pkg_manager() {
    if   command -v pacman >/dev/null 2>&1; then echo "pacman"
    elif command -v apt-get >/dev/null 2>&1; then echo "apt"
    elif command -v dnf >/dev/null 2>&1; then echo "dnf"
    elif command -v zypper >/dev/null 2>&1; then echo "zypper"
    else echo "unknown"
    fi
}

ensure_kernel_headers() {
    local pm
    pm="$(detect_pkg_manager)"
    case "$pm" in
        pacman)
            # CachyOS / Arch: linux-zen-headers, linux, linux-lts, etc. Probe running kernel.
            local krel
            krel="$(uname -r)"
            local hdr
            case "$krel" in
                *-zen*)      hdr="linux-zen-headers" ;;
                *-lts*)      hdr="linux-lts-headers" ;;
                *-hardened*) hdr="linux-hardened-headers" ;;
                *-cachyos*)  hdr="linux-cachyos-headers" ;;
                *)           hdr="linux-headers" ;;
            esac
            if ! pacman -Qq "$hdr" >/dev/null 2>&1; then
                echo "fatal: kernel headers not installed." >&2
                echo "       Run: sudo pacman -S $hdr" >&2
                exit 1
            fi
            ;;
        apt)
            if ! dpkg -s linux-headers-"$(uname -r)" >/dev/null 2>&1; then
                echo "fatal: kernel headers not installed." >&2
                echo "       Run: sudo apt install linux-headers-\$(uname -r)" >&2
                exit 1
            fi
            ;;
        dnf)
            if ! rpm -q kernel-devel-"$(uname -r)" >/dev/null 2>&1; then
                echo "fatal: kernel-devel not installed." >&2
                echo "       Run: sudo dnf install kernel-devel-\$(uname -r)" >&2
                exit 1
            fi
            ;;
        *)
            echo "fatal: unknown package manager; install kernel headers manually." >&2
            exit 1
            ;;
    esac
}

case "$action" in
    install)
        ensure_kernel_headers
        if ! command -v dkms >/dev/null 2>&1; then
            echo "fatal: dkms not installed." >&2
            echo "       Install it via your distro package manager (dkms on all major distros)." >&2
            exit 1
        fi
        echo "[install] building + DKMS-installing Hermes-KMS..."
        ( cd "$HK_SRC" && make dkms-install )
        echo "[install] loading hermes_kms with initial_enabled=1..."
        modprobe hermes_kms initial_enabled=1 || true
        echo "[install] done. Source selector should now show 'HERMES-1'."
        ;;
    load)
        modprobe hermes_kms initial_enabled=1 || true
        ;;
    uninstall)
        echo "[uninstall] removing Hermes-KMS via DKMS..."
        ( cd "$HK_SRC" && make dkms-uninstall ) || true
        rmmod hermes_kms 2>/dev/null || true
        echo "[uninstall] done."
        ;;
esac
