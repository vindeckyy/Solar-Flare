# Porting SolarFlare to other Linux distributions

The SolarFlare fork ([vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare))
is developed on CachyOS but targets any recent Arch-family, Debian/Ubuntu,
Fedora/openSUSE, Bazzite (rpm-ostree), or NixOS host. CMake patches (Zen 1–5
auto-detection, `-march`/`-mtune`/`-flto`/`-O3`, Linux-only source guards) work
everywhere; only **package names** and **post-install policy** differ.

[`scripts/linux-install.sh`](../scripts/linux-install.sh) auto-detects the distro
via `/etc/os-release` and installs the correct packages. This document is the
manual fallback when detection fails, plus distro-specific troubleshooting.

---

## Supported distribution IDs

Detected by `linux-install.sh` (`detect_distro()`):

| `$ID` / pattern | Package manager | Notes |
|-----------------|-----------------|-------|
| `cachyos`, `arch`, `manjaro`, `endeavouros`, `arco`, `garuda` | `pacman` | Primary development target |
| `debian`, `ubuntu`, `pop`, `linuxmint`, `elementary`, `zorin`, `kali`, `mx` | `apt` | GCC 13+ may need PPA on older releases |
| `fedora`, `nobara`, `rocky`, `almalinux`, `rhel`, `centos` | `dnf` | `ffmpeg-devel` needs RPM Fusion on Fedora |
| `bazzite` or `/run/ostree-booted` | `rpm-ostree` | **Reboot required** after layering |
| `opensuse*`, `sles` | `zypper` | Underscore package names (`nlohmann_json-devel`) |
| `nixos` or `ID_LIKE` contains `nixos` | Nix shell | User-local `~/.local` install |

Print your detected ID:

```bash
./scripts/linux-install.sh --print-distro-id
```

---

## Standard CMake configure (all distros)

```bash
git submodule update --init --recursive
npm install --no-audit --no-fund

cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUNSHINE_ENABLE_TRAY=OFF \
  -DSUNSHINE_ENABLE_CUDA=OFF \
  -DCUDA_FAIL_ON_MISSING=OFF \
  -DBUILD_DOCS=OFF \
  -DBUILD_TESTS=OFF \
  -DSUNSHINE_CACHYOS_NATIVE=ON

cmake --build cmake-build-release -j"$(nproc)"
sudo cmake --install cmake-build-release
```

| Flag | When to change |
|------|----------------|
| `-DSUNSHINE_CACHYOS_NATIVE=OFF` | Ship a generic binary or build on CI for heterogeneous CPUs |
| `-DSUNSHINE_ENABLE_CUDA=ON` | You have NVIDIA CUDA toolkit and want NVENC paths |
| `-DBUILD_TESTS=ON` | Development / regression testing |
| `CC=gcc-14 CXX=g++-14` | Distro default GCC is below 13 |

