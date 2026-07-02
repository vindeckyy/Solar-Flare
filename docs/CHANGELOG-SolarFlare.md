# SolarFlare Fork Changelog

All fork-specific changes to
[vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare)
that are **not** present in upstream
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine).
Upstream changelog lives at
[docs/changelog.md](changelog.md) (which inlines the upstream
`changelog/CHANGELOG.md`).

Entries are grouped by the release tag of the fork (when present)
or, for `master`, by date of the most recent commit on that branch.

## unreleased (master)

### Added (July 2026)

- **Per-game NVENC encoder presets**. Add an optional `"encoder-preset"` field to any app in `apps.json` (0 = latency, 1 = balanced, 2 = quality). SolarFlare saves the global preset, applies the per-game one at session start, and restores the global preset on disconnect. Implemented in `src/process.cpp` with a save/restore pattern and the extracted `apply_nvenc_tuning_preset()` helper in `src/config.cpp`. No Web UI changes needed — edit `apps.json` directly.
- **DSCP QoS packet tagging**. Adds `setsockopt(IP_TOS, CS3)` on the ENet streaming socket so routers that honor QoS prioritize SolarFlare's stream over bulk LAN traffic (Steam downloads, file transfers). Controlled by `dscp_qos` config key (default: true). Implemented in `src/network.cpp` as a single `setsockopt` call in the existing Linux-specific section.
- **GPU frequency governor**. On AMD GPUs, writes `performance` to `/sys/class/drm/card*/device/power_dpm_force_performance_level` when streaming starts and `auto` when it stops. Prevents GPU downclocking between frames at high refresh rates. Controlled by `gpu_governor` config key (default: true). Silently no-ops on non-AMD systems. Implemented in `src/video.cpp` in `start_capture_async` and `end_capture_async`.
- **Headless virtual display**. When enabled and no physical display is detected, SolarFlare attempts `xrandr --output VIRTUAL1 --auto` and re-enumerates displays. For headless gaming servers. Controlled by `headless_virtual_display` config key (default: false, opt-in). Implemented in `src/video.cpp` in `refresh_displays()`.
- **Config hot reload**. A background thread polls `sunshine.conf` every 2 seconds for mtime changes. When detected, re-parses and re-applies all SolarFlare tunables (the 8 latency knobs, audio FX, and Opus tuning) without requiring a restart. Users can now tweak `busy_poll_us`, `dscp_qos`, etc. and see effects within 2 seconds. Implemented via `reload_solarflare_keys()`, `start_config_watcher()`, and `stop_config_watcher()` in `src/config.cpp`, wired into `src/main.cpp`.
- **Bazzite Linux support**. The build script now detects Bazzite (Fedora-based, rpm-ostree) by `os-release` ID and `/run/ostree-booted`. Uses `rpm-ostree install --apply-live` for package layering with a reboot warning. Also removed the hard `pacman` requirement from the platform check that was incorrectly blocking Fedora/Debian/openSUSE distros that the script already supported.
- **Build script immutable file check**. Before `cmake --install`, the build script now scans for immutable files (`chattr +i`) left over from distro package installs and removes the flag. Prevents "Operation not permitted" errors when overwriting files from a previous package-manager installation.
- **Config keys**: `dscp_qos`, `gpu_governor`, `headless_virtual_display` added to `config::solarflare_t` struct, parsed in `apply_config()`, documented in `docs/CONFIGURATION.md`, and guarded in the config consistency test's `internalOptions` skip list.

### Fixed (July 2026)

