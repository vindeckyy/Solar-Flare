# Porting the SolarFlare fork to other distros

The SolarFlare fork ([vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare))
is developed against CachyOS but is portable to any Arch-family or
Debian/Ubuntu/Fedora/openSUSE-based distro. The CMake / build-system
patches (Zen 1/2/3/4 auto-detection, `-march`/`-mtune`/`-flto`/`-O3`,
Linux-only `__linux__`-guarded source patches) work everywhere; only
the package names change between distros.

`scripts/cachyos-build.sh` already auto-detects the distro via
`/etc/os-release` and installs the right package set. This document
covers the manual fallback when the script's auto-detection doesn't
match your `$ID`, plus the conceptual background for each
distro-specific gotcha.

## Why CachyOS specifically?

Three things stack here:

1. **CachyOS ships GCC 14+ with `-march=x86-64-v3` baselines.** Older
   toolchains (Debian 12 ships GCC 12) can still build, but won't
   produce the AVX2/BMI2/FMA code paths that the CachyOS build of
   Sunshine targets. You can build on Debian 12; you just won't get
   the inliner unrolling the BGR->NV12 color-conversion loop.
2. **CachyOS's BBRv3 + CachyOS-tuned kernel** gives better congestion
   behaviour on Wi-Fi 7 out of the box than mainline kernels. If you
   switch to a generic distro kernel, run `sudo sysctl -w
   net.ipv4.tcp_congestion_control=bbr` after install to compensate.
3. **CachyOS packages** `pipewire`, `wayland`, `wlroots`, etc. with the
   patches and protocol versions Sunshine expects. On other distros you
   may need to also `git submodule update --init --recursive` to make
   sure the bundled `wlr-protocols` and `wayland-protocols` submodules
   take precedence.

The CMake flag set is the same on every distro:

```bash
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSUNSHINE_ENABLE_TRAY=OFF \
    -DSUNSHINE_ENABLE_DBUS=OFF \
    -DBUILD_DOCS=OFF \
    -DBUILD_TESTS=OFF \
    -DFFMPEG_PREBUILT=ON
cmake --build build
sudo cmake --install build
```

The optional `-DSUNSHINE_CACHYOS_NATIVE=OFF` switch disables the Zen
auto-detect, `-march=x86-64-v3`, `-flto`, `-fno-plt` set in
`cmake/compile_definitions/common.cmake`. Use it when you're building
on one machine but shipping the binary to another.

## Package name translation

The columns below mirror the case blocks in
`scripts/cachyos-build.sh` step 3/7.

### Build toolchain

| Purpose      | Arch / CachyOS | Debian / Ubuntu | Fedora / Nobara | openSUSE |
|--------------|----------------|-----------------|-----------------|----------|
| C/C++        | `base-devel`   | `build-essential` | `gcc-c++`     | `gcc-c++` |
| CMake        | `cmake`        | `cmake`         | `cmake`         | `cmake`  |
| Ninja        | `ninja`        | `ninja-build`   | `ninja-build`   | `ninja`  |
| Git          | `git`          | `git`           | `git`           | `git`    |
| NodeJS       | `nodejs npm`   | `nodejs npm`    | `nodejs npm`    | `nodejs npm` |

### Core libraries

| Purpose              | Arch / CachyOS        | Debian / Ubuntu                | Fedora / Nobara              | openSUSE                |
|----------------------|-----------------------|--------------------------------|------------------------------|-------------------------|
| TLS                  | `openssl`             | `libssl-dev`                   | `openssl-devel`              | `libopenssl-3-devel`    |
| HTTP                 | `curl`                | `libcurl4-openssl-dev`         | `libcurl-devel`              | `libcurl-devel`         |
| PulseAudio           | `libpulse`            | `libpulse-dev`                 | `pulseaudio-libs-devel`      | `libpulse-devel`        |
| DRM                  | `libdrm`              | `libdrm-dev`                   | `libdrm-devel`               | `libdrm-devel`          |
| VA-API               | `libva`               | `libva-dev`                    | `libva-devel`                | `libva-devel`           |
| X11 / Xfixes / Xrandr| `libx11 libxfixes libxrandr libxcb libxkbcommon` | `libx11-dev libxfixes-dev libxrandr-dev libxcb1-dev libxkbcommon-dev` | `libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel` | `libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel` |
| evdev                | `libevdev`            | `libevdev-dev`                 | `libevdev-devel`             | `libevdev-devel`        |
| Opus                 | `opus`                | `libopus-dev`                  | `opus-devel`                 | `opus-devel`            |
| FFmpeg               | `ffmpeg`              | `ffmpeg`                       | `ffmpeg-devel`               | `ffmpeg-4-devel`        |

### Capture / streaming

| Purpose              | Arch / CachyOS | Debian / Ubuntu | Fedora / Nobara | openSUSE |
|----------------------|----------------|-----------------|-----------------|----------|
| PipeWire             | `libpipewire`  | `libpipewire-0.3-dev` | `pipewire-devel` | `pipewire-devel` |
| XDG Desktop Portal   | `libportal`    | `libportal-dev` | `libportal-devel` | `libportal-devel` |
| Wayland              | `wayland wayland-protocols` | `libwayland-dev wayland-protocols` | `wayland-devel wayland-protocols-devel` | `wayland-devel wayland-protocols-devel` |
| UDev                 | `libudev`      | `libudev-dev`   | `systemd-devel` | `libudev-devel` |
| Capabilities         | `libcap`       | `libcap-dev`    | `libcap-devel`  | `libcap-devel` |
| NAT-PMP              | `libnatpmp`    | `libnatpmp-dev` | `libnatpmp-devel` | `libnatpmp-devel` |

