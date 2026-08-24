# Flatpak packaging (upstream Sunshine compatibility)

<div align="center">
  <img src="https://raw.githubusercontent.com/vindeckyy/Solar-Flare/master/sunshine.png" alt="SolarFlare mark (compatibility filename sunshine.png)" />
  <h1 align="center">Sunshine Flatpak</h1>
  <h4 align="center">Self-hosted game stream host for Moonlight (Flatpak sandbox).</h4>
</div>

> [!WARNING]
> This directory holds an **inherited upstream Flatpak definition** for packaging
> experiments inside [vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare).
> The published [Flathub](https://flathub.org/apps/dev.lizardbyte.app.Sunshine)
> application is **upstream LizardByte Sunshine** - it does **not** include
> SolarFlare's observatory UI or fork-specific performance controls.
>
> **End users:** install SolarFlare via the [repository install guide](../../../README.md#install)
> (`./scripts/linux-install.sh`), not Flathub.

<div align="center">
  <a href="https://flathub.org/apps/dev.lizardbyte.app.Sunshine"><img src="https://img.shields.io/flathub/downloads/dev.lizardbyte.app.Sunshine?style=for-the-badge&logo=flathub" alt="Flathub installs"></a>
  <a href="https://flathub.org/apps/dev.lizardbyte.app.Sunshine"><img src="https://img.shields.io/flathub/v/dev.lizardbyte.app.Sunshine?style=for-the-badge&logo=flathub" alt="Flathub Version"></a>
</div>

Upstream Sunshine documentation:

* [Stable](https://docs.lizardbyte.dev/projects/sunshine/latest/)
* [Beta](https://docs.lizardbyte.dev/projects/sunshine/master/)

Report **non-fork-specific** Flatpak bugs upstream at
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine/issues).

---

## Contents of this directory

| Path | Purpose |
|------|---------|
| [`dev.lizardbyte.app.Sunshine.yml`](dev.lizardbyte.app.Sunshine.yml) | Manifest template (`@PROJECT_FQDN@`, `@GITHUB_COMMIT@`, …) |
| [`modules/`](modules/) | CUDA, FFmpeg, Boost, Avahi, miniupnpc, etc. |
| [`scripts/`](scripts/) | `sunshine.sh` wrapper, host integration helpers |
| [`apps.json`](apps.json) | Default application entries shipped in the sandbox |
| [`flathub.json`](flathub.json) | Flathub metadata sidecar |
| [`exceptions.json`](exceptions.json) | `flatpak-builder-lint` allowlist |
| [`deps/flatpak-builder-tools/`](deps/flatpak-builder-tools/) | Submodule: node/pip generators |

CMake substitutes placeholders when `-DSUNSHINE_CONFIGURE_FLATPAK_MAN=ON` is set
(see [Configure manifest](#1-configure-manifest)).

---

## Runtime overview

| Setting | Value |
|---------|-------|
| App ID | `dev.lizardbyte.app.Sunshine` |
| Runtime | `org.kde.Platform` **6.10** |
| SDK | `org.kde.Sdk` **6.10** |
| Node extension | `org.freedesktop.Sdk.Extension.node20` **25.08** |
| Command | `sunshine` |

Key `finish-args` (sandbox permissions): `--device=all`, `--share=network`,
`--socket=wayland`, `--socket=fallback-x11`, `--socket=pulseaudio`,
`--filesystem=home`, `--filesystem=xdg-run/pipewire-0`, Avahi + Flatpak D-Bus.

CMake flags inside the manifest module ([`dev.lizardbyte.app.Sunshine.yml`](dev.lizardbyte.app.Sunshine.yml)):

```
-DSUNSHINE_BUILD_FLATPAK=ON
-DSUNSHINE_ENABLE_CUDA=ON
-DSUNSHINE_ENABLE_DRM=ON
-DSUNSHINE_ENABLE_WAYLAND=ON
-DSUNSHINE_ENABLE_X11=ON
-DSUNSHINE_ENABLE_KWIN=ON
-DSUNSHINE_ENABLE_PORTAL=ON
-DFFMPEG_PREPARED_BINARIES=/app/ffmpeg
-DCMAKE_CUDA_COMPILER=/app/cuda/bin/nvcc
```

> [!NOTE]
> Flatpak builds **do not** use host `setcap`; KMS inside the sandbox follows
> Flatpak device/API rules. Host integration for uinput/udev uses
> [`scripts/additional-install.sh`](scripts/additional-install.sh).

---

## Prerequisites (local build)

Tested on Ubuntu 22.04+ (x86_64 or aarch64):

```bash
sudo apt-get update
sudo apt-get install -y cmake flatpak

# User Flathub remote
flatpak remote-add --if-not-exists --user flathub \
  https://flathub.org/repo/flathub.flatpakrepo

flatpak install --user -y flathub \
  org.flatpak.Builder \
  org.kde.Platform//6.10 \
  org.kde.Sdk//6.10 \
  org.freedesktop.Sdk.Extension.node20//25.08
```

Also required:

- **Python 3.14+** and **uv** (repo root `uv.lock`) for generators
- **Git submodules**: `git submodule update --init --recursive`
- **Disk space**: ≥ 30 GB free (CUDA module is large; CI caches between stages)

---

## Local build workflow

Mirror [`.github/workflows/ci-flatpak.yml`](../../../.github/workflows/ci-flatpak.yml).

### 1. Configure manifest

From repository root:

```bash
export BRANCH="$(git rev-parse --abbrev-ref HEAD)"
export BUILD_VERSION="$(git describe --tags --always)"
export COMMIT="$(git rev-parse HEAD)"
export CLONE_URL="https://github.com/vindeckyy/Solar-Flare.git"

mkdir -p build artifacts

cmake -DGITHUB_CLONE_URL="${CLONE_URL}" \
  -B build -S . \
  -DSUNSHINE_CONFIGURE_FLATPAK_MAN=ON \
  -DSUNSHINE_CONFIGURE_ONLY=ON
```

Output: `build/dev.lizardbyte.app.Sunshine.yml` (resolved placeholders).

### 2. Generate npm offline sources

```bash
uv sync --locked --only-group flatpak --no-install-project
uv run --locked --no-sync python -m flatpak_node_generator npm package-lock.json
# Creates generated-sources.json in repo root - copy to build/
cp generated-sources.json build/
```

### 3. Generate glad Python dependencies

```bash
uv export --locked --only-group glad --no-emit-project --no-emit-local \
  --no-header -o glad-requirements.txt

uv run --locked --no-sync python \
  ./packaging/linux/flatpak/deps/flatpak-builder-tools/pip/flatpak-pip-generator.py \
  --runtime="org.kde.Sdk//6.10" \
  --output glad-dependencies \
  --build-only \
  --requirements-file=glad-requirements.txt

cp glad-dependencies.json build/
cp package-lock.json build/
```

### 4. Build with flatpak-builder

CI uses a **two-stage** build to cache CUDA separately:

```bash
cd build
APP_ID=dev.lizardbyte.app.Sunshine
ARCH=x86_64   # or aarch64

# Stage 1: cache CUDA (optional but matches CI)
flatpak run org.flatpak.Builder \
  --arch="${ARCH}" \
  --force-clean \
  --repo=repo \
  --sandbox \
  --stop-at=cuda build-sunshine "${APP_ID}.yml"

# Stage 2: full build + tests
flatpak run org.flatpak.Builder \
  --arch="${ARCH}" \
  --force-clean \
  --repo=repo \
  --sandbox \
  build-sunshine "${APP_ID}.yml"
```

### 5. Create installable bundles

```bash
flatpak build-bundle --arch="${ARCH}" ./repo \
  ../artifacts/sunshine_${ARCH}.flatpak "${APP_ID}"

flatpak build-bundle --runtime --arch="${ARCH}" ./repo \
  ../artifacts/sunshine_debug_${ARCH}.flatpak "${APP_ID}.Debug"
```

### 6. Install locally

```bash
flatpak install --user ./artifacts/sunshine_x86_64.flatpak
```

First-run host setup (udev, uinput, user systemd unit):

```bash
flatpak run dev.lizardbyte.app.Sunshine additional-install.sh
```

Remove host changes:

```bash
flatpak run dev.lizardbyte.app.Sunshine remove-additional-install.sh
```

---

## Module reference

| Module | Version / URL |
|--------|----------------|
| **cuda** | [CUDA 13.2.0 / 595.45.04 runfile](modules/cuda.json) (x86_64 + aarch64 SBSA) |
| **ffmpeg-prebuilt** | [build-deps `Linux-*-ffmpeg.tar.gz`](modules/ffmpeg.json) |
| **boost** | 1.89.0 |
| **nlohmann_json** | 3.11.3 |
| **miniupnpc** | 2.3.3 |
| **libevdev**, **numactl**, **avahi** | See `modules/*.json` |
| **xvfb** | Test-only (headless gtest in manifest `test-commands`) |

CUDA install uses `--no-opengl-libs --no-drm`; only the toolkit is retained.

---

## Linting

```bash
cd build
flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
  --exceptions \
  --user-exceptions ../packaging/linux/flatpak/exceptions.json \
  manifest dev.lizardbyte.app.Sunshine.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
  --exceptions \
  --user-exceptions ../packaging/linux/flatpak/exceptions.json \
  repo repo
```

---

## Failure recovery

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `org.kde.Platform//6.10` not found | Flathub remote missing / wrong arch | `flatpak install flathub org.kde.Platform//6.10` |
| `generated-sources.json` missing | Skipped node generator | Run `flatpak_node_generator` (step 2) |
| CUDA run silent failure | Driver/toolkit mismatch in builder | Read `/tmp/cuda-installer.log` inside build log |
| OOM during link | LTO + large CUDA | Use CI `--stop-at=cuda` cache pattern; add swap |
| `flatpak-builder-lint` failures | New permission or CPE | Update `exceptions.json` with justification |
| Tests fail in sandbox | xvfb / npm serve | Manifest runs `npm run serve & xvfb-run tests/test_sunshine` |
| Wrong commit built | Stale `COMMIT` env | Re-run CMake configure with current `git rev-parse HEAD` |

---

## Flathub submission archive

CI (x86_64 only) packs `flathub.tar.gz` containing:

- Resolved `dev.lizardbyte.app.Sunshine.yml`
- `generated-sources.json`, `glad-dependencies.json`, `package-lock.json`
- `modules/`, `flathub.json`, this README

Downstream Flathub PR workflows must still initialize git submodules in the
published repo.

---

## SolarFlare fork note

To ship **SolarFlare** as a Flatpak you would need to:

1. Change `SUNSHINE_PUBLISHER_*` and fork branding in the manifest `config-opts`.
2. Use a distinct `app-id` (Flathub policy for forks).
3. Re-run lint with updated `exceptions.json`.

The maintained fork install path remains **native** via
[`scripts/linux-install.sh`](../../../scripts/linux-install.sh), including redesign
services and Hermes-KMS documented in
[`packaging/linux/redesign/README.md`](../redesign/README.md).
