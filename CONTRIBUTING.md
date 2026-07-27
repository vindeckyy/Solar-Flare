# Contributing to SolarFlare

SolarFlare accepts focused, testable changes that improve the Linux streaming
path, host operations, or Web UI without breaking Moonlight
compatibility.

This document defines fork policy. The detailed C++ style, localization,
documentation, and development conventions live in
[docs/contributing.md](docs/contributing.md).

## Before opening a change

1. Search existing issues and recent commits for overlapping work.
2. Separate SolarFlare-specific behavior from inherited Sunshine behavior.
3. Keep protocol identifiers, configuration paths, and executable/service
   compatibility intact unless the change includes a migration plan.
4. Add or update tests for every modified behavior.
5. Update Doxygen and user documentation in the same change.

SolarFlare-specific work belongs in this repository. A defect that reproduces
unchanged in upstream Sunshine should normally be reported upstream first;
fork integrations or regressions should be reported here.

## Areas maintained by SolarFlare

- Linux transport tuning: pacing, busy polling, socket buffers, QoS, and
  adaptive bitrate.
- Capture and scheduling: CPU affinity, GPU governor behavior, headless paths,
  native compiler tuning, and optional performance services.
- Video and audio controls: NVENC presets, per-application overrides, Audio FX,
  and Opus configuration.
- Host access: scoped API tokens, trusted-subnet pairing, and security
  hardening around network-reachable surfaces.
- The SolarFlare Web UI and its responsive layout.
- Multi-distribution build, install, release, and verification tooling.
- Fork documentation and regression contracts.

The authoritative fork-control inventory is
[docs/CONFIGURATION.md](docs/CONFIGURATION.md); do not copy hard-coded setting
counts into new documents.

## Development workflow

Create a topic branch from `master` and keep each commit scoped to one logical
change. Use conventional commit subjects where practical, for example:

```text
feat(web-ui): add encoder health telemetry
fix(network): preserve pacing fallback on unknown links
docs(readme): clarify binary-only updates
```

### Configure and build

Build directory names must start with `cmake-build-`.

```bash
git submodule update --init --recursive

cmake -S . -B cmake-build-dev -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=OFF
cmake --build cmake-build-dev --target test_sunshine -j2
```

Platform-specific options and dependencies are documented in
[docs/building.md](docs/building.md) and [docs/PORTING.md](docs/PORTING.md).

### Test

```bash
./cmake-build-dev/tests/test_sunshine --gtest_brief=1
```

Run the narrowest relevant test filter while iterating, then run the complete
suite before requesting review. Hardware-dependent tests may skip when their
capture, encoder, audio, or input device is unavailable; failures must be
resolved or explained.

### Format and document

```bash
clang-format --dry-run --Werror src/path/to/changed_file.cpp
git diff --check
```

Every new or modified C/C++ declaration requires Doxygen. Primary comments use
the project `/** ... */` format; inline member documentation uses `///<`.

When changing localization, update only the `en` catalog. Do not edit `en_US`,
`en_GB`, or other translations in fork changes.

## Web UI changes

The Web UI is a multi-entry Vite application rather than a client-side routed
SPA. Preserve these contracts:

- `Navbar.vue` owns the semantic navigation shell.
- `init.js` initializes theme and locale state for every entry point.
- `sunshine.css` owns the shared responsive design system.
- Network endpoints and form serialization must remain independent from the
  visual layer.
- Motion must communicate interaction or state; avoid ambient looping effects.
- User-facing product copy should say SolarFlare. Compatibility identifiers
  and upstream links may retain Sunshine where technically required.

Build the Web UI through CMake so assets land in the correct build tree:

```bash
cmake --build cmake-build-dev --target web-ui -j2
```

If the navigation or page layout changes, refresh all six README screenshots
with `scripts/screenshot-ui.sh`.

## Pull request checklist

- The change has a clear problem statement and bounded scope.
- New or changed behavior has tests.
- Doxygen and user documentation are current.
- C/C++ files pass `.clang-format`.
- `git diff --check` passes.
- The Web UI builds when frontend files change.
- No generated build output, credentials, local state, or unrelated submodule
  changes are included.
- Upstream commits or issues are linked when relevant.

Open changes against
[`vindeckyy/Solar-Flare`](https://github.com/vindeckyy/Solar-Flare), not the
LizardByte organization.

## Reporting problems

- Product defects and feature requests:
  [SolarFlare issues](https://github.com/vindeckyy/Solar-Flare/issues).
- SolarFlare-specific vulnerabilities:
  [private security advisory](https://github.com/vindeckyy/Solar-Flare/security/advisories/new).
- Defects reproduced in an unmodified upstream build:
  [LizardByte/Sunshine issues](https://github.com/LizardByte/Sunshine/issues).

Be precise, include reproduction steps and logs with secrets removed, and state
the host distribution, desktop session, GPU, capture backend, encoder, and
Moonlight client version.
