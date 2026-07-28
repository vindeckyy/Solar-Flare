#!/usr/bin/env bash
#
# scripts/linux-install.sh
#
# Build and install SolarFlare on a supported Linux host in one shot.
# Auto-detects Arch/CachyOS, Debian/Ubuntu, Fedora-family, openSUSE,
# Bazzite, and NixOS via /etc/os-release.
#
# This is the maintained SolarFlare user path. Do not confuse it with
# scripts/linux_build.sh, which is the inherited upstream Docker/CI
# packaging builder.
#
# What it does:
#   1. Verifies we're on Linux and detects the distribution.
#   2. Makes sure all submodules are fetched, even if a previous run
#      died mid-clone. Tolerates individual submodule failures
#      (e.g. transient network) and retries them.
#   3. Installs the build dependencies, or enters the Nix build shell.
#   4. Runs cmake with the SolarFlare fast-path flags (auto-detects
#      Zen 1/2/3/4/5 from /proc/cpuinfo, enables LTO, drops docs/tests).
#   5. Builds with ninja.
#   6. Runs `sudo cmake --install cmake-build-cachyos` and reloads the user
#      systemd manager.
#   7. Verifies that `sunshine` is on $PATH and prints a "what's
#      next" with the exact next commands.
#
# Usage:
#   ./scripts/linux-install.sh
#
# Re-runnable: cmake will reuse the existing build directory and only
# rebuild what changed. Submodule state is preserved between runs.
# The build directory remains cmake-build-cachyos for compatibility
# with existing checkouts.
#
# To force a clean rebuild:
#   ./scripts/linux-install.sh --clean
#
# To skip dependency setup (deps already installed) and just rebuild:
#   ./scripts/linux-install.sh --skip-deps
#   ./scripts/linux-install.sh --no-pacman   # legacy alias for --skip-deps
#
# To print the canonical detected distribution ID and exit:
#   ./scripts/linux-install.sh --print-distro-id
#
# Compatibility: scripts/cachyos-build.sh is a thin wrapper that execs
# this script with the same arguments.

set -uo pipefail   # NB: NOT -e. We want to keep going through non-fatal errors.

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/cmake-build-cachyos"
CLEAN=0
RUN_PACMAN=1
PRINT_DISTRO_ID=0

for arg in "$@"; do
  case "$arg" in
    --clean)    CLEAN=1 ;;
    --skip-deps|--no-pacman) RUN_PACMAN=0 ;;
    --print-distro-id) PRINT_DISTRO_ID=1 ;;
    -h|--help)
      sed -n '2,55p' "$0"
      exit 0
      ;;
    *) echo "Unknown arg: $arg" >&2; exit 2 ;;
  esac
done

# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------
say()  { printf '\033[1;36m[%s]\033[0m %s\n' "$(date +%H:%M:%S)" "$*"; }
warn() { printf '\033[1;33m[warn]\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31m[fatal]\033[0m %s\n' "$*" >&2; exit 1; }

step() {
  printf '\n'
  printf '\033[1;35m═══ %s ═══\033[0m\n' "$*"
  printf '\n'
}

# ---------------------------------------------------------------------------
# 1. Platform check
# ---------------------------------------------------------------------------
if [[ "$(uname -s)" != "Linux" ]]; then
  die "This script is for Linux. Sunshine builds on macOS and Windows too; use the upstream docs for those."
fi

## @brief Detect the canonical Linux distribution identifier.
## @return The lowercase distribution ID, or `unknown` when unavailable.
detect_distro() {
  local os_release_file="${SOLARFLARE_OS_RELEASE_FILE:-/etc/os-release}"
  if [[ -f "$os_release_file" ]]; then
    local ID=""
    local ID_LIKE=""
    # shellcheck disable=SC1090
    . "$os_release_file"
    if [[ "${ID:-}" == "nixos" ]] || [[ " ${ID_LIKE:-} " == *" nixos "* ]]; then
      echo nixos
    else
      echo "${ID:-unknown}"
    fi
  elif command -v lsb_release >/dev/null 2>&1; then
    lsb_release -is | tr '[:upper:]' '[:lower:]'
  else
    echo unknown
  fi
}
DISTRO_ID="$(detect_distro)"
if [[ "$PRINT_DISTRO_ID" -eq 1 ]]; then
  printf '%s\n' "$DISTRO_ID"
  exit 0