FFmpeg is fetched automatically from the pinned
[`third-party/build-deps`](https://github.com/LizardByte/build-deps) submodule tag
(see [Third-party packages](third_party_packages.md)). No separate `-DFFMPEG_*=ON`
flag is required for the default path.

Optional linker speedups (installer auto-detects):

```bash
# mold (fastest), then lld, else system ld
cmake ... -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold"
```

---

## Complete prerequisite matrices

The tables below list **exact package names** copied from
`scripts/linux-install.sh` step 3/7. Install all packages in a row unless noted.

### Arch / CachyOS / Manjaro / EndeavourOS / Arco / Garuda

```bash
sudo pacman -S --needed --noconfirm \
  base-devel cmake ninja git \
  openssl curl libpulse libdrm libva \
  libx11 libxfixes libxrandr libxcb libxkbcommon \
  libevdev opus \
  libpipewire libportal \
  wayland wayland-protocols \
  systemd-libs libcap libnatpmp \
  vulkan-headers shaderc glslang \
  boost miniupnpc nlohmann-json \
  libpng libxext libxtst nodejs npm
```

Optional for Hermes-KMS DKMS (post-install):

```bash
sudo pacman -S --needed dkms linux-headers   # or linux-zen-headers on CachyOS
```

### Debian / Ubuntu / Pop!_OS / Mint / elementary / Zorin / Kali / MX

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build git pkg-config \
  libssl-dev libcurl4-openssl-dev libpulse-dev libdrm-dev libva-dev \
  libx11-dev libxfixes-dev libxrandr-dev libxcb1-dev libxkbcommon-dev \
  libevdev-dev libopus-dev ffmpeg \
  libpipewire-0.3-dev libportal-dev \
  libwayland-dev wayland-protocols \
  libudev-dev libcap-dev libnatpmp-dev \
  vulkan-tools glslang-tools spirv-tools \
  libboost-all-dev libminiupnpc-dev nlohmann-json3-dev \
  libpng-dev libxext-dev libxtst-dev nodejs npm
```

> [!NOTE]
> On Ubuntu, PipeWire dev headers may be `libpipewire-dev` instead of
> `libpipewire-0.3-dev`. Do not install both.

### Fedora / Nobara / Rocky / Alma / RHEL / CentOS

```bash
# Fedora only - enable RPM Fusion first:
sudo dnf install -y \
  https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
  https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm

sudo dnf install -y \
  gcc-c++ cmake ninja-build git pkgconfig \
  openssl-devel libcurl-devel pulseaudio-libs-devel libdrm-devel libva-devel \
  libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel \
  libevdev-devel opus-devel ffmpeg-devel \
  pipewire-devel libportal-devel \
  wayland-devel wayland-protocols-devel \
  systemd-devel libcap-devel libnatpmp-devel \
  vulkan-devel glslang-devel spirv-tools-devel \
  boost-devel miniupnpc-devel json-devel \
  libpng-devel libXext-devel libXtst-devel nodejs npm
```

### openSUSE Tumbleweed / Leap

```bash
sudo zypper --non-interactive install \
  gcc-c++ cmake ninja git pkg-config \
  libopenssl-3-devel libcurl-devel libpulse-devel libdrm-devel libva-devel \
  libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel \
  libevdev-devel opus-devel ffmpeg-4-devel \
  pipewire-devel libportal-devel \
  wayland-devel wayland-protocols-devel \
  libudev-devel libcap-devel libnatpmp-devel \
  vulkan-devel shaderc glslang-devel \
  boost-devel libminiupnpc-devel nlohmann_json-devel \
  libpng-devel libXext-devel libXtst-devel nodejs npm
```

### Bazzite (rpm-ostree)

```bash
rpm-ostree install --apply-live --allow-inactive \
  gcc-c++ cmake ninja-build git pkgconfig \
  openssl-devel libcurl-devel pulseaudio-libs-devel libdrm-devel libva-devel \
  libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel \
  libevdev-devel opus-devel \
  pipewire-devel libportal-devel \
  wayland-devel wayland-protocols-devel \
  systemd-devel libcap-devel \
  vulkan-devel glslang-devel \
  miniupnpc-devel nodejs npm
sudo systemctl reboot   # required before building
```

### NixOS

Do not use `apt`/`pacman` on the host. The installer re-enters via:

```bash
./scripts/linux-install.sh
```

which realizes `packaging/linux/nixos/shell.nix` and installs to `~/.local`.
See [NixOS host configuration](#nixos) below.

---

## Package name translation (quick lookup)

### Build toolchain

| Purpose | Arch / CachyOS | Debian / Ubuntu | Fedora / Nobara | openSUSE |
|---------|----------------|-----------------|-----------------|----------|
| C/C++ toolchain | `base-devel` | `build-essential` | `gcc-c++` | `gcc-c++` |
| CMake | `cmake` | `cmake` | `cmake` | `cmake` |
| Ninja | `ninja` | `ninja-build` | `ninja-build` | `ninja` |
| Git | `git` | `git` | `git` | `git` |
| Node + npm | `nodejs npm` | `nodejs npm` | `nodejs npm` | `nodejs npm` |
| pkg-config | (in base-devel) | `pkg-config` | `pkgconfig` | `pkg-config` |

### Core libraries

| Purpose | Arch / CachyOS | Debian / Ubuntu | Fedora / Nobara | openSUSE |
|---------|----------------|-----------------|-----------------|----------|
| TLS | `openssl` | `libssl-dev` | `openssl-devel` | `libopenssl-3-devel` |
| HTTP | `curl` | `libcurl4-openssl-dev` | `libcurl-devel` | `libcurl-devel` |
| PulseAudio | `libpulse` | `libpulse-dev` | `pulseaudio-libs-devel` | `libpulse-devel` |
| DRM | `libdrm` | `libdrm-dev` | `libdrm-devel` | `libdrm-devel` |
| VA-API | `libva` | `libva-dev` | `libva-devel` | `libva-devel` |
| X11 stack | `libx11 libxfixes libxrandr libxcb libxkbcommon` | `libx11-dev libxfixes-dev libxrandr-dev libxcb1-dev libxkbcommon-dev` | `libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel` | same as Fedora column |
| evdev | `libevdev` | `libevdev-dev` | `libevdev-devel` | `libevdev-devel` |
| Opus | `opus` | `libopus-dev` | `opus-devel` | `opus-devel` |
| FFmpeg (headers) | `ffmpeg` | `ffmpeg` | `ffmpeg-devel` | `ffmpeg-4-devel` |

### Capture / streaming

| Purpose | Arch / CachyOS | Debian / Ubuntu | Fedora / Nobara | openSUSE |
|---------|----------------|-----------------|-----------------|----------|
| PipeWire | `libpipewire` | `libpipewire-0.3-dev` | `pipewire-devel` | `pipewire-devel` |
| XDG Portal | `libportal` | `libportal-dev` | `libportal-devel` | `libportal-devel` |
| Wayland | `wayland wayland-protocols` | `libwayland-dev wayland-protocols` | `wayland-devel wayland-protocols-devel` | `wayland-devel wayland-protocols-devel` |
| UDev | `systemd-libs` | `libudev-dev` | `systemd-devel` | `libudev-devel` |
| Capabilities | `libcap` | `libcap-dev` | `libcap-devel` | `libcap-devel` |
| NAT-PMP | `libnatpmp` | `libnatpmp-dev` | `libnatpmp-devel` | `libnatpmp-devel` |

### Graphics / JSON / misc

| Purpose | Arch / CachyOS | Debian / Ubuntu | Fedora / Nobara | openSUSE |
|---------|----------------|-----------------|-----------------|----------|
| Vulkan | `vulkan-headers` | `vulkan-tools` / SDK | `vulkan-devel` | `vulkan-devel` |
| glslang / SPIR-V | `shaderc glslang` | `glslang-tools spirv-tools` | `glslang-devel spirv-tools-devel` | `shaderc glslang-devel` |
| Boost | `boost` | `libboost-all-dev` | `boost-devel` | `boost-devel` |
| miniupnpc | `miniupnpc` | `libminiupnpc-dev` | `miniupnpc-devel` | `libminiupnpc-devel` |
| nlohmann-json | `nlohmann-json` | `nlohmann-json3-dev` | `json-devel` | `nlohmann_json-devel` |
| PNG | `libpng` | `libpng-dev` | `libpng-devel` | `libpng-devel` |
| Xext / Xtst | `libxext libxtst` | `libxext-dev libxtst-dev` | `libXext-devel libXtst-devel` | `libXext-devel libXtst-devel` |

---

## Distro-specific notes and failure recovery

### CachyOS / Arch family

**Why CachyOS?**

1. GCC 14+ and `-march=x86-64-v3` baseline match `SUNSHINE_CACHYOS_NATIVE` defaults.
2. BBRv3 kernel tuning benefits Wi-Fi streaming; on other kernels:
   `sudo sysctl -w net.ipv4.tcp_congestion_control=bbr`
3. PipeWire/Wayland versions align with bundled `wlr-protocols` / `wayland-protocols`
   submodules.

| Problem | Recovery |
|---------|----------|
| `pacman` keyring / sync errors | `sudo pacman -Syy archlinux-keyring && sudo pacman -Syu` |
| Conflicting `sunshine` package | `sudo pacman -Rns sunshine` before `cmake --install` |
| Hermes-KMS DKMS fails | Install `dkms` + `linux-headers`; rerun `packaging/linux/redesign/install-hermes-kms.sh` |
| OOM during LTO link | Re-run installer (caps jobs at 6) or `cmake --build ... -j2` |

### Debian 12 / Ubuntu 22.04+

| Problem | Recovery |
|---------|----------|
| GCC &lt; 13 | `sudo apt install gcc-13 g++-13` then `CC=gcc-13 CXX=g++-13 cmake ...` |
| Missing `<format>` / C++23 errors | Same as above; or enable `ppa:ubuntu-toolchain-r/test` |
| PipeWire header not found | Use `libpipewire-0.3-dev` (Debian) or `libpipewire-dev` (Ubuntu), not both |
| Submodule fetch blocked | Install `wayland-protocols` as fallback; fix network; retry `git submodule update` |

### Fedora 39+ / Nobara

| Problem | Recovery |
|---------|----------|
| `ffmpeg-devel` not found | Install RPM Fusion (see matrix above) |
| `pipewire-devel` missing protocols | Add `wireplumber-devel` on plain Fedora |
| SELinux denials on capture | Check `ausearch -m avc`; adjust booleans or use portal capture |

### openSUSE Tumbleweed / Leap 15.6+

| Problem | Recovery |
|---------|----------|
| Package not found (`nlohmann-json-devel`) | Use `nlohmann_json-devel` (underscore) |
| TCP slow on Wi-Fi | `sudo sysctl -w net.ipv4.tcp_congestion_control=bbr` and persist in `/etc/sysctl.d/99-solarflare.conf` |

### Steam Deck (SteamOS 3.x)

- Same packages as Arch; user `deck`, limited `/home/deck` space - need **≥ 4 GB** free for `cmake-build-*`.
- `pipewire-media-session` instead of WirePlumber: harmless log noise about missing `wireplumber`.

### Bazzite

| Problem | Recovery |
|---------|----------|
| Build deps missing after install script | Reboot after `rpm-ostree install` (script exits with reboot reminder) |
| `rpm-ostree` transaction failed | Run the install command manually; check `rpm-ostree status` |

### NixOS

The installer recognizes `ID=nixos` and re-enters through
`packaging/linux/nixos/shell.nix`, retaining a GC root at
`${XDG_DATA_HOME:-~/.local/share}/solarflare/build-environment`.

**User-local install** (default):

- Binary: `~/.local/bin/sunshine`
- User unit: `~/.config/systemd/user/app-dev.lizardbyte.app.Sunshine.service`
- Redesign services and Hermes-KMS **skipped** (declarative host config required)

**Host module** (replace `your-user`):

```nix
{
  hardware.uinput.enable = true;
  users.users.your-user.extraGroups = [ "input" "video" ];

  networking.firewall = {
    allowedTCPPorts = [ 47984 47989 47990 48010 ];
    allowedUDPPorts = [ 47998 47999 48000 48002 48010 ];
  };
}
```

Then:

```bash
sudo nixos-rebuild switch
./scripts/linux-install.sh
systemctl --user daemon-reload
systemctl --user enable --now app-dev.lizardbyte.app.Sunshine.service
```

DRM/KMS may need a declarative wrapper with `CAP_SYS_ADMIN` - do **not**
`setcap` binaries in the Nix store.

| Problem | Recovery |
|---------|----------|
| `nix-shell` unavailable | Enable flakes/Nix on host or use `--skip-deps` inside dev shell |
| Binary not on PATH | Add `~/.local/bin` to `environment.sessionVariables` or shell profile |

### Generic / unknown distro

```bash
./scripts/linux-install.sh --skip-deps
```

Install packages manually using the matrices above, then re-run.

---

## Post-install: capabilities, services, fork tuning

### `setcap` (KMS capture)

After `cmake --install`, verify:

```bash
getcap "$(command -v sunshine)"
# Expected: cap_sys_admin,cap_sys_nice=p
```

If empty:

```bash
sudo setcap 'cap_sys_admin,cap_sys_nice+p' "$(command -v sunshine)"
```

DEB/RPM packages run [`postinst`](../src_assets/linux/misc/postinst) automatically.
RPM spec uses `%caps` in [`packaging/linux/copr/Sunshine.spec`](../packaging/linux/copr/Sunshine.spec).

### Redesign boot-time services

Installed automatically by `linux-install.sh` (non-NixOS):

```bash
sudo ./packaging/linux/redesign/install-redesign-services.sh
```

See [`packaging/linux/redesign/README.md`](../packaging/linux/redesign/README.md).

### User service

```bash
systemctl --user enable --now app-dev.lizardbyte.app.Sunshine.service
```

---

## Verifying a successful port

```bash
systemctl --user --no-pager status app-dev.lizardbyte.app.Sunshine.service
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -n 50 --no-pager

curl -sS https://localhost:47990 -k -o /dev/null -w '%{http_code}\n'
# Expected: 401 (HTTPS UI up, auth required)

sunshine --version 2>&1 | grep 'Fork: SolarFlare'
strings "$(command -v sunshine)" | grep -m1 SolarFlare
```

If the fork banner is missing:

1. Confirm `cmake --install` ran after the SolarFlare build (not distro `sunshine` package).
2. Check commit/tag: `git describe --tags`.
3. Remove stale packages: `sudo pacman -Rns sunshine` (or equivalent).
4. Force clean rebuild: `./scripts/linux-install.sh --clean`.

---

## CI / Docker reference builder

[`scripts/linux_build.sh`](../scripts/linux_build.sh) is the **upstream packaging**
builder (Debian/Ubuntu/Fedora/Arch in Docker/CI). It is **not** the default
end-user path. It documents additional deps (Qt6, Doxygen, CUDA runfile install)
used for DEB/RPM/AppImage release artifacts.

Supported CI distros (from script): Arch, Debian 12/13, Fedora 42–45, Ubuntu
22.04–26.04. CUDA defaults: **13.1.1** / driver **590.48.01** (Flatpak module
uses **13.2.0** / **595.45.04**).
