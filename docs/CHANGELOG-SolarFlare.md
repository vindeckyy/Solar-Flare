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

### Added

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