fi

step "1/7  Platform check"
OS_RELEASE_FILE="${SOLARFLARE_OS_RELEASE_FILE:-/etc/os-release}"
if [[ -f "$OS_RELEASE_FILE" ]]; then
  PRETTY_NAME=""
  # shellcheck disable=SC1090
  . "$OS_RELEASE_FILE"
fi
say "Linux: ✓ (${PRETTY_NAME:-unknown distribution})"
say "Detected distro: ${DISTRO_ID}"

IS_NIXOS=0
if [[ "$DISTRO_ID" == "nixos" ]]; then
  IS_NIXOS=1
  if [[ "$RUN_PACMAN" -eq 1 && "${SOLARFLARE_NIX_SHELL:-0}" != "1" ]]; then
    if ! command -v nix-shell >/dev/null 2>&1 || ! command -v nix-build >/dev/null 2>&1; then
      die "NixOS detected, but nix-shell or nix-build is unavailable. Enable Nix or re-run with --no-pacman inside a suitable development shell."
    fi

    nix_gc_root_dir="${XDG_DATA_HOME:-${HOME}/.local/share}/solarflare"
    mkdir -p "$nix_gc_root_dir"
    say "NixOS detected. Realizing and retaining the SolarFlare build environment."
    if ! nix-build "$REPO_ROOT/packaging/linux/nixos/shell.nix" \
      --out-link "$nix_gc_root_dir/build-environment" >/dev/null; then
      die "Could not realize the SolarFlare Nix build environment."
    fi

    say "Entering the reproducible SolarFlare build shell."
    quoted_command=""
    printf -v quoted_command '%q ' "$0" "$@"
    exec env SOLARFLARE_NIX_SHELL=1 nix-shell \
      "$REPO_ROOT/packaging/linux/nixos/shell.nix" \
      --run "$quoted_command"
  fi
fi

# Bazzite is Fedora-based but uses rpm-ostree instead of dnf.
# Flag it early so later steps know to use the right package manager.
IS_BAZZITE=0
if [[ "$DISTRO_ID" == "bazzite" ]] || [[ -f /run/ostree-booted ]]; then
  IS_BAZZITE=1
  say "Bazzite / rpm-ostree detected. Will use rpm-ostree for package installs."
  if ! command -v rpm-ostree >/dev/null 2>&1; then
    warn "Bazzite detected, but 'rpm-ostree' command not found. Falling back to dnf."
    IS_BAZZITE=0
  fi
fi

# ---------------------------------------------------------------------------
# 2. Submodules
# ---------------------------------------------------------------------------
step "2/7  Git submodules"

# Required submodules. If any of these are missing or empty, init.
REQUIRED_SUBS=(
  third-party/moonlight-common-c
  third-party/Simple-Web-Server
  third-party/libdisplaydevice
  third-party/tray
  third-party/glad
  third-party/nv-codec-headers
  third-party/nanors
  third-party/wlr-protocols
  third-party/wayland-protocols
  third-party/doxyconfig
  third-party/build-deps
  third-party/lizardbyte-common
)

missing=()
for sub in "${REQUIRED_SUBS[@]}"; do
  # Four conditions that mean "not actually populated":
  #   1. Directory doesn't exist at all.
  #   2. Directory is empty.
  #   3. Directory has only a .git FILE (a stub created by `git submodule init`;
  #      for a *populated* submodule .git is a directory containing the real repo).
  #   4. There's a broken .git stub from a failed clone — same fix.
  if [[ ! -d "${REPO_ROOT}/${sub}" ]] \
     || [[ -z "$(ls -A "${REPO_ROOT}/${sub}" 2>/dev/null)" ]] \
     || [[ -f "${REPO_ROOT}/${sub}/.git" ]]; then
    missing+=("$sub")
  fi
done

