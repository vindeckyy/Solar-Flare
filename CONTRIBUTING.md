# Contributing to SolarFlare

Thanks for your interest in the SolarFlare fork of
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine). This
document covers the fork's contribution policy. For day-to-day
development workflow (building, testing, code style, etc.) please
read the upstream
[docs/contributing.md](docs/contributing.md) — most of what applies
to Sunshine applies to the fork too.

## TL;DR

- **Small fixes that only touch fork-specific files** (the 5 fork
  config keys, the CachyOS native build flags, the `scripts/cachyos-build.sh`
  multi-distro logic, the SolarFlare web UI rebrand, `docs/PORTING.md`,
  `docs/CONFIGURATION.md`, `docs/CHANGELOG-SolarFlare.md`,
  `tests/unit/test_config_fork_keys.cpp`): open a PR on this fork.
- **Anything that touches upstream code that we haven't modified**
  (`src/crypto.cpp`, `src/audio.cpp`, `src/nvenc/*`, etc.): open a
  PR on [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine)
  first, then optionally cherry-pick onto this fork.
- **Anything that touches the upstream UI** (`src_assets/...`):
  check that the change doesn't undo the fork's SolarFlare rebrand
  before opening a PR upstream; re-apply the fork branding after the
  upstream merge.

## What this fork IS

SolarFlare is a derivative of upstream LizardByte/Sunshine focused on
zero-latency local-LAN game streaming on CachyOS x86_64 + GNOME/Wayland
+ PipeWire + an NVIDIA Turing (or any modern) GPU. The fork's primary
contributions are:

1. **Zen microarchitecture auto-detection + LTO + native flags** in
   `cmake/compile_definitions/common.cmake`, gated by the cmake option
   `SUNSHINE_CACHYOS_NATIVE` (default ON on Linux).
2. **Five fork-specific Linux-only tunables** that can be set in
   `~/.config/sunshine/sunshine.conf`:
   - `busy_poll_us`       — `SO_BUSY_POLL` on the ENet socket
   - `rate_cap_pct`       — rate-control pacer, % of link speed
   - `enet_4mib_buffer`   — grow ENet UDP buffers to 4 MiB
   - `pipewire_latency_ms`— `PW_KEY_NODE_LATENCY` hint to compositor
   - `cpu_pinning`        — `SCHED_RR` + core affinity for capture
   See [docs/CONFIGURATION.md](docs/CONFIGURATION.md) for ranges and
   behaviour.
3. **A multi-distro build script** (`scripts/cachyos-build.sh`) that
   auto-detects Arch / Debian / Fedora / openSUSE families and
   installs the right package set.
4. **A web UI rebrand** — fork logo, fork wordmark, fork footer,
   fork version banner, "SolarFlare" theme — see
   `src_assets/common/assets/web/Navbar.vue` and
   `src_assets/common/assets/web/index.html`.
5. **Fork-specific docs**: `docs/PORTING.md` (multi-distro build),
   `docs/CONFIGURATION.md` (fork config keys),
   `docs/CHANGELOG-SolarFlare.md` (fork changelog), and a new
   `## SolarFlare Fork` section appended to `docs/configuration.md`.
6. **Fork-specific CI** (`.github/workflows/ci-solarflare.yml`) that
   runs `scripts/cachyos-build.sh` on an Arch container, verifies the
   fork banner + 5 config keys, and runs the full test suite.

## What this fork IS NOT

- **Not a general-purpose Sunshine fork**. If you want a Sunshine
  build for a non-Linux distro, non-Zen CPU, or non-LAN target, the
  upstream build (or your distro's Sunshine package) is what you
  want.
- **Not a security-focused fork**. We cherry-pick upstream security
  fixes when they don't conflict with our modifications, but we
  don't track CVEs ourselves. For the latest security patches,
  check upstream.
- **Not a long-term support branch**. The fork tracks upstream's
  `master` branch. There's no `release/X` branches here.

## How to submit a PR

1. Fork this repo on GitHub.
2. Create a branch from `master` with a descriptive name
   (`fix/<short-desc>`, `feat/<short-desc>`, `docs/<short-desc>`, etc.).
3. Make your changes. Follow the
   [upstream code style](https://docs.lizardbyte.dev/projects/sunshine/latest/md_docs_2contributing.html)
   and run `clang-format -i` on any C/C++ you touched before
   committing.
4. Run the test suite locally:
   ```bash
   cmake -B build -G Ninja \
       -DCMAKE_BUILD_TYPE=Release \
       -DBUILD_TESTS=ON \
       -DSUNSHINE_ENABLE_TRAY=OFF \
       -DSUNSHINE_ENABLE_CUDA=OFF \
       -DCUDA_FAIL_ON_MISSING=OFF \
       -DFFMPEG_PREBUILT=ON
   cmake --build build -j$(($(nproc) / 2))
   ./build/tests/test_sunshine --gtest_brief=1
   ```
   Tests should report `0 FAILED`. The pre-existing 5 skipped tests
   are Windows-specific or TODO inputtino and not your concern.
5. Run `clang-format --dry-run --Werror` on the files you touched:
   ```bash
   clang-format --dry-run --Werror src/<your-changed-file>.cpp
   ```
6. Push the branch and open a PR against `vindeckyy/Solar-Flare@master`.
7. The PR template will ask you to:
   - Describe what the change does and why.
   - List the files you touched.
   - Confirm you ran the test suite and clang-format.
   - Note any upstream PRs you cherry-picked (if any).

## Reporting issues

- **Fork-specific issues** (the 5 config keys, the build script, the
  web UI rebrand, the docs): open a GitHub issue on
  [vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare/issues).
- **Generic Sunshine issues** (anything that would also happen on
  upstream): open an issue on
  [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine/issues)
  first. If it turns out to be a fork-only regression, we'll close
  the upstream one and re-open here.

## Code of conduct

This fork follows the same code of conduct as the upstream project.
Be kind, be helpful, and remember that the maintainer
([@vindeckyy](https://github.com/vindeckyy)) runs this fork for their
own CachyOS box; if you want to add a major feature, propose it in
an issue first so we can agree on scope.

## See also

- [README.md](README.md) — fork entry point
- [docs/PORTING.md](docs/PORTING.md) — multi-distro build instructions
- [docs/CONFIGURATION.md](docs/CONFIGURATION.md) — fork config keys
- [docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md) — fork
  changelog
- [docs/contributing.md](docs/contributing.md) — upstream contribution
  guide (most of it applies here too)