### Graphics / shader

| Purpose              | Arch / CachyOS        | Debian / Ubuntu                | Fedora / Nobara              | openSUSE                |
|----------------------|-----------------------|--------------------------------|------------------------------|-------------------------|
| Vulkan headers       | `vulkan-headers`      | `vulkan-headers` (or `vulkan-sdk` package) | `vulkan-devel`    | `vulkan-devel`          |
| glslang              | `glslang shaderc`     | `glslang-tools spirv-tools`    | `glslang-devel spirv-tools` | `shaderc glslang-devel` |
| Boost                | `boost`               | `libboost-all-dev`             | `boost-devel`                | `boost-devel`           |
| miniupnpc            | `miniupnpc`           | `libminiupnpc-dev`             | `miniupnpc-devel`            | `libminiupnpc-devel`    |
| nlohmann-json        | `nlohmann-json`       | `nlohmann-json3-dev`           | `json-devel`                 | `nlohmann_json-devel`   |
| PNG                  | `libpng`              | `libpng-dev`                   | `libpng-devel`               | `libpng-devel`          |
| Xext / Xtst          | `libxext libxtst`     | `libxext-dev libxtst-dev`      | `libXext-devel libXtst-devel` | `libXext-devel libXtst-devel` |

## Distro-specific gotchas

### Debian 12 / Ubuntu 22.04

- **GCC 12 vs GCC 14.** Sunshine's source requires GCC 13+ for
  `<format>` and a few of the C++23 features in `src/stream.cpp` and
  `src/audio.cpp`. The `cachyos-build.sh` defaults to the system
  compiler. If `gcc --version` reports anything below 13, install
  `gcc-13 g++-13` and either `update-alternatives` to point at it or
  pass `CC=gcc-13 CXX=g++-13` to the `cmake -B build` line.
- **PipeWire 0.3 dev headers** are split out as
  `libpipewire-0.3-dev` on Debian; on Ubuntu they're inside
  `libpipewire-dev`. Don't install both.
- **Wayland protocols**. Sunshine pulls in `wlr-protocols` and
  `wayland-protocols` as git submodules under `third-party/`, and the
  CMake build prefers those over the system copies. You can install
  the distro copy (`wayland-protocols` on Debian) as a no-op fallback
  in case a submodule fetch fails.

### Fedora 39+ / Nobara 39+

- `rpm-fusion` is required for the ffmpeg-devel headers on plain Fedora.
  `sudo dnf install https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm`
  before the `dnf install ... ffmpeg-devel` line.
- Nobara ships `pipewire-devel` directly; Fedora needs
  `pipewire-devel` + `wireplumber-devel` for the protocol buffer
  pieces.

### openSUSE Tumbleweed / Leap 15.6+

- `zypper` package names use `_` instead of `-` in some cases
  (`nlohmann_json-devel` not `nlohmann-json-devel`).
- The Tumbleweed default kernel has BBR but the Congestion Control
  sysctl is `cubic`. After install, run `sudo sysctl -w
  net.ipv4.tcp_congestion_control=bbr` and add it to
  `/etc/sysctl.d/99-sunshine.conf` to make it stick.

### Steam Deck (SteamOS 3.x, Arch-derived)

- Same package names as CachyOS but the default user is `deck` and
  `/home/deck` is on a small SSD; the `cmake-build-` directory needs
  at least 4 GB of free space.
- SteamOS uses `pipewire-media-session` not `wireplumber`; either
  works but you'll see harmless debug logs about the missing
  `wireplumber` service in `~/.config/sunshine/sunshine.log`.

### Generic / non-pacman distros

If your distro has none of the above package managers, the safest
approach is to use the `--no-pacman` flag in `cachyos-build.sh` and
install the dependencies manually. Look at the
`scripts/cachyos-build.sh` step 3/7 case statement for the canonical
list; it's the same on every distro, just with the distro-specific
package names above.

## Verifying a successful port

After `cmake --install build`:

```bash
sunshine --version
# Expected: a few info-level 'config: ...' lines, no errors, exit 0.

sunshine --help
# Expected: usage block, exit 0.

curl -sS https://localhost:47990 -k -o /dev/null -w '%{http_code}\n'
# Expected: 200 (after `systemctl --user start sunshine` and the
# initial PIN prompt).
```

If any of the five SolarFlare-specific tunables doesn't appear in
`sunshine --version` (with `min_log_level = 1` set), check that:

1. The binary at `/usr/local/bin/sunshine` was installed after the
   `cmake --install` step (not the upstream-distro package).
2. The fork source actually contains the keys: `grep -c solarflare_t
   src/config.h` should print `1`.

If both check out but the keys still don't appear, you're probably
running a stale install from `pacman -S sunshine` upstream. Uninstall
that first: `sudo pacman -Rns sunshine` (or distro equivalent).