if [[ ${#missing[@]} -gt 0 ]]; then
  say "Missing submodules: ${missing[*]}"
  say "Running: git submodule update --init --recursive (this can take a few minutes)..."
  if ! git -C "${REPO_ROOT}" submodule update --init --recursive; then
    warn "submodule update exited non-zero. Retrying the missing ones individually..."
    for sub in "${missing[@]}"; do
      say "  retrying: $sub"
      git -C "${REPO_ROOT}" submodule update --init --recursive -- "$sub" || \
        warn "  $sub still failed. Build will probably break at the cmake step."
    done
  fi
else
  say "All required submodules look present."
fi

# Verify the critical ones actually have content.
for sub in third-party/moonlight-common-c third-party/Simple-Web-Server third-party/glad; do
  if [[ ! -d "${REPO_ROOT}/${sub}" ]] || [[ -z "$(ls -A "${REPO_ROOT}/${sub}" 2>/dev/null)" ]]; then
    die "Required submodule still missing after update: $sub. Check your network / GitHub access, then re-run."
  fi
done
say "Submodule check: ✓"

# ---------------------------------------------------------------------------
# 3. Build dependencies
# ---------------------------------------------------------------------------
step "3/7  Build dependencies"
if [[ "$RUN_PACMAN" -eq 0 ]]; then
  say "Skipping package install (--no-pacman)."
elif [[ "$IS_BAZZITE" -eq 1 ]]; then
  say "Bazzite uses rpm-ostree for package layering. This process is non-destructive but requires a reboot to take effect."
  say "Installing build dependencies..."
  warn "IMPORTANT: A reboot is REQUIRED after this step completes for the changes to apply."
  warn "Run 'systemctl reboot' after the script finishes."

  if ! rpm-ostree install --apply-live --allow-inactive \
        gcc-c++ cmake ninja-build git pkgconfig \
        openssl-devel libcurl-devel pulseaudio-libs-devel libdrm-devel libva-devel \
        libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel \
        libevdev-devel opus-devel \
        pipewire-devel libportal-devel \
        wayland-devel wayland-protocols-devel \
        systemd-devel libcap-devel \
        vulkan-devel glslang-devel \
        miniupnpc-devel nodejs npm 2>&1; then
    die "rpm-ostree install failed. Please check the error messages above. You may need to run it manually."
  fi
  say "Packages layered successfully. Please reboot your system now to continue the installation."
  die "REBOOT REQUIRED: Run 'systemctl reboot'"
else
  case "${DISTRO_ID}" in
    cachyos|arch|manjaro|endeavouros|arco|garuda)
      say "Installing build deps via pacman --needed (CachyOS/Arch path)..."
      if ! sudo pacman -S --needed --noconfirm \
            base-devel cmake ninja git \
            openssl curl libpulse libdrm libva \
            libx11 libxfixes libxrandr libxcb libxkbcommon \
            libevdev opus \
            libpipewire libportal \
            wayland wayland-protocols \
            systemd-libs libcap libnatpmp \
            vulkan-headers shaderc glslang \
            boost miniupnpc nlohmann-json \
            libpng libxext libxtst nodejs npm; then
        warn "pacman returned non-zero. Continuing. The build will tell us if anything's actually missing."
      fi
      ;;
    debian|ubuntu|pop|linuxmint|elementary|zorin|kali|mx)
      say "Installing build deps via apt (Debian/Ubuntu path)..."
      if ! sudo apt-get update; then
        warn "apt-get update failed; trying install anyway."
      fi
      if ! sudo apt-get install -y --no-install-recommends \
            build-essential cmake ninja-build git pkg-config \
            libssl-dev libcurl4-openssl-dev libpulse-dev libdrm-dev libva-dev \
            libx11-dev libxfixes-dev libxrandr-dev libxcb1-dev libxkbcommon-dev \
            libevdev-dev libopus-dev ffmpeg \
            libpipewire-0.3-dev libportal-dev \
            libwayland-dev wayland-protocols \
            libudev-dev libcap-dev libnatpmp-dev \
            vulkan-tools glslang-tools spirv-tools \
            libboost-all-dev libminiupnpc-dev nlohmann-json3-dev \
            libpng-dev libxext-dev libxtst-dev nodejs npm; then
        warn "apt-get returned non-zero. Continuing. The build will tell us if anything's actually missing."
      fi
      ;;
    fedora|nobara|rocky|almalinux|rhel|centos|bazzite)
      say "Installing build deps via dnf (Fedora/Nobara path)..."
      if ! sudo dnf install -y \
            gcc-c++ cmake ninja-build git pkgconfig \
            openssl-devel libcurl-devel pulseaudio-libs-devel libdrm-devel libva-devel \
            libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel \
            libevdev-devel opus-devel \
            pipewire-devel libportal-devel \
            wayland-devel wayland-protocols-devel \
            systemd-devel libcap-devel \
            vulkan-devel glslang-devel \
            miniupnpc-devel nodejs npm; then
        warn "dnf returned non-zero. Continuing. The build will tell us if anything's actually missing."
      fi
      ;;
    opensuse*|sles)
      say "Installing build deps via zypper (openSUSE path)..."
      if ! sudo zypper --non-interactive install \
            gcc-c++ cmake ninja git pkg-config \
            libopenssl-3-devel libcurl-devel libpulse-devel libdrm-devel libva-devel \
            libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel \
            libevdev-devel opus-devel ffmpeg-4-devel \
            pipewire-devel libportal-devel \
            wayland-devel wayland-protocols-devel \
            libudev-devel libcap-devel libnatpmp-devel \
            vulkan-devel shaderc glslang-devel \
            boost-devel libminiupnpc-devel nlohmann_json-devel \
            libpng-devel libXext-devel libXtst-devel nodejs npm; then
        warn "zypper returned non-zero. Continuing. The build will tell us if anything's actually missing."
      fi
      ;;
    nixos)
      if [[ "${SOLARFLARE_NIX_SHELL:-0}" == "1" ]]; then
        say "Build dependencies are provided by the Nix shell."
      else
        say "Skipping Nix dependency setup (--no-pacman)."
      fi
      ;;
    *)
      warn "Unknown distro '${DISTRO_ID}'. Skipping automatic package install."
      warn "Either install the packages manually and re-run with --no-pacman,"
      warn "or open an issue with your distro's package names."
      warn "See docs/PORTING.md for the package-name translation table."
      ;;
  esac
