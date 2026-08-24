# Building

> [!NOTE]
> SolarFlare build directories **must** use the `cmake-build-` prefix (for example
> `cmake-build-release`, `cmake-build-tests`). The maintained Linux install path is
> [`scripts/linux-install.sh`](../scripts/linux-install.sh); see
> [Porting SolarFlare](PORTING.md) for per-distro package names and manual fallbacks.

SolarFlare (Sunshine binary name) is built with [CMake](https://cmake.org) ≥ 3.20
(`CMakeLists.txt`). The upstream Docker/CI builder
([`scripts/linux_build.sh`](../scripts/linux_build.sh)) additionally expects CMake ≥
4.0.0 and may bootstrap CMake 4.3.0 when the distro package is too old. Prefer a
current CMake from your distribution or from [Kitware](https://cmake.org/download/).

## Quick reference

| Platform | Recommended path | Build directory |
|----------|------------------|-----------------|
| Linux (end user) | `./scripts/linux-install.sh` | `cmake-build-cachyos` (installer default) |
| Linux (manual / packaging) | CMake + Ninja (this document) | `cmake-build-release` |
| macOS | [`scripts/macos_build.sh`](../scripts/macos_build.sh) or manual CMake | `build` (script default) or `cmake-build-release` |
| Windows | MSYS2 UCRT64 / CLANGARM64 shell | `cmake-build-release` |
| FreeBSD | Manual CMake + `pkg` deps | `cmake-build-release` |

## Compiler requirements

| Compiler    | Minimum version | Notes |
|:------------|:--------------|:------|
| GCC         | 13+ (14+ recommended) | Required for C++23 (`<format>`, etc.). CI uses GCC 14 on most targets. |
| Clang       | 17+ | Supported on Linux and FreeBSD. |
| Apple Clang | 15+ | Xcode toolchain on macOS. |
| MinGW-w64   | UCRT64 or CLANGARM64 | Windows only; must build on the target architecture (no cross-compile). |

## CMake configuration options

All options are declared in [`cmake/prep/options.cmake`](../cmake/prep/options.cmake).
SolarFlare-specific compile flags live in
[`cmake/compile_definitions/common.cmake`](../cmake/compile_definitions/common.cmake).

### Global options

| Option | Default | Purpose |
|--------|---------|---------|
| `BUILD_DOCS` | `ON` | Build Doxygen documentation. Set `OFF` for faster dev builds. |
| `BUILD_TESTS` | `ON` | Build `test_sunshine` and enable CTest. |
| `BUILD_WERROR` | `OFF` | Treat warnings as errors (`-Werror`). CI sets `ON`. |
| `ENABLE_COVERAGE` | `OFF` | Enable gcov instrumentation for tests. |
| `NPM_OFFLINE` | `OFF` | Use offline npm cache only (Flatpak / reproducible builds). |
| `SUNSHINE_CONFIGURE_ONLY` | `OFF` | Generate packaging manifests only, then exit. |
| `SOLARFLARE_FORK` | `ON` | SolarFlare branding in version banner (see `cmake/prep/build_version.cmake`). |
| `BOOST_USE_STATIC` | `ON` (Linux), `OFF` (macOS) | Link Boost statically vs shared. |
| `CUDA_FAIL_ON_MISSING` | `ON` | Fail configure when CUDA is enabled but not found. |
| `CUDA_INHERIT_COMPILE_OPTIONS` | `ON` | Pass host CXX flags into NVCC. |
| `SUNSHINE_ENABLE_TRAY` | `ON` | Build system tray (Qt). SolarFlare installer sets `OFF`. |
| `SUNSHINE_SYSTEM_VULKAN_HEADERS` | `OFF` | Use system Vulkan headers instead of submodule. |
| `SUNSHINE_SYSTEM_WAYLAND_PROTOCOLS` | `OFF` | Use system Wayland protocols instead of submodule. |

### Publisher metadata (cache strings)

| Variable | Default in tree |
|----------|-----------------|
| `SUNSHINE_PUBLISHER_NAME` | `Third Party Publisher` |
| `SUNSHINE_PUBLISHER_WEBSITE` | `https://github.com/vindeckyy/Solar-Flare` |
| `SUNSHINE_PUBLISHER_ISSUE_URL` | `https://github.com/vindeckyy/Solar-Flare/issues` |

### Linux-only options

| Option | Default | Purpose |
|--------|---------|---------|
| `SUNSHINE_CACHYOS_NATIVE` | `ON` (auto on Linux) | Zen microarch detection, `-O3`, LTO, `-march` tuning. |
| `SUNSHINE_ENABLE_CUDA` | `ON` | NVENC / CUDA capture paths. Installer sets `OFF`. |
| `SUNSHINE_ENABLE_DRM` | `ON` | KMS/DRM screen capture. |
| `SUNSHINE_ENABLE_VAAPI` | `ON` | VA-API encode paths. |
| `SUNSHINE_ENABLE_VULKAN` | `ON` | Vulkan video encoding. |
| `SUNSHINE_ENABLE_WAYLAND` | `ON` | Wayland capture. |
| `SUNSHINE_ENABLE_X11` | `ON` | X11 capture. |
| `SUNSHINE_ENABLE_KWIN` | `ON` | KWin ScreenCast portal. |
| `SUNSHINE_ENABLE_PORTAL` | `ON` | XDG Desktop Portal capture. |
| `SUNSHINE_BUILD_APPIMAGE` | `OFF` | AppImage layout and rules. |
| `SUNSHINE_BUILD_FLATPAK` | `OFF` | Flatpak layout (no host setcap). |
| `SUNSHINE_BUILD_HOMEBREW` | `OFF` | Homebrew formula install paths. |
| `SUNSHINE_CONFIGURE_PKGBUILD` | `OFF` | Generate AUR files only. |
| `SUNSHINE_CONFIGURE_FLATPAK_MAN` | `OFF` | Generate Flatpak manifest only. |

### FFmpeg prebuilt binaries

When `FFMPEG_PREPARED_BINARIES` is **not** set, CMake downloads pinned FFmpeg
static libraries from the
[`third-party/build-deps`](https://github.com/LizardByte/build-deps) submodule tag
(see [Third-party packages](third_party_packages.md)). Override with:

```bash
-DFFMPEG_PREPARED_BINARIES=/path/to/extracted/ffmpeg
```

> [!TIP]
> `scripts/linux-install.sh` passes `-DFFMPEG_PREBUILT=ON` for historical
> compatibility; the effective mechanism is the automatic `build-deps` tag lookup
> in [`cmake/dependencies/ffmpeg.cmake`](../cmake/dependencies/ffmpeg.cmake).

### Recommended SolarFlare Linux configure

Matches [`scripts/linux-install.sh`](../scripts/linux-install.sh):

```bash
git submodule update --init --recursive
npm install --no-audit --no-fund

cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_DOCS=OFF \
  -DBUILD_TESTS=OFF \
  -DSUNSHINE_ENABLE_TRAY=OFF \
  -DSUNSHINE_ENABLE_CUDA=OFF \
  -DCUDA_FAIL_ON_MISSING=OFF \
  -DSUNSHINE_CACHYOS_NATIVE=ON

cmake --build cmake-build-release -j"$(nproc)"
sudo cmake --install cmake-build-release
```

For a generic binary (build on one machine, run on another):

```bash
cmake ... -DSUNSHINE_CACHYOS_NATIVE=OFF
```

### Test build

```bash
cmake -S . -B cmake-build-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=OFF
cmake --build cmake-build-tests --target test_sunshine -j"$(nproc)"
./cmake-build-tests/tests/test_sunshine --gtest_brief=1
```

---

## Linux

### End-user install (recommended)

```bash
git clone --recursive https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/linux-install.sh
```

| Flag | Effect |
|------|--------|
| `--clean` | Remove `cmake-build-cachyos` and reconfigure from scratch. |
| `--skip-deps` / `--no-pacman` | Skip package manager step; only rebuild. |
| `--print-distro-id` | Print detected `/etc/os-release` ID and exit. |

The installer also runs post-install steps: redesign systemd units, Hermes-KMS
DKMS (when headers exist), polkit update helper, and fork verification.

Per-distro **exact package names** are documented in [Porting SolarFlare](PORTING.md).

### KMS capture and `setcap`

DRM/KMS capture requires Linux capabilities on the `sunshine` binary:

```bash
sudo setcap 'cap_sys_admin,cap_sys_nice+p' /usr/local/bin/sunshine
getcap /usr/local/bin/sunshine
```

`cmake --install` on a normal Linux install runs
[`src_assets/linux/misc/postinst`](../src_assets/linux/misc/postinst) via CPack,
which applies the same capabilities. AppImage and Flatpak builds use different
paths (see their READMEs). If `getcap` shows nothing after install:

1. Install `libcap2-bin` (Debian) / ensure `libcap` (Arch) is present.
2. Re-run postinst manually: `sudo sh src_assets/linux/misc/postinst`
3. Or apply setcap directly to the installed binary (see above).

**Failure recovery - KMS / permission denied**

| Symptom | Fix |
|---------|-----|
| `Permission denied` opening `/dev/dri/card*` | Apply setcap; add user to `video` group; log out/in. |
| `Operation not permitted` during `cmake --install` | Previous package set immutable flag: `sudo chattr -R -i /usr/local` then reinstall. |
| Wrong GPU / Mesa errors | See [Troubleshooting](troubleshooting.md); verify NVIDIA PRIME or single-GPU setup. |

### CUDA (optional, NVFBC / NVENC)

SolarFlare's default installer disables CUDA (`-DSUNSHINE_ENABLE_CUDA=OFF`) because
most Linux users rely on VA-API/Vulkan. To build with CUDA:

1. Install [NVIDIA driver](https://www.nvidia.com/drivers) and CUDA Toolkit matching your GCC.
2. CI defaults (`linux_build.sh`): CUDA **13.1.1**, driver build **590.48.01**.
3. Configure with `-DSUNSHINE_ENABLE_CUDA=ON` and `-DCMAKE_CUDA_COMPILER=$(which nvcc)`.

Ubuntu 26.04+ may use `cuda-toolkit-13-1` system packages; older distros often use
the [CUDA runfile](https://developer.nvidia.com/cuda-toolkit-archive). See
[glibc/CUDA patches](../packaging/linux/patches/) if NVCC fails on `math_functions.h`.

---

## Windows

> [!WARNING]
> Cross-compilation is **not** supported. Build on the same architecture you deploy
> (AMD64 in UCRT64, ARM64 in CLANGARM64).

### Prerequisites

1. Install [MSYS2](https://www.msys2.org).
2. Open **MSYS2 UCRT64** (x86_64) or **MSYS2 CLANGARM64** (aarch64).
3. On Windows, prefix every build command with the MSYS2 launcher (from repo root):

```bat
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "<command>"
```

Replace `ucrt64` with `clangarm64` for ARM64 builds.

### Update and toolchain

```bash
pacman -Syu
```

UCRT64 (x86_64):

```bash
export TOOLCHAIN="ucrt-x86_64"
```

CLANGARM64:

```bash
export TOOLCHAIN="clang-aarch64"
```

### Install dependencies

```bash
dependencies=(
  "git"
  "mingw-w64-${TOOLCHAIN}-boost"
  "mingw-w64-${TOOLCHAIN}-cmake"
  "mingw-w64-${TOOLCHAIN}-cppwinrt"
  "mingw-w64-${TOOLCHAIN}-curl-winssl"
  "mingw-w64-${TOOLCHAIN}-miniupnpc"
  "mingw-w64-${TOOLCHAIN}-nlohmann-json"
  "mingw-w64-${TOOLCHAIN}-onevpl"
  "mingw-w64-${TOOLCHAIN}-openssl"
  "mingw-w64-${TOOLCHAIN}-opus"
  "mingw-w64-${TOOLCHAIN}-toolchain"
)
if [[ "${MSYSTEM}" == "UCRT64" ]]; then
  dependencies+=(
    "mingw-w64-${TOOLCHAIN}-MinHook"
    "mingw-w64-${TOOLCHAIN}-nsis"
  )
fi
pacman -S "${dependencies[@]}"
```

Additional requirements:

- **Node.js** (LTS): install from [nodejs.org](https://nodejs.org/) and add to `PATH`
  (CI uses Windows Node outside MSYS2 for the Web UI).
- **WiX installer**: install [.NET SDK](https://dotnet.microsoft.com/download) 10.x.
- **Doxygen** (optional docs): CI uses standalone Doxygen 1.11.0 installer because
  the MSYS2 build interacts poorly with Graphviz.

### Clone and build

```bash
git clone --recursive https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
npm install

mkdir -p cmake-build-release
cmake -B cmake-build-release -G Ninja -S . \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_WERROR=ON \
  -DSUNSHINE_ASSETS_DIR=assets
ninja -C cmake-build-release
```

### Package

```bash
cpack -G NSIS --config ./cmake-build-release/CPackConfig.cmake   # installer
cpack -G WIX  --config ./cmake-build-release/CPackConfig.cmake   # MSI (needs .NET)
cpack -G ZIP  --config ./cmake-build-release/CPackConfig.cmake   # portable
```

**Failure recovery - Windows**

| Symptom | Fix |
|---------|-----|
| `pacman -Syu` hangs on file locks | Close all MSYS2 windows; retry; reboot if needed. |
| CMake cannot find OpenSSL / Opus | Ensure `TOOLCHAIN` matches your MSYS2 environment (`echo $MSYSTEM`). |
| Web UI / `npm` errors | Run `npm install` from repo root; verify `node -v` in the same shell used for Ninja. |
| Missing `VCRUNTIME` at runtime | Use NSIS/ZIP from CPack; do not copy `sunshine.exe` without bundled DLLs from `cmake-build-release`. |
| ARM64: no NSIS | Expected; use ZIP or build frontend with external Node per upstream CI notes. |

---

## macOS

Build with **Homebrew** (recommended) or **MacPorts**. Apple Silicon and Intel use
different OpenSSL paths.

### Homebrew dependencies

```bash
brew install cmake doxygen graphviz node pkgconf icu4c@78 miniupnpc openssl@3 opus llvm
```

Optional: `boost` (CMake can fetch/build if omitted).

If CMake reports missing OpenSSL headers:

@tabs{
  @tab{ Intel | ```bash
    ln -s /usr/local/opt/openssl/include/openssl /usr/local/include/openssl
    ```}
  @tab{ Apple Silicon | ```bash
    ln -s /opt/homebrew/opt/openssl/include/openssl /opt/homebrew/include/openssl
    ```
  }
}

### MacPorts dependencies

```bash
sudo port install cmake curl doxygen graphviz libopus miniupnpc ninja npm9 pkgconfig
```

### Manual CMake build

```bash
git submodule update --init --recursive
npm install

cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_DOCS=OFF \
  -DBUILD_TESTS=OFF \
  -DICU_ROOT="$(brew --prefix icu4c@78)" \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DOpus_ROOT_DIR="$(brew --prefix opus)" \
  -DSUNSHINE_ENABLE_TRAY=ON

cmake --build cmake-build-release -j"$(sysctl -n hw.ncpu)"
```

### Release script (signed .dmg)

[`scripts/macos_build.sh`](../scripts/macos_build.sh) automates deps, configure,
build, and `cpack -G DragNDrop`. Signing requires `APPLE_CODESIGN_IDENTITY` and a
notarytool keychain profile `notarytool-password`.

```bash
./scripts/macos_build.sh --skip-tests
./scripts/macos_build.sh --skip-codesign   # local unsigned build
```

**Failure recovery - macOS**

| Symptom | Fix |
|---------|-----|
| `openssl/ssl.h` not found | Set `OPENSSL_ROOT_DIR` to `brew --prefix openssl@3`. |
| Codesign / notarize failure | Use `--skip-codesign` for dev builds; verify Developer ID cert in Keychain. |
| Tray icon missing at runtime | Ensure `-DSUNSHINE_ENABLE_TRAY=ON` and Qt is linked (default in `macos_build.sh`). |

---

## FreeBSD

> [!CAUTION]
> FreeBSD support is experimental and may be incomplete.

### Install dependencies

```sh
pkg install -y \
  audio/opus \
  audio/pulseaudio \
  devel/cmake \
  devel/evdev-proto \
  devel/git \
  devel/libevdev \
  devel/libnotify \
  devel/ninja \
  devel/pkgconf \
  devel/qt6-base \
  ftp/curl \
  graphics/libdrm \
  graphics/qt6-svg \
  graphics/wayland \
  multimedia/libva \
  net/miniupnpc \
  security/openssl \
  shells/bash \
  www/npm-node22 \
  x11/libX11 \
  x11/libxcb \
  x11/libXfixes \
  x11/libXrandr \
  x11/libXtst
```

### Build and package

```bash
git submodule update --init --recursive
npm install

cmake -B cmake-build-release -G Ninja -S .
ninja -C cmake-build-release -j2
cpack -G FREEBSD --config ./cmake-build-release/CPackConfig.cmake
```

---

## Packaging (all platforms)

@tabs{
  @tab{Linux deb/rpm | ```bash
    cpack -G DEB --config ./cmake-build-release/CPackConfig.cmake
    cpack -G RPM --config ./cmake-build-release/CPackConfig.cmake
    ```}
  @tab{macOS | ```bash
    cpack -G DragNDrop --config ./cmake-build-release/CPackConfig.cmake
    ```}
  @tab{Windows | ```bash
    cpack -G NSIS --config ./cmake-build-release/CPackConfig.cmake
    cpack -G WIX  --config ./cmake-build-release/CPackConfig.cmake
    cpack -G ZIP  --config ./cmake-build-release/CPackConfig.cmake
    ```}
  @tab{FreeBSD | ```bash
    cpack -G FREEBSD --config ./cmake-build-release/CPackConfig.cmake
    ```}
}

Flatpak and AppImage workflows are documented under
[`packaging/linux/flatpak/README.md`](../packaging/linux/flatpak/README.md).

---

## Remote / CI builds

To obtain binaries without a local toolchain:

1. Fork the repository and enable GitHub Actions.
2. Trigger the **CI** workflow manually.
3. Download artifacts from the workflow run summary.

> [!IMPORTANT]
> CI artifacts are **not** official SolarFlare release packages. End users should
> install via `./scripts/linux-install.sh` or published GitHub release assets
> (see repository README).

---

## Common build failures (all platforms)

| Failure | Likely cause | Recovery |
|---------|--------------|----------|
| Empty `third-party/*` at configure | Submodules not initialized | `git submodule update --init --recursive` |
| `FFmpeg release tag is unavailable` | `build-deps` submodule not at a tag | `cd third-party/build-deps && git fetch --tags` |
| `No pinned FFmpeg checksum` | Unsupported arch/OS combo | Build on x86_64/aarch64 Linux, Windows, macOS, or FreeBSD amd64 |
| Doxygen / doc target fails | Missing `doxygen` or `graphviz` | `-DBUILD_DOCS=OFF` |
| Link OOM during LTO | `-flto` on low RAM | `-DSUNSHINE_CACHYOS_NATIVE=OFF` or reduce `-j` |
| `ninja: error: loading 'build.ninja'` | Wrong build dir | Use consistent `-B cmake-build-release` |

<div class="section_buttons">

| Previous                              |                            Next |
|:--------------------------------------|--------------------------------:|
| [Troubleshooting](troubleshooting.md) | [Contributing](contributing.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