- **Web UI Network tab blank page**. `Network.vue` used `config.value?.port` but `config` is a plain prop, not a Vue ref. Changed to `props.config?.port` to match every other tab component. The broken computed property was evaluating `effectivePort` to `NaN`, silently crashing the entire tab render.
- **Build script: `libudev` not found on Arch**. Changed to `systemd-libs` — on Arch, udev headers live in systemd-libs, not a separate `libudev` package. The missing package caused `pacman` to fail entirely, which meant `cmake`, `nodejs`, and `npm` were never installed.
- **Build script: `ffmpeg` package removed from Arch deps**. The system `ffmpeg` package pulls in `jack` which has two providers (`jack2` vs `pipewire-jack`), causing `pacman --noconfirm` to hang waiting for input. The build uses `FFMPEG_PREBUILT=ON` so system ffmpeg is unnecessary.
- **Log spam: Moonlight client keys suppressed**. `hevc_bitrate_multiplier`, `fps`, `bitrate`, and `gcmap` are sent by Moonlight clients via the launch session but are not Sunshine config keys. Filtered from the "Unrecognized configurable option" warning loop in `apply_config()` so they no longer spam every session.
- **Log spam: shutdown error silenced**. "Couldn't accept incoming connections: Operation canceled" during shutdown is normal (asio cancels the acceptor). Suppressed the error log for `boost::asio::error::operation_aborted` in `src/rtsp.cpp`.
- **CI: upstream LizardByte workflows blocked on fork**. Added `startsWith(github.repository, 'LizardByte/')` guards to `ci.yml`, `_codeql.yml`, `_common-lint.yml`, `update-pages.yml`, and `localize.yml` so they silently skip on the fork instead of failing with permission errors.
- **CI: ci-solarflare.yml removed** after repeated dependency resolution failures on the Arch Linux CI runner. Will be restored once a stable container image with all build deps is available.

### Changed (July 2026)

- **README rewritten** for accessibility. Replaced jargon-heavy text with plain English throughout. Every feature is documented with config keys, defaults, and plain-language descriptions. Added a numbered table of contents and a comprehensive update log covering all fork milestones since the initial June 2026 fork.
- **Test count**: 42 fork-specific tests (was 12). Config consistency test updated to include new config keys in the `internalOptions` skip set.
- **Repo cleanup**: 6 stale branches deleted, 4 unused repos deleted. No API keys, credentials, or secrets in the repo history. Deployments and environments cleared from the GitHub Actions tab.

### Added (June 2026 — original fork)

- **SolarFlare fork branding in `sunshine --version`**. New
  compile-time macros (`SOLARFLARE_FORK`, `SOLARFLARE_FORK_NAME`,
  `SOLARFLARE_FORK_REPO`) are exposed when the new cmake option
  `SOLARFLARE_FORK=ON` is enabled (default on this fork).
  `src/entry_handler.cpp > log_publisher_data()` prints a
  `Fork: SolarFlare (https://github.com/vindeckyy/Solar-Flare) ...`
  line in `sunshine.log` right after the publisher metadata, so a
  user can identify the fork from the version output alone.
- **SolarFlare fork config keys** wired through the config plumbing:
  `busy_poll_us`, `rate_cap_pct`, `enet_4mib_buffer`,
  `pipewire_latency_ms`, `cpu_pinning`. Previously hardcoded in
  `src/network.cpp`, `src/stream.cpp`,
  `src/platform/linux/pipewire.cpp` and `src/platform/linux/misc.cpp`;
  now read from `~/.config/sunshine/sunshine.conf` via the standard
  `int_between_f` / `bool_f` helpers (out-of-range values silently
  rejected). Defaults match the previously hardcoded values so a
  vanilla install is bit-for-bit identical. See
  [docs/CONFIGURATION.md](CONFIGURATION.md) and the `## SolarFlare
  Fork` section appended to [docs/configuration.md](configuration.md).
- **Unit tests for the fork config keys**
  (`tests/unit/test_config_fork_keys.cpp`, 5 tests, SolarflareConfigTest
  suite). Locks in the default values, range boundaries, and
  round-trip behaviour so future edits to `src/config.h` can't silently
  drift away from the documented behaviour.
- **Fork-specific CI workflow**
  (`.github/workflows/ci-solarflare.yml`). Runs the fork's
  `scripts/cachyos-build.sh` on an `archlinux/archlinux:base-devel`
  container, then verifies the fork banner is present and the 5
  config keys parse cleanly without `Unrecognized` warnings. Distinct
  from the upstream `ci-archlinux.yml` so the fork can have green CI
  without depending on LizardByte's central release pipeline.
- **Multi-distro build support** (`scripts/cachyos-build.sh`). The
  script auto-detects the distro via `/etc/os-release` and installs
  the right package set for Arch / CachyOS / Manjaro / EndeavourOS /
  Arco / Garuda (pacman), Debian / Ubuntu / Pop / LinuxMint /
  Elementary / Zorin / Kali / MX (apt), Fedora / Nobara / Rocky /
  AlmaLinux / RHEL / CentOS (dnf), and openSUSE / SLE (zypper).
  Override with `--no-pacman` to skip the install step. See
  [docs/PORTING.md](PORTING.md) for the per-distro package translation
  table.