fi
say "Dependencies: assuming OK (build will fail loudly if not)."

# ---------------------------------------------------------------------------
# 3.5  Web UI dependencies
# ---------------------------------------------------------------------------
# The CMake `web-ui` target writes directly to
# cmake-build-cachyos/assets/web. Install dependencies here; the main CMake
# build invokes Vite with the correct destination later.
if command -v npm >/dev/null 2>&1; then
  step "4/7  Web UI dependencies"
  if [[ ! -d "$REPO_ROOT/node_modules" ]]; then
    say "Installing npm dependencies (one-time)..."
    (cd "$REPO_ROOT" && npm install --no-audit --no-fund 2>&1 | tail -5)
  fi
else
  die "npm not found. Install Node.js and npm so CMake can build the Web UI."
fi

# ---------------------------------------------------------------------------
# 4. CMake configure
# ---------------------------------------------------------------------------
step "5/7  CMake configure"

if [[ "$CLEAN" -eq 1 && -d "$BUILD_DIR" ]]; then
  say "Removing old build directory (--clean)..."
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
say "Running cmake..."

# Pick an available compiler launcher. Prefer ccache (massive rebuild speedup),
# otherwise let cmake call gcc/g++ directly.
_launcher=""
if command -v ccache >/dev/null 2>&1; then
  _launcher=ccache
else
  warn "ccache not found; falling back to direct compiler invocation."
fi

# Pick an available linker. Prefer mold (fast), then lld, then system ld.
# Hard-coding -fuse-ld=mold breaks the build on distros / CI runners
# without the mold package installed; binutils' ld is the universal fallback.
_linker_flag=""
if command -v mold >/dev/null 2>&1; then
  _linker_flag="-fuse-ld=mold"
elif command -v lld >/dev/null 2>&1; then
  _linker_flag="-fuse-ld=lld"
else
  warn "neither mold nor lld found; falling back to system ld (slow link)."
fi

cmake_platform_args=()
SOLARFLARE_INSTALL_PREFIX=""
if [[ "$IS_NIXOS" -eq 1 ]]; then
  SOLARFLARE_INSTALL_PREFIX="${HOME}/.local"
  mkdir -p "${HOME}/.config/systemd/user"
  cmake_platform_args=(
    "-DCMAKE_INSTALL_PREFIX=${SOLARFLARE_INSTALL_PREFIX}"
    "-DSUNSHINE_EXECUTABLE_PATH=${SOLARFLARE_INSTALL_PREFIX}/bin/sunshine"
    "-DUDEV_FOUND=ON"
    "-DUDEV_RULES_INSTALL_DIR=${SOLARFLARE_INSTALL_PREFIX}/lib/udev/rules.d"
    "-DSYSTEMD_FOUND=ON"
    "-DSYSTEMD_USER_UNIT_INSTALL_DIR=${HOME}/.config/systemd/user"
    "-DSYSTEMD_MODULES_LOAD_DIR=${SOLARFLARE_INSTALL_PREFIX}/lib/modules-load.d"
  )
  say "NixOS user-local install prefix: ${SOLARFLARE_INSTALL_PREFIX}"
fi

set +e
cmake -B "$BUILD_DIR" -G Ninja -S "$REPO_ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_DOCS=OFF \
    -DBUILD_TESTS=OFF \
    -DSUNSHINE_ENABLE_TRAY=OFF \
    -DSUNSHINE_ENABLE_CUDA=OFF \
    -DCUDA_FAIL_ON_MISSING=OFF \
    -DFFMPEG_PREBUILT=ON \
    -DCMAKE_CXX_COMPILER_LAUNCHER="${_launcher}" \
    -DCMAKE_C_COMPILER_LAUNCHER="${_launcher}" \
    -DCMAKE_EXE_LINKER_FLAGS="${_linker_flag}" \
    -DCMAKE_SHARED_LINKER_FLAGS="${_linker_flag}" \
    "${cmake_platform_args[@]}"
cmake_rc=$?
set -u

if [[ $cmake_rc -ne 0 ]]; then
  die "cmake configure failed (exit $cmake_rc). Look at the last 50 lines of output above for the actual error; usually a missing dep or a submodule not fully fetched."
fi
say "CMake configure: ✓"

# Show the user the CachyOS native flag line so they can confirm.
if grep -q "CachyOS native build" "$BUILD_DIR/CMakeFiles/CMakeOutput.log" 2>/dev/null; then
  say "Native flags engaged (look for the line above):"
  grep "CachyOS native build" "$BUILD_DIR/CMakeFiles/CMakeOutput.log" 2>/dev/null | head -1 | sed 's/^/    /'
else
  warn "Did not find 'CachyOS native build' in CMakeOutput.log. Microarch auto-detect may have failed."
  warn "Check: cat /proc/cpuinfo | grep 'model name' | head -1"
fi

# ---------------------------------------------------------------------------
# 5. Build
# ---------------------------------------------------------------------------
step "6/7  Build (ninja)"
say "Building with ninja (this takes 2-5 minutes on a Ryzen 5 4600H)..."
_jobs=$(nproc)
if [[ $_jobs -gt 8 ]]; then _jobs=6; fi
[[ $_jobs -lt 2 ]] && _jobs=2
say "Using -j$_jobs (capped at 6 to avoid OOM on 16GB with LTO)."
set +e
cmake --build "$BUILD_DIR" -j"$_jobs"
build_rc=$?
set -u

if [[ $build_rc -ne 0 ]]; then
  die "Build failed (exit $build_rc). Run 'cmake --build $BUILD_DIR' manually to see the full error."
fi
say "Build: ✓"

# ---------------------------------------------------------------------------
# 6. Install + final check
# ---------------------------------------------------------------------------
step "7/7  Install + verification"
if [[ "$IS_NIXOS" -eq 1 ]]; then
  say "cmake --install $BUILD_DIR..."
else
  say "sudo cmake --install $BUILD_DIR..."
fi

