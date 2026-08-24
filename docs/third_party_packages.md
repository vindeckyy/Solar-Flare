# Third-Party Packages

This document covers **bundled dependencies** (git submodules, FFmpeg prebuilts,
Flatpak module pins) used when building SolarFlare from source, and **community
packages** of upstream Sunshine that do **not** include this fork.

---

## Bundled git submodules

Initialize before any CMake configure:

```bash
git submodule update --init --recursive
```

`scripts/linux-install.sh` verifies these paths and retries failed clones:

| Path | Upstream repository | Branch |
|------|---------------------|--------|
| `third-party/moonlight-common-c` | https://github.com/moonlight-stream/moonlight-common-c | `master` |
| `third-party/Simple-Web-Server` | https://github.com/LizardByte-infrastructure/Simple-Web-Server | `master` |
| `third-party/libdisplaydevice` | https://github.com/LizardByte/libdisplaydevice | `master` |
| `third-party/tray` | https://github.com/LizardByte/tray | `master` |
| `third-party/glad` | https://github.com/Dav1dde/glad | (default branch) |
| `third-party/nv-codec-headers` | https://github.com/FFmpeg/nv-codec-headers | `master` |
| `third-party/nanors` | https://github.com/sleepybishop/nanors | `master` |
| `third-party/wlr-protocols` | https://github.com/LizardByte-infrastructure/wlr-protocols | `master` |
| `third-party/wayland-protocols` | https://github.com/LizardByte-infrastructure/wayland-protocols | `main` |
| `third-party/doxyconfig` | https://github.com/LizardByte/doxyconfig | `master` |
| `third-party/build-deps` | https://github.com/LizardByte/build-deps | `master` |
| `third-party/lizardbyte-common` | https://github.com/LizardByte/lizardbyte-common | `master` |
| `third-party/hermes-kms` | https://github.com/MrOz59/Hermes-KMS | `main` |
| `packaging/linux/flatpak/deps/flatpak-builder-tools` | https://github.com/flatpak/flatpak-builder-tools | `master` |

Additional submodules (Windows/macOS / optional):

| Path | Repository | Branch |
|------|------------|--------|
| `third-party/googletest` | https://github.com/google/googletest | `main` |
| `third-party/inputtino` | https://github.com/games-on-whales/inputtino | `stable` |
| `third-party/nvapi` | https://github.com/NVIDIA/nvapi | `main` |
| `third-party/plasma-wayland-protocols` | https://github.com/KDE/plasma-wayland-protocols | `master` |
| `third-party/TPCircularBuffer` | https://github.com/michaeltyson/TPCircularBuffer | `master` |
| `third-party/ViGEmClient` | https://github.com/LizardByte/Virtual-Gamepad-Emulation-Client | `master` |

### Pinning and updating submodules

```bash
# Show current commit for build-deps (FFmpeg pin source)
git -C third-party/build-deps describe --tags --exact-match

# Update one submodule to upstream
git submodule update --remote third-party/glad

# Record new pin in parent repo
git add third-party/glad && git commit -m "chore: bump glad submodule"
```

**Failure recovery - submodules**

| Symptom | Fix |
|---------|-----|
| Empty directory under `third-party/` | `git submodule update --init --recursive -- <path>` |
| `Required submodule still missing` (installer) | Check GitHub access / proxy; clone URL uses HTTPS |
| CMake cannot find Wayland protocols | Ensure `wlr-protocols` and `wayland-protocols` are populated |
| Shallow CI clone missing tags | Script runs `git fetch --tags --depth=1` in `build-deps` |

---

## FFmpeg prebuilt binaries (`build-deps`)