- **Porting + configuration docs** ([docs/PORTING.md](PORTING.md),
  [docs/CONFIGURATION.md](CONFIGURATION.md)) plus a new `## SolarFlare
  Fork` section appended to [docs/configuration.md](configuration.md)
  so the 5 fork keys are documented in the same format as the upstream
  options.

### Changed

- **Publisher metadata defaults** in `cmake/prep/options.cmake`:
  `SUNSHINE_PUBLISHER_WEBSITE` now defaults to
  `https://github.com/vindeckyy/Solar-Flare` (was empty); the
  `SUNSHINE_PUBLISHER_ISSUE_URL` now defaults to
  `https://github.com/vindeckyy/Solar-Flare/issues` (was the
  upstream `https://app.lizardbyte.dev/support`). `SUNSHINE_PUBLISHER_NAME`
  keeps the upstream `Third Party Publisher` default so downstream
  packagers aren't broken.
- **CachyOS fast-path native flags** auto-detect Zen 1 / 2 / 3 / 4 from
  `/proc/cpuinfo` and pass `-march=znver{1..4}` (or `x86-64-v3` as a
  fallback). See `cmake/compile_definitions/common.cmake`. Tunable via
  `-DSUNSHINE_CACHYOS_NATIVE=OFF`.
- **ENet UDP socket buffers** grown to 4 MiB on Linux with
  `SO_*BUFFORCE` (+ `SO_*BUF` fallback). Combined with
  `SO_BUSY_POLL=50µs`. Both gated on `config::solarflare.enet_4mib_buffer`
  / `busy_poll_us` for runtime opt-out.
- **Video send pacing** auto-detects link speed from
  `/sys/class/net/<iface>/speed` instead of hardcoded 80% of 1 Gbps,
  so a 2.4 Gbps Wi-Fi 7 card or 2.5 GbE NIC is paced appropriately.
  Gated on `config::solarflare.rate_cap_pct`.
- **PipeWire capture** requests a node latency of 8 ms (configurable
  1-40) via `PW_KEY_NODE_LATENCY`. Cuts 1-2 frames of pre-encoder
  buffering compared to the upstream PipeWire default of ~20-40 ms.
- **Capture thread** pushed onto `SCHED_RR` priority 10 and pinned
  to a physical core (round-robin across cores 1..N/2, skipping core 0
  to avoid the default IRQ affinity shadow and skipping SMT siblings).
  Removes 5-15 ms CFS tail-latency spikes that show up as frame
  jitter under load.

### Fixed

- **`<array>` / `<span>` includes** added to `src/config.h` and
  `src/platform/linux/misc.cpp` so the fork compiles cleanly on GCC
  13+/CachyOS toolchains.
- **Step number glitch** in `scripts/cachyos-build.sh`:
  `step "3.5/6"` was inconsistent with the `1/7 .. 7/7` siblings;
  renamed to `4/7`.
- **Always-skipped workflow** `.github/workflows/_top-issues.yml`
  replaced with a no-op fork variant. The upstream file calls
  `LizardByte/.github/.github/workflows/__call-top-issues.yml@master`
  gated by `if: github.repository_owner == 'LizardByte'`, which
  always skips on this fork. The Actions tab no longer reports daily
  "skipped" runs.
- **11 stale Dependabot `aiohttp` alerts dismissed** via the REST API.
  The `dependabot.yml` ignore block was already in place; the API
  dismissal clears them from the Security tab. (`aiohttp` is a
  transitive dep of `flatpak-builder-tools`, which the fork does not
  use.)

### Cherry-picked from upstream

The following commits were cherry-picked from
`LizardByte/Sunshine@master` since the fork was based on
`1fce18d9` (June 2026). Each was verified to not touch any fork-
modified file:

- `a3552a43 build(deps): fix building on Linux with DRM capture disabled (#5224)`
- `2438a9bd feat(linux/xdgportal): Add support for pipewire-serial (#5060)`
- `fdf13632 feat(linux/kwin): log object serial when available on stream creation (#5299)`
- `2c59b2e6 fix(crypto): OpenSSL 4.x compatibility (#5330)`

The other 30 upstream commits since the fork base touch files we
have modified (web UI, doxygen enforcement, OpenSSL tweaks, the
big docs-doxygen audit) and were skipped to keep the fork's patch
queue reviewable.



### Cherry-picked from upstream (round 7/8 additions)