# If a previous install (e.g. from a distro package) set the immutable
# flag on any install-path file, cmake --install will fail with
# "Operation not permitted". Remove the flag before installing.
install_prefix="$(cmake -LA "$BUILD_DIR" 2>/dev/null | grep CMAKE_INSTALL_PREFIX | cut -d= -f2)"
if [[ "$IS_NIXOS" -eq 0 && -n "$install_prefix" && -d "$install_prefix" ]]; then
  say "Checking for immutable files under $install_prefix..."
  if command -v lsattr &>/dev/null; then
    immutable_files=$(sudo lsattr -R "$install_prefix" 2>/dev/null | grep '^....i' | sed 's/ *$//' || true)
    if [[ -n "$immutable_files" ]]; then
      warn "Found immutable files left over from a previous package install:"
      echo "$immutable_files" >&2
      say "Removing immutable flag (sudo chattr -R -i)..."
      sudo chattr -R -i "$install_prefix" 2>/dev/null || true
    fi
  fi
fi

set +e
if [[ "$IS_NIXOS" -eq 1 ]]; then
  cmake --install "$BUILD_DIR"
else
  sudo cmake --install "$BUILD_DIR"
fi
install_rc=$?
set -u

if [[ $install_rc -ne 0 ]]; then
  die "Install failed (exit $install_rc)."
fi

say "Reloading user systemd manager..."
systemctl --user daemon-reload 2>/dev/null || true

# ---------------------------------------------------------------------------
# Install the Solar-Flare fork's boot-time redesign services.
# These are the cpu-performance / nic-tuning / nvidia-clock-lock units that
# live in packaging/linux/redesign/. They probe hardware first and skip
# cleanly on boxes where the relevant sysfs / driver / GPU API is absent,
# so a fresh install on different hardware does not stall boot.
# ---------------------------------------------------------------------------
step "post-install  fork redesign services"
if [[ "$IS_NIXOS" -eq 1 ]]; then
  say "Skipping imperative system tuning services on NixOS."
  say "See docs/PORTING.md for the declarative NixOS host configuration."
elif [[ -x "$REPO_ROOT/packaging/linux/redesign/install-redesign-services.sh" ]]; then
  say "Installing fork redesign services..."
  if sudo "$REPO_ROOT/packaging/linux/redesign/install-redesign-services.sh"; then
    say "Fork redesign services installed and enabled."
  else
    warn "Fork redesign services install failed (non-fatal; you can rerun it manually later)."
  fi
else
  warn "install-redesign-services.sh missing — skipping."
fi

# ---------------------------------------------------------------------------
# Install the privileged self-update helper used by the Web UI Update now flow.
# cmake --install also places these when packaging; this covers source installs
# that may have an older tree without the install() rules yet.
# ---------------------------------------------------------------------------
step "post-install  self-update helper"
if [[ "$IS_NIXOS" -eq 1 ]]; then
  say "Skipping pkexec update helper on NixOS (user-writable ~/.local install)."
elif [[ -f "$REPO_ROOT/packaging/linux/solarflare-update-apply" ]]; then
  say "Installing solarflare-update-apply + polkit policy..."
  sudo install -d /usr/local/libexec
  sudo install -m 0755 "$REPO_ROOT/packaging/linux/solarflare-update-apply" /usr/local/libexec/solarflare-update-apply
  if [[ -f "$REPO_ROOT/packaging/linux/polkit/org.vindeckyy.solarflare.update.policy" ]]; then
    sudo install -d /usr/local/share/polkit-1/actions
    # Prefer the system actions dir when present.
    if [[ -d /usr/share/polkit-1/actions ]]; then
      sudo install -m 0644 "$REPO_ROOT/packaging/linux/polkit/org.vindeckyy.solarflare.update.policy" \
        /usr/share/polkit-1/actions/org.vindeckyy.solarflare.update.policy
    else
      sudo install -m 0644 "$REPO_ROOT/packaging/linux/polkit/org.vindeckyy.solarflare.update.policy" \
        /usr/local/share/polkit-1/actions/org.vindeckyy.solarflare.update.policy
    fi
  fi
  say "Self-update helper installed."