SolarFlare links **static** FFmpeg libraries downloaded from
[LizardByte/build-deps](https://github.com/LizardByte/build-deps) GitHub Releases.
The release tag is read from the `third-party/build-deps` submodule
([`cmake/dependencies/ffmpeg.cmake`](../cmake/dependencies/ffmpeg.cmake)).

### Current pin (example)

Check your tree:

```bash
git -C third-party/build-deps describe --tags --exact-match
```

At documentation time the submodule pointed at **`v2026.713.132551`**. Archives follow
the naming pattern:

```
https://github.com/LizardByte/build-deps/releases/download/<TAG>/<OS>-<ARCH>-ffmpeg.tar.gz
```

| Platform | Archive name | SHA-256 (pinned in CMake) |
|----------|--------------|---------------------------|
| Linux x86_64 | `Linux-x86_64-ffmpeg.tar.gz` | `66512409857d7c11c18875193c098a5131baec060169c8f8e6397387e7a1af7d` |
| Linux aarch64 | `Linux-aarch64-ffmpeg.tar.gz` | `2bdcfa663bb7a1b241a47665c94aa288ef2ec40c6a212cc0a8ec63904b886c6d` |
| Linux ppc64le | `Linux-ppc64le-ffmpeg.tar.gz` | `61522f3424311154c6902fc1f427336eff084ff338c7b2d960cfa010183970f7` |
| Windows x86_64 | `Windows-x86_64-ffmpeg.tar.gz` | `6bf702af027d849f326823b9cfe058ddc3eff05d5e424624552bcb71c2415c68` |
| Windows arm64 | `Windows-arm64-ffmpeg.tar.gz` | `8cc219946f6bf45512612785e518814c22d0e73c8fa1235d7e84a795056c76c1` |
| macOS x86_64 | `Darwin-x86_64-ffmpeg.tar.gz` | `5b15f4283a2aa94d42abfd55e361cd4520021a7499fcbd693d3533f3ecb0904e` |
| macOS arm64 | `Darwin-arm64-ffmpeg.tar.gz` | `056122301edcdec74e00cfa9a3091bf3135d5fe1472234ce7f46426325081bca` |
| FreeBSD amd64 | `FreeBSD-amd64-ffmpeg.tar.gz` | `a4dee66179bd72221f83874beb95afd79ea70159782a53adff0579d494c9f0b3` |
| FreeBSD aarch64 | `FreeBSD-aarch64-ffmpeg.tar.gz` | `0adc7baead743be37ae66ff92c634764f5418fae3d5c5ea2ad4ec962cd45c3ce` |
| Alpine x86_64 | `Alpine-x86_64-ffmpeg.tar.gz` | `d2447166f2793917a2fec58ab969a176abe9bca114c92960177e86ee333ccf26` |
| Alpine aarch64 | `Alpine-aarch64-ffmpeg.tar.gz` | `fbcbed54baffa8ec7d722de44e76a2056d75cae1df045069867f058cf836ec91` |

Checksums are authoritative in `ffmpeg.cmake`; update both when bumping `build-deps`.

### Manual fetch

```bash
TAG="$(git -C third-party/build-deps describe --tags --exact-match)"
curl -LO "https://github.com/LizardByte/build-deps/releases/download/${TAG}/Linux-x86_64-ffmpeg.tar.gz"
tar -xzf Linux-x86_64-ffmpeg.tar.gz -C /tmp
cmake ... -DFFMPEG_PREPARED_BINARIES=/tmp/ffmpeg
```

CMake cache location (default download): `${CMAKE_BINARY_DIR}/_deps/ffmpeg/`.

**Failure recovery - FFmpeg**

| Symptom | Fix |
|---------|-----|
| `FFmpeg release tag is unavailable` | `git -C third-party/build-deps fetch --tags` |
| `Failed to download FFmpeg binaries` | Proxy/firewall; manual `curl` + extract |
| `No pinned FFmpeg checksum` | Unsupported CPU/OS; build on supported platform |
| `libavcodec.a` missing after extract | Re-delete `${CMAKE_BINARY_DIR}/_deps` and reconfigure |

### Flatpak FFmpeg pin

[`packaging/linux/flatpak/modules/ffmpeg.json`](../packaging/linux/flatpak/modules/ffmpeg.json)
may pin a **different** release (e.g. `v2026.516.30821`) with its own SHA-256.
Flatpak builds use `-DFFMPEG_PREPARED_BINARIES=/app/ffmpeg`.

---

## Node.js / Web UI dependencies

Root [`package-lock.json`](../package-lock.json) locks the observatory UI. Install
before CMake build:

```bash
npm install --no-audit --no-fund
```

Key runtime versions (see `package.json`):

| Package | Version |
|---------|---------|
| `vue` | 3.5.39 |
| `vite` | 6.4.3 |
| `bootstrap` | 5.3.8 |

Flatpak uses Node 20 SDK extension (`org.freedesktop.Sdk.Extension.node20` /
runtime `25.08`) and generates offline sources via `flatpak-node-generator`.

---

## Flatpak module versions (build-time)

Pinned in [`packaging/linux/flatpak/modules/`](../packaging/linux/flatpak/modules/):

| Module | Version / source |
|--------|------------------|
| CUDA | 13.2.0 / driver 595.45.04 ([`cuda.json`](../packaging/linux/flatpak/modules/cuda.json)) |
| FFmpeg prebuilt | build-deps release (see `ffmpeg.json`) |
| Boost | 1.89.0 ([`boost.json`](../packaging/linux/flatpak/modules/boost.json)) |
| nlohmann_json | 3.11.3 ([`nlohmann_json.json`](../packaging/linux/flatpak/modules/nlohmann_json.json)) |
| miniupnpc | 2.3.3 tag `miniupnpc_2_3_3` ([`miniupnpc.json`](../packaging/linux/flatpak/modules/miniupnpc.json)) |
| KDE Platform / SDK | 6.10 |
| Freedesktop SDK | 25.08 |

---

## Upstream CI tool versions (`linux_build.sh`)

Reference pins for Docker/CI packaging (not required for `linux-install.sh`):

| Tool | Version |
|------|---------|
| CMake (minimum) | 4.0.0 |
| CMake (bootstrap) | 4.3.0 |
| Doxygen | 1.10.0 – 1.12.0 (or compile 1.11.0) |
| CUDA (default runfile) | 13.1.1 / 590.48.01 |
| NVM (Ubuntu Node) | v0.40.3 |

---

## Community packages (upstream Sunshine only)

> [!WARNING]
> The links below install **third-party packages of upstream LizardByte Sunshine**,
> not SolarFlare. They do **not** include this fork's observatory UI, performance
> controls, or maintainer support. For SolarFlare, use
> [`scripts/linux-install.sh`](../scripts/linux-install.sh) or
> [release assets](https://github.com/vindeckyy/Solar-Flare/releases).

| Ecosystem | Link | Fetch / install |
|-----------|------|-----------------|
| Chocolatey | [community.chocolatey.org/packages/sunshine](https://community.chocolatey.org/packages/sunshine) | `choco install sunshine` |
| Scoop (extras) | [scoop.sh search](https://scoop.sh/#/apps?q=sunshine) | `scoop bucket add extras; scoop install sunshine` |
| nixpkgs | [package.nix](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/su/sunshine/package.nix) | `nix-env -iA nixos.sunshine` or Home Manager |
| Solus | [getsol/packages](https://github.com/getsolus/packages/tree/main/packages/s/sunshine) | `sudo eopkg install sunshine` |
| Flathub | [dev.lizardbyte.app.Sunshine](https://flathub.org/apps/dev.lizardbyte.app.Sunshine) | `flatpak install flathub dev.lizardbyte.app.Sunshine` |

Repology aggregate: https://repology.org/project/sunshine/versions

### Building a SolarFlare Flatpak locally

See [`packaging/linux/flatpak/README.md`](../packaging/linux/flatpak/README.md).
The in-tree manifest is inherited from upstream; published Flathub builds are
**not** SolarFlare.

---

## Python build tools (glad / Flatpak)

Glad may pull Python packages (e.g. Jinja2) via `uv`:

```bash
uv export --locked --only-group glad -o glad-requirements.txt
```

Flatpak CI runs `flatpak-pip-generator` to produce `glad-dependencies.json`.
Set `-DGLAD_SKIP_PIP_INSTALL=ON` when those deps are pre-staged (Flatpak).

<div class="section_buttons">

| Previous                      |                                            Next |
|:------------------------------|------------------------------------------------:|
| [Docker](../DOCKER_README.md) | [Gamestream Migration](gamestream_migration.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