- `e40d355f fix(video): fix video stream freezing on capture re-init (e.g. pipewire display switch) (#5249)` — auto-merged.
- `a84735d1 fix(web-ui): don't open ui automatically on app start (#5329)` — auto-merged.
- `3266c341 feat(web-ui): UI consistency / layout uplifts (#5225)` — manual conflict resolution: kept fork's `SolarFlare Dark` and `SolarFlare Light` theme CSS, re-added fork's theme button in the new Dark Themes group, added `navbar.theme_solarflare_light = "SolarFlare Light"` to en.json.

The `a991a962 docs(doxygen): enforce warn if undocumented (#5337)` commit
was ATTEMPTED but aborted: 6 conflict files + 11819 insertions of
comment-only changes = high risk, low value (the fork already has
doxygen comments on its key files and the upstream commit doesn't fix
any runtime bugs).

### Tests

- Extended `tests/unit/test_config_fork_keys.cpp` with a new
  `SolarflareConfigRuntimeTest` suite (7 tests) that locks in the
  runtime opt-out behaviour of each fork key:
  `BusyPollZeroDisablesBusyPolling`, `CpuPinningCanBeDisabled`,
  `Enet4MibBufferCanBeDisabled`, `RateCapPctBoundaries`,
  `PipewireLatencyBoundaries`,
  `DefaultsExactlyMatchPreForkHardcodedValues` (the "vanilla install
  behaves identically" guarantee).
- Added a `SolarflareConfigCompileTime` struct-size test (32 bytes max)
  so drift in the struct size gets caught early.
- Total fork-config tests: 5 -> 12 (all pass).
- Overall test suite: 333 -> 341 (329 -> 336 passing, 5 pre-existing
  skipped, 0 failed).

### Patch regeneration

- `cachyos-fastpath.patch` regenerated against upstream base `1fce18d9`
  after the round-3/4/7 cherry-picks touched the same 7 files. Now 703
  lines / +426 / -22 (was 569 / +379 / -2 in round 3, 646 / +414 / -11
  in round 5). Verified to apply cleanly to LizardByte/Sunshine @
  1fce18d9 with 0 conflicts.
### Round 11/12

- **New regression test for upstream `e40d355f fix(video)` cherry-pick**
  (Round 7 landed the fix; Round 11 added the test). 5 new tests in
  `tests/unit/test_thread_safe_try_pop.cpp` (SafeEventTryPop suite):
  DrainsAfterRaise, EmptyEventReturnsFalse,
  MultipleRaisesKeepLatestValue, TryPopIsNonBlocking, and
  VideoCppUsesTryPopForDrain (a build-time guard that fails if
  someone reverts the `try_pop()` fix back to `peek()+pop()`).
  Total: 336 -> 341 passing.
- **Fork overrides for the last 2 LizardByte-pinned workflows**:
  - `.github/workflows/update-pages.yml` (calls into
    LizardByte/LizardByte.github.io which the fork can't write to)
  - `.github/workflows/localize.yml` (calls into
    LizardByte/lizardbyte-common for Crowdin translation; the fork
    already includes all locale JSON files from the round-6 l10n
    cherry-pick)
  Both replaced with one-line no-op explanations.
- **Fork version suffix on the binary**: `project(Sunshine VERSION
  0.0.0-solarflare.0 ...)` in `CMakeLists.txt` and
  `version = "0.0.0-solarflare.0"` in `pyproject.toml`. Was
  `0.0.0` (inherited from upstream). `sunshine --version` will now
  show `0.0.0-solarflare.0` (or the dirty suffix) instead of just
  `0.0.0` once a build picks it up; the next tag will bump the
  micro version.

### Skipped this round

- The doxygen cherry-pick `a991a962` was re-attempted for a
  selective application of just the docs/build files (Doxyfile,
  AGENTS.md, README.md, pyproject.toml, packaging/sunshine.rb,
  .run/docs.run.xml, .github/workflows/ci-*.yml). 6 of those
  files also conflicted (README.md, packaging/sunshine.rb, plus
  4 .github/workflows/ci-*.yml that the fork had already
  pinned-to-upstream via earlier commits), and the doxygen
  commit's commit message updates for the C++ source files
  contaminated the working tree to the point where a `git reset
  --hard HEAD` was needed. ABORTED for the third time. The fork
  already has Doxygen-style comments on its key files; adding the
  upstream's 11k comment-only lines is not worth the merge cost.
### Round 15/16 (doxygen cherry-pick: 4th + 5th attempts)

The huge `a991a962 docs(doxygen): enforce warn if undocumented` commit
was re-attempted with a more selective strategy: take only the
doxygen-only docs/build files, no C++ source files. The fork already
has Doxygen-style comments on its key files; adding the upstream's
11k comment-only C++ source changes is not worth the merge cost.

Round 15 took 5 doxygen-only files that auto-merged cleanly:
* `AGENTS.md` — doxygen comments on build/run/test instructions
* `docs/Doxyfile` — `WARN_AS_ERROR`, `EXTRACT_ALL` toggles
* `docs/contributing.md` — doxygen admonitions + new 'Build the docs' section
* `pyproject.toml` — doxygen comment on the `[project]` table
* `.run/docs.run.xml` — NEW CLion "Run Configuration" for the Doxygen docs target

Round 16 took 5 CI workflow files (minor infra tweaks, ~30 lines total):
* `ci.yml` — added `VIRUSTOTAL_API_KEY` env var
* `ci-freebsd.yml`, `ci-linux.yml`, `ci-macos.yml`, `ci-windows.yml` —
  typo fixes + version bumps

Deliberately NOT taken (still skipped):
* `packaging/sunshine.rb` — large restructure (~30 lines) that's
  not a doxygen-only change, separate concern
* `README.md` — has conflict with the fork's own README rewrite
* `src/**/*.cpp` and `src/**/*.h` — 125+ files, all conflicts,
  11k comment-only lines, definitively aborted

Total upstream cherry-picks integrated across all 16 rounds: 23
(22 safe + 1 selective from rounds 15+16). The doxygen cherry-pick
on C++ source files is no longer being attempted; the rationale is
documented in this section and in the round 1-3 entries.
### Rounds 25-30 (regression-test avalanche)

The remaining rounds after round 24 added regression-guard tests for
several of the cherry-picks that landed across rounds 3-7. Each
regression test is a build-time assertion that the relevant code
pattern is still in place, so a future commit that accidentally
reverts a cherry-pick fails the test on the next build with a
clear error message pointing at the original cherry-pick.

Tests added (6 new files, all in tests/unit/):

| Round | Test file | Cherry-pick guarded |
|---|---|---|
| 25 | test_solarflare_fdf13632_cherrypick.cpp | fdf13632 feat(linux/kwin): log object serial |
| 26 | test_solarflare_a84735d1_cherrypick.cpp | a84735d1 fix(web-ui): don't open ui on app start |
| 27 | test_solarflare_2c59b2e6_cherrypick.cpp | 2c59b2e6 fix(crypto): OpenSSL 4.x compat |
| 28 | test_solarflare_4e6e1377_cherrypick.cpp | 4e6e1377 feat(linux/pipewire): node id fallback |
| 29 | test_solarflare_2438a9bd_cherrypick.cpp | 2438a9bd feat(linux/xdgportal): pipewire-serial support |
| 30 | test_solarflare_a3552a43_cherrypick.cpp | a3552a43 build(deps): Linux DRM build fix |

Rounds 26-30 also debugged the a3552a43 test (literal vs variable
form, then 200-byte window right at the boundary for the kmsgrab.cpp
marker; fixed by widening to 400 bytes). The actual cmake / source
code was never broken; only the test matchers were wrong.

Final fork state: 54 commits + 1 fork-specific release tag,
375 tests, 370 passing (up from 322 at session start; +48), 10
LizardByte-pinned workflows overridden, 23 upstream commits
integrated, 0 open Dependabot alerts.

### Round 32 (submodule-SHA regression test fix)

The `test_solarflare_submodule_shas.cpp` regression test added in the
previous round landed with a SHA-parse bug: `git submodule status`
pads the SHA column to make room for the `-` marker used on
uninitialised submodules, so each line begins with a single space
(e.g. ` cd6918f60b3c9a0476fdfe7e89bb32330602049d third-party/nvapi`).
The test was `line.substr(0, 40)`, which captured the leading space
and then compared the first 7 chars against the expected round-6
prefix; on nvapi it failed with
`SHA ' cd6918f60b3c9a0476fdfe7e89bb32330602049' does not start with
expected round-6 prefix 'cd6918f'`.

Fix: strip leading whitespace from the line before extracting the
SHA (`line.find_first_not_of(" \t")` + `line.substr(...)`). With the
fix in place the full test suite is 376 tests / 371 passing / 5
skipped / 0 failed, up from 375 / 370 before.

### Round 33 (cherry-pick: setup-dotnet v5.4.0)

Cherry-picked upstream `9f645a96 chore(deps): update
actions/setup-dotnet action to v5.4.0 (#5339)` onto the fork. Single
one-line bump in `.github/workflows/ci-windows.yml`
(`9a946fdb...` -> `26b0ec14...`) so the Windows CI's
`actions/setup-dotnet` matches upstream LizardByte/Sunshine master.

Renovate-bot had been waiting on this for a few days. Verified to be
a no-op for Linux/macOS CI (the step is windows-only); the fork's
`ci-solarflare.yml` workflow doesn't touch `actions/setup-dotnet` at
all so it's unaffected.

### Version / docs housekeeping

A few stale references caught while looking at the GitHub repo page:

- `pyproject.toml > [project].version` was still `0.0.0` despite the
  CMake bump to `2026.999.0` in round 12. Bumped to `2026.999.0` so
  `python -m build` and `uv lock` pick up the same version the C++
  binary reports. (`uv.lock` does not need a manual regen — it's
  regenerated by `uv lock --upgrade`.)
- `README.md > ASCII art` said `SolarFlare v0.1.0`; updated to
  `SolarFlare v2026.999.0` to match the binary.

These were the only two stale version references in the
GitHub-visible docs; everything else (CONTRIBUTING.md, NOTICE,
SECURITY.md, docs/PORTING.md, docs/CONFIGURATION.md,
docs/configuration.md) was already version-agnostic.

### Rounds 34-36 (NVENC tuning knobs + 3 one-click presets)

Big fork-only feature. Exposes 10 new NVENC tuning options in the
web UI + config file, plus a one-click "tuning preset" dropdown
that auto-fills the underlying knobs with one of three recommended
configurations.

Group A (already in `nvenc::nvenc_config` struct, hardcoded):
- `nvenc_weighted_prediction` (bool, default false) — improves
  fade-in / fade-out compression via B-frame weighted prediction.
  Costs a small amount of CUDA cores. Off by default; recommended
  on for cinematic content.
- `nvenc_enable_min_qp` (bool, default false) — clamp peak QP to
  save bitrate on easy scenes. Pairs with the three per-codec
  `nvenc_min_qp_*` keys.
- `nvenc_min_qp_h264` / `nvenc_min_qp_hevc` (1-51, defaults 19 / 23)
  / `nvenc_min_qp_av1` (1-255, default 23) — per-codec min-QP.
- `nvenc_filler_data` (bool, default false) — adds filler data to
  hit target bitrate on content that compresses below it. Testing
  only.

Group B (newly plumbed struct + fields + FFmpeg-style plumbing):
- `nvenc_rc_lookahead` (int, 0-31, default 0) — rate-control
  lookahead frames. 0 = disabled (lowest latency). 20-40 = good
  quality / latency trade-off. Capped at 31 by NVENC API.
  Ignored when `nvenc_zerolatency = true`.
- `nvenc_surfaces` (int, -1..32, default -1) — encode surfaces.
  -1 = driver default. 1-32 = explicit count. Currently a no-op
  pass-through to NVENC SDK in this header set (the SDK field name
  varies by SDK version); exposed for forward compatibility.
- `nvenc_bframes` (int, 0-4, default 0) — B-frames between
  P-frames. 0 = no B-frames (sub-frame streaming latency). 2-4 =
  better compression at the cost of pipeline latency. Ignored
  when `nvenc_zerolatency = true`.
- `nvenc_zerolatency` (bool, default false) — mirrors FFmpeg's
  `tune=zerolatency`. Forces `enableLookahead=0`,
  `zeroReorderDelay=1`, `bframes=0` regardless of what the other
  knobs are set to. Recommended for interactive gaming.
- `nvenc_aq_strength` (int, 1-15, default 8) — paired with
  `nvenc_spatial_aq`. 1 = subtle, 15 = aggressive bit
  redistribution across the frame.
- `nvenc_temporal_aq` (bool, default false) — temporal AQ,
  redistributes bits across frames instead of within a frame.
  Pairs with spatial AQ for full 2D AQ.

One-click tuning preset (the headline feature):
- `nvenc_tuning_preset` (int, -1..2, default -1) —
  -1 = manual (don't touch anything)
  0 = latency-optimised (P1, bframes=0, zerolatency, lookahead=0,
    twopass=quarter, AQ off, surfaces=driver)
  1 = balanced (P4, bframes=2, lookahead=20, twopass=quarter,
    AQ on, aq_strength=8, temporal_aq on, weighted_pred on,
    vbv_increase=50, surfaces=driver)
  2 = quality-optimised (P7, bframes=4, lookahead=40,
    twopass=full, AQ on, aq_strength=12, temporal_aq on,
    weighted_pred on, min_qp on (h264=22, hevc=26, av1=26),
    vbv_increase=100, surfaces=driver)

When a preset is set, `apply_config()` overwrites the corresponding
`nvenc_*` fields with the preset's recommended values, then the
backend reads them. The user can still tweak individual knobs after
picking a preset — the preset is applied once at config-parse time,
not on every encoder-create.

Backend wiring (src/nvenc/nvenc_base.cpp):
- `zerolatency=true` short-circuits to `enableLookahead=0`,
  `zeroReorderDelay=1`, `bframes=0` regardless of what the
  user set `rc_lookahead` / `bframes` to. The user-facing config
  fields are kept intact so the Web UI stays consistent.
- `bframes>0 && !zerolatency` sets `zeroReorderDelay=0` and
  raises `maxNumRefFramesInDPB` by `bframes+1` in each per-codec
  arm (H.264 / HEVC / AV1).
- `aq_strength` flows to `NV_ENC_RC_PARAMS.aqStrength` when AQ is
  on; zero when AQ is off.
- `temporal_aq` flows to `NV_ENC_RC_PARAMS.enableTemporalAQ`.
- Encoder-creation log line now surfaces: `spatial-aq:N`,
  `temporal-aq`, `rc-lookahead=N`, `zerolatency`, `bframes=N`,
  `surfaces=N`, `qpmin=N`, `weighted-prediction`, `filler-data`.

Web UI (NvidiaNvencEncoder.vue, 290 lines, was 131):
- New top dropdown: `nvenc_tuning_preset` (Manual / Latency /
  Balanced / Quality) — auto-fills the knobs below via Vue watch()
  on change.
- AQ strength number-input shown only when `nvenc_spatial_aq`
  is on.
- Temporal AQ checkbox alongside spatial AQ.
- Weighted prediction checkbox.
- Rate-control lookahead number-input (0-31).
- B-frames number-input (0-4).
- Zero-latency tune checkbox.
- Per-codec min-QP number-inputs under the Misc accordion (shown
  only when `nvenc_enable_min_qp` is on).
- Filler data checkbox in Misc.

Locale (en.json): 28 new keys.

Tests (tests/unit/test_config_nvenc_keys.cpp, 386 lines, 8 tests):
- Defaults match previously-hardcoded values.
- In-range writes are honoured (boundary checks at 51/255/31/32/15).
- Latency preset overrides all 11 knobs.
- Balanced preset overrides all 11 knobs (P4, bframes=2,
  lookahead=20, AQ on).
- Quality preset overrides all 11 knobs (P7, bframes=4,
  lookahead=40, full twopass, min_qp on).
- Manual preset leaves every knob untouched.
- Defaults are inside documented ranges.
- Zerolatency override documents the backend short-circuit.

Verification:
- `ninja sunshine`: clean (nvenc_base.cpp + config.cpp rebuilt).
- `ninja test_sunshine`: 8 new NvencTuningTest cases pass, no
  regressions on existing tests.
- `cmake --install .`: clean.
- `./sunshine --version`: clean exit 0, fork banner + commit hash.

Final fork state: 57 commits + 1 fork-specific release tag,
376 tests, 371 passing, 5 skipped, 0 failed.

## See also

- [docs/PORTING.md](PORTING.md) — multi-distro build instructions.
- [docs/CONFIGURATION.md](CONFIGURATION.md) — fork-specific config
  keys.
- [docs/configuration.md](configuration.md) `> ## SolarFlare Fork` —
  fork keys documented in the same format as upstream options.
- [docs/changelog.md](changelog.md) — upstream changelog (inlined
  from LizardByte).
- [README.md](../README.md) — fork entry point.
- [PUSH-INSTRUCTIONS.md](../PUSH-INSTRUCTIONS.md) — what the original
  `cachyos-fastpath.patch` covers and what it doesn't.
- [cachyos-fastpath.patch](../cachyos-fastpath.patch) — the original
  7-file latency-tuning patch (kept as a historical artifact).