else
  warn "solarflare-update-apply missing; Web UI updates that need root will fail until it is installed."
fi

# ---------------------------------------------------------------------------
# Install the Hermes-KMS kernel module from third-party/hermes-kms.
# Builds + DKMS-installs hermes_kms.ko so the capture backend in
# src/platform/linux/hermes_kms.cpp can talk to /dev/dri/card*. Requires
# kernel headers (linux-zen-headers, linux-headers, etc.) — skipped if
# missing so the build still completes on CI / chroots.
# ---------------------------------------------------------------------------
step "post-install  Hermes-KMS kernel module"
if [[ "$IS_NIXOS" -eq 1 ]]; then
  say "Skipping the DKMS installer on NixOS. Kernel modules must be declared in the NixOS configuration."
elif [[ -x "$REPO_ROOT/packaging/linux/redesign/install-hermes-kms.sh" ]]; then
  say "Building + DKMS-installing Hermes-KMS from third-party/hermes-kms..."
  if sudo "$REPO_ROOT/packaging/linux/redesign/install-hermes-kms.sh"; then
    say "Hermes-KMS installed and loaded. 'HERMES-1' should now appear in the source selector."
  else
    warn "Hermes-KMS install failed (likely missing kernel-headers or dkms). Non-fatal."
    warn "  Fix later: install kernel-headers and rerun"
    warn "    sudo packaging/linux/redesign/install-hermes-kms.sh"
  fi
else
  warn "install-hermes-kms.sh missing — skipping."
fi

# Verify the binary is on $PATH.
if [[ "$IS_NIXOS" -eq 1 ]]; then
  export PATH="${SOLARFLARE_INSTALL_PREFIX}/bin:${PATH}"
fi
if command -v sunshine >/dev/null 2>&1; then
  say "Verified: $(command -v sunshine) is on PATH"
elif [[ "$IS_NIXOS" -eq 1 ]]; then
  warn "sunshine is not on \$PATH. Add ${SOLARFLARE_INSTALL_PREFIX}/bin to your shell's PATH."
else
  warn "sunshine is not on \$PATH. Check /usr/local/bin/sunshine (or wherever --install put it) and add it to your shell's PATH if needed."
fi

# ---------------------------------------------------------------------------
# Post-install SolarFlare fork verification.
# This catches a broken install (wrong binary, missing macro, accidental
# upstream build) before the user runs into it at runtime. Non-fatal: a
# missing/old binary on a non-standard install path is just a warning.
# ---------------------------------------------------------------------------
step "post-install  SolarFlare fork verification"
if command -v sunshine >/dev/null 2>&1; then
  _sf_bin="$(command -v sunshine)"

  # Check 1: fork banner must be present.
  if "$_sf_bin" --version 2>&1 | grep -q 'Fork: SolarFlare'; then
    say "Fork banner present (✓ SolarFlare fork branding intact)"
  else
    warn "Fork banner MISSING from $_sf_bin."
    warn "  Did you accidentally install an upstream LizardByte/Sunshine build?"
    warn "  Re-run ./scripts/linux-install.sh --clean to force a rebuild."
  fi

  # Check 2: representative fork config keys must parse without "Unrecognized".
  # Use a test config that exercises both bool + int parsing and
  # intentionally uses values INSIDE the documented ranges so that
  # the apply_config() int_between_f / bool_f helpers accept them
  # (out-of-range values are silently rejected and we'd see no
  # 'config: ...' line for the rejected key, which would fail check 3).
  _sf_tmpconf="$(mktemp -t sf-test.conf-XXXXXX)"
  cat > "$_sf_tmpconf" <<'SFEOF'
