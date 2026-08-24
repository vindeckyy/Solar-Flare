# Building

> [!NOTE]
> This document retains inherited cross-platform build details. SolarFlare
> build directories use the `cmake-build-` prefix; the maintained Linux path is
> documented in [Porting SolarFlare](PORTING.md).
Sunshine binaries are built using [CMake](https://cmake.org). The tree
requires CMake ≥ 3.20 (`CMakeLists.txt`). The upstream Docker/CI builder
(`scripts/linux_build.sh`) expects CMake ≥ 4.0.0. Prefer a current CMake
from your distro.

## Building Locally

### Compiler
It is recommended to use one of the following compilers:

| Compiler    | Version |
|:------------|:--------|
| GCC         | 14+     |
| Clang       | 17+     |
| Apple Clang | 15+     |

### Dependencies

#### FreeBSD
> [!CAUTION]
> Sunshine support for FreeBSD is experimental and may be incomplete or not work as expected

##### Install dependencies
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
  ports-mgmt/pkg \
  security/openssl \
  shells/bash \
  www/npm-node22 \
  x11/libX11 \
  x11/libxcb \
  x11/libXfixes \
  x11/libXrandr \
  x11/libXtst
```

#### Linux
Dependencies vary depending on the distribution. The maintained SolarFlare
installer is [`scripts/linux-install.sh`](../scripts/linux-install.sh); see
[Porting SolarFlare](PORTING.md) for per-distro package translation.
`scripts/cachyos-build.sh` is a compatibility wrapper for that installer.
Upstream's
[linux_build.sh](../scripts/linux_build.sh)
remains a useful reference for
inherited Debian-, Fedora-, and Arch-family dependency lists used by Docker/CI
packaging. It is not the default end-user install path.

##### KMS Capture
If you are using KMS, patching the Sunshine binary with `setcap` is required. Some post-install scripts handle this. If building
from source and using the binary directly, this will also work:

```bash
sudo cp cmake-build-release/sunshine /tmp
sudo setcap cap_sys_admin,cap_sys_nice+p /tmp/sunshine
sudo getcap /tmp/sunshine
sudo mv /tmp/sunshine cmake-build-release/sunshine
```

##### CUDA Toolkit
Sunshine requires CUDA Toolkit for NVFBC capture. There are two caveats to CUDA:

1. The version installed depends on the version of GCC.
2. The version of CUDA you use will determine compatibility with various GPU generations.
   At the time of writing, match the defaults in `scripts/linux_build.sh`
   and [Getting Started](getting_started.md): CUDA 13.1.1 with driver
   590.48.01 (or newer compatible). See
   [CUDA compatibility](https://docs.nvidia.com/deploy/cuda-compatibility/index.html)
   for GPU generation coverage.

> [!NOTE]
> To install older versions, select the appropriate run file based on your desired CUDA version and architecture
> according to [CUDA Toolkit Archive](https://developer.nvidia.com/cuda-toolkit-archive)

#### macOS
You can either use [Homebrew](https://brew.sh) or [MacPorts](https://www.macports.org) to install dependencies.

##### Homebrew
```bash
dependencies=(
  "boost"  # Optional
  "cmake"
  "doxygen"  # Optional, for docs
  "graphviz"  # Optional, for docs
  "icu4c"  # Optional, if boost is not installed
  "miniupnpc"
  "ninja"
  "node"
  "openssl@3"
  "opus"
  "pkg-config"
)
brew install "${dependencies[@]}"
```

If there are issues with an SSL header that is not found:

@tabs{
  @tab{ Intel | ```bash
    ln -s /usr/local/opt/openssl/include/openssl /usr/local/include/openssl
    ```}
  @tab{ Apple Silicon | ```bash
    ln -s /opt/homebrew/opt/openssl/include/openssl /opt/homebrew/include/openssl
    ```
  }
}

##### MacPorts
```bash
dependencies=(
  "cmake"
  "curl"
  "doxygen"  # Optional, for docs
  "graphviz"  # Optional, for docs
  "libopus"
  "miniupnpc"
  "ninja"
  "npm9"
  "pkgconfig"
)
sudo port install "${dependencies[@]}"
```

#### Windows

> [!WARNING]
> Cross-compilation is not supported on Windows. You must build on the target architecture.

First, you need to install [MSYS2](https://www.msys2.org).

For AMD64 startup "MSYS2 UCRT64" (or for ARM64 startup "MSYS2 CLANGARM64") then execute the following commands.

##### Update all packages
```bash
pacman -Syu
```

##### Set toolchain variable
For UCRT64:
```bash
export TOOLCHAIN="ucrt-x86_64"
```

For CLANGARM64:
```bash
export TOOLCHAIN="clang-aarch64"
```

##### Install dependencies
```bash
dependencies=(
  "git"
  "mingw-w64-${TOOLCHAIN}-boost"  # Optional
  "mingw-w64-${TOOLCHAIN}-cmake"
  "mingw-w64-${TOOLCHAIN}-cppwinrt"
  "mingw-w64-${TOOLCHAIN}-curl-winssl"
  "mingw-w64-${TOOLCHAIN}-doxygen"  # Optional, for docs... better to install official Doxygen
  "mingw-w64-${TOOLCHAIN}-graphviz"  # Optional, for docs
  "mingw-w64-${TOOLCHAIN}-miniupnpc"
  "mingw-w64-${TOOLCHAIN}-onevpl"
  "mingw-w64-${TOOLCHAIN}-openssl"
  "mingw-w64-${TOOLCHAIN}-opus"
  "mingw-w64-${TOOLCHAIN}-toolchain"
)
if [[ "${MSYSTEM}" == "UCRT64" ]]; then
  dependencies+=(
    "mingw-w64-${TOOLCHAIN}-MinHook"
    "mingw-w64-${TOOLCHAIN}-nodejs"
    "mingw-w64-${TOOLCHAIN}-nsis"
  )
fi
pacman -S "${dependencies[@]}"
```

To create a WiX installer, you also need to install [.NET](https://dotnet.microsoft.com/download).

For ARM64: To build frontend, you also need to install [Node.JS](https://nodejs.org/en/download)

### Clone
Ensure [git](https://git-scm.com) is installed on your system, then clone the repository using the following command:

```bash
git clone --recursive https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
mkdir cmake-build-release
```

### Build

```bash
cmake -B cmake-build-release -G Ninja -S .
ninja -C cmake-build-release -j2
```

> [!TIP]
> Available build options live in this tree's
> [`cmake/prep/options.cmake`](../cmake/prep/options.cmake). SolarFlare also
> documents `SUNSHINE_CACHYOS_NATIVE` and related Linux flags in
> [Porting SolarFlare](PORTING.md).

### Package

@tabs{
  @tab{FreeBSD | @tabs{
    @tab{pkg | ```bash
      cpack -G FREEBSD --config ./cmake-build-release/CPackConfig.cmake
      ```}
  }}
  @tab{Linux | @tabs{
    @tab{deb | ```bash
      cpack -G DEB --config ./cmake-build-release/CPackConfig.cmake
      ```}
    @tab{rpm | ```bash
      cpack -G RPM --config ./cmake-build-release/CPackConfig.cmake
      ```}
  }}
  @tab{macOS | @tabs{
    @tab{DragNDrop | ```bash
      cpack -G DragNDrop --config ./cmake-build-release/CPackConfig.cmake
      ```}
  }}
  @tab{Windows | @tabs{
    @tab{NSIS Installer | ```bash
      cpack -G NSIS --config ./cmake-build-release/CPackConfig.cmake
      ```}
    @tab{WiX Installer | ```bash
      cpack -G WIX --config ./cmake-build-release/CPackConfig.cmake
      ```}
    @tab{Portable | ```bash
      cpack -G ZIP --config ./cmake-build-release/CPackConfig.cmake
      ```}
  }}
}

### Remote Build
It may be beneficial to build remotely in some cases. This will enable easier building on different operating systems.

1. Fork the project
2. Activate workflows
3. Trigger the *CI* workflow manually
4. Download the artifacts/binaries from the workflow run summary

> [!IMPORTANT]
> CI workflow artifacts are not SolarFlare release packages. End users should
> install with `./scripts/linux-install.sh` or update an existing install from
> the published GitHub release assets documented in the repository README.

<div class="section_buttons">

| Previous                              |                            Next |
|:--------------------------------------|--------------------------------:|
| [Troubleshooting](troubleshooting.md) | [Contributing](contributing.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