min_log_level = debug
busy_poll_us = 75
rate_cap_pct = 85
enet_4mib_buffer = false
pipewire_latency_ms = 12
cpu_pinning = false
SFEOF
  _sf_log="$(mktemp -t sf-test.log-XXXXXX)"
  # 1-second timeout so this doesn't hang on a misconfigured DRM/CUDA attempt.
  # Note: this script uses `set -uo pipefail` (no -e), so the legacy
  # `set +e` / `set -e` toggles are no-ops. We rely on the timeout
  # exit code and explicit checks below instead.
  timeout 1 "$_sf_bin" "$_sf_tmpconf" > "$_sf_log" 2>&1
  _sf_rc=$?
  if grep -qE "config: '(busy_poll_us|rate_cap_pct|enet_4mib_buffer|pipewire_latency_ms|cpu_pinning)'" "$_sf_log"; then
    say "Representative fork config keys parse cleanly (✓ recognised)"

    # Check 3: the parsed values must equal what we set. This catches
    # the case where the binary is older than the source and silently
    # falls back to the defaults (i.e. 'Unrecognized' is silent in some
    # error paths). The test config sets 5 specific values; if any of
    # them is missing from the log, apply_config() either rejected the
    # value or didn't see the key.
    _sf_problems=""
    for _sf_expected in 'busy_poll_us.*=.*75' 'rate_cap_pct.*=.*85' \
                        'pipewire_latency_ms.*=.*12'; do
      if ! grep -qE "$_sf_expected" "$_sf_log"; then
        _sf_problems="$_sf_problems $_sf_expected"
      fi
    done
    if [ -n "$_sf_problems" ]; then
      warn "Fork config keys parsed but some values are missing from the log:"
      warn "  Expected (regex):$_sf_problems"
      warn "  This usually means the binary is older than the source --"
      warn "  re-run ./scripts/linux-install.sh --clean to force a rebuild."
    fi
  else
    warn "Fork config keys did NOT appear in the parse log -- something is wrong."
    warn "  Expected 'config: ...' lines for busy_poll_us / rate_cap_pct /"
    warn "  enet_4mib_buffer / pipewire_latency_ms / cpu_pinning."
  fi
  if grep -iE 'Unrecognized.*(busy_poll_us|rate_cap_pct|enet_4mib_buffer|pipewire_latency_ms|cpu_pinning)' "$_sf_log" >/dev/null; then
    warn "Fork config keys logged as 'Unrecognized' -- the binary is older than the source."
  fi
  rm -f "$_sf_tmpconf" "$_sf_log"
else
  warn "sunshine not on \$PATH; skipping post-install fork verification."
fi

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
if [[ "$IS_NIXOS" -eq 1 ]]; then
  cat <<EOF

  NixOS host integration:

    1. Apply the uinput, video-group, and firewall settings from
       docs/PORTING.md with sudo nixos-rebuild switch.
    2. Ensure ${SOLARFLARE_INSTALL_PREFIX}/bin is in your login PATH.

  The generated service uses ${SOLARFLARE_INSTALL_PREFIX}/bin/sunshine.
EOF
fi

cat <<'EOF'

  ══════════════════════════════════════════════════════════════════════
   SolarFlare is installed.
  ══════════════════════════════════════════════════════════════════════

  Quick start:

    systemctl --user enable --now app-dev.lizardbyte.app.Sunshine.service
    # Or, for one-off runs without systemd:
    sunshine

  The web UI is at https://localhost:47990. Pair your Moonlight
  client using the PIN it prints on first run.

  If you hit "MESA-LOADER: failed to open" or "libnvidia-glcore.so"
  errors, your discrete NVIDIA GPU isn't set up for the right user;
  see https://wiki.cachyos.org/nvidia/ for the CachyOS NVIDIA guide.

  Fork tunables (edit ~/.config/sunshine/sunshine.conf directly):
    busy_poll_us        ENet SO_BUSY_POLL microseconds (0 disables, default 50)
    rate_cap_pct        rate-control pacer, % of link speed (50-95, default 80)
    enet_4mib_buffer    grow ENet UDP buffers to 4 MiB (default true)
    pipewire_latency_ms PW_KEY_NODE_LATENCY hint (1-40, default 8)
    cpu_pinning         SCHED_RR + core affinity for capture (default true)

  See docs/CONFIGURATION.md for ranges and behaviour.

  Tunable knobs for the build:
    ./scripts/linux-install.sh --clean      # nuke the build dir and start over
    ./scripts/linux-install.sh --skip-deps  # skip deps, just rebuild
EOF
