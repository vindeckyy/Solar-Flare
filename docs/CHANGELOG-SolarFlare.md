# SolarFlare Fork Changelog

All fork-specific changes to [vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare) that are **not** present in upstream [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine). Upstream changelog lives at [docs/changelog.md](changelog.md) (which inlines the upstream `changelog/CHANGELOG.md`).

Curated sections below group commits by feature and date, oldest commit first within each topic. The **Full commit index** at the bottom lists any commits that fell between the cracks — the curated sections above should cover nearly everything. Use `git show dbf8232` for the full diff.

---

## 2026-07-26 — SolarFlare v1.0.7 (`v2026.726.1-solarflare`)

### SolarFlare identity system

Replaced the inherited Sunshine artwork and interim SolarFlare icons with the
Vector Eclipse identity across the Linux application icon, Web UI, system tray
states, favicon, documentation, README, and GitHub Pages site. Added
size-appropriate raster assets and retained clear playing, pausing, and locked
tray-state indicators.

Linux release artifacts are built and validated on the target system before
manual publication. Verification checks the SolarFlare logo bytes inside the
packaged Web UI, and each release includes SHA-256 checksums.

## 2026-07-25 — SolarFlare v1.0.6 (`v2026.725.1-solarflare`)

Release notes are published with the corresponding GitHub release. Compare this tag with the previous SolarFlare release for the complete change set.

## 2026-07-25

### KMS capture on Wayland without `skip_wayland_correlation`

Fixed a black-screen regression on KDE Plasma Wayland with KMS capture
([#19](https://github.com/vindeckyy/Solar-Flare/issues/19)). The
timeout-guarded `wl_display` dispatch loop in `wl::monitors()` consumed
`wl_output` geometry/mode events before the output listener was attached,
leaving `viewport.width/height` at zero. `correlate_to_wayland()` then
overwrote the correct KMS-derived resolution with that zeroed viewport,
producing a 0×0 capture region unless `skip_wayland_correlation` was set.

- attach the `wl_output` listener when the monitor is bound, not after the
  first dispatch loop
- keep the KMS-derived size when the compositor reports no physical mode
- restore the resolution-mismatch warning so disagreements are visible in logs
- unit test the viewport merge helper

## 2026-07-20

### KMS capability check and capture fallback

KMS capture now checks for `CAP_SYS_ADMIN` before advertising the backend. If
KMS initialization fails at runtime, SolarFlare logs the failure and falls
through to the next available capture source instead of returning a dead
capture path.

## 2026-07-21

### Polaris acknowledgment

Added an explicit acknowledgment that SolarFlare's Linux capture, compositor,
and stream-health design was informed by reviewing the
[papi-ux/polaris](https://github.com/papi-ux/polaris) source. SolarFlare remains
a Sunshine-derived project; this records design inspiration separately from
source-code attribution.

## 2026-07-18 — SolarFlare v1.0.5 (`v2026.718.5-solarflare`)

SolarFlare's Web UI is now a responsive observatory console with a persistent
desktop navigation rail, compact mobile controls, a magnetic-field host
dashboard, denser configuration surfaces, and a fully local featured-client
catalog. User-facing upstream branding is normalized to SolarFlare while
protocol and configuration identifiers remain compatible. The release also
adds focused UI contract tests, six README screenshots, and a reproducible
all-tab screenshot script.

## 2026-07-18

### Observatory Web UI and SolarFlare branding

Rebuilt the host Web UI around a responsive observatory console: a persistent
desktop navigation rail, compact mobile control bar, magnetic-field dashboard,
denser configuration surfaces, and shared theme bootstrap for first-run and
authentication pages. The visual layer preserves existing endpoints,
configuration serialization, theme variants, and the keyboard command palette.
All localized presentation copy now normalizes the upstream product name to
SolarFlare at runtime while protocol identifiers remain compatible.

### Event-driven latency pipeline

Reworked the streaming hot path around event-driven capture and bounded
queues. PipeWire capture is driven by frame arrival instead of phase-based
sleeps, with frame metadata and client-rate decimation for PipeWire and
Hermes-KMS. Per-session pacing now bounds batches, tracks queue age, rejects
stale frames, honors send deadlines, and parses per-frame FEC status for
adaptive network stats. The encoder can apply live NVENC bitrate changes when
supported, and the first-frame path avoids dummy allocation when a real frame
arrives quickly.

Input delivery now uses single-flight batching with bounded drains and stale
HOME timer protection. Audio tracks frame gaps and repairs RTP
sequence/timestamps around dropped frames. Packet ownership is explicit
through move-only encoded packets and queue timestamps, reducing copies and
making queue-age decisions safe.

Added the `latency_mode` setting with `safe` and `aggressive` policies.
Aggressive mode tightens audio and scaler latency tradeoffs, and the latency
NVENC preset disables two-pass encoding. Added six new test files and expanded
regression coverage for queue overflow, input batching, pacing, FEC parsing,
video packet ownership, PipeWire behavior, and configuration consistency.

---

## 2026-07-16

### Portal and PipeWire capture reliability

Hardened portal capture by requesting an embedded cursor only when advertised,
falling back from zero physical monitor dimensions to logical or stream
dimensions, bounding portal D-Bus waits to 15 seconds, subscribing before the
proxy call, validating request paths, and cleaning up response subscriptions
and variants. This keeps absolute input and cursor capture usable and prevents
a stalled portal from wedging the HTTPS control plane.

### SolarFlare v1.0.4 (`v2026.708.4-solarflare`)

Published the follow-up SolarFlare release with the fork's version and binary
packaging paths aligned after the initial Linux binary release.


## 2026-07-15

### Audio controls and packet hardening

Documented the remaining `sf_audio_*` controls: `sf_audio_vad_hysteresis_db`,
`sf_audio_vad_min_speech_ms`, `sf_audio_vad_min_silence_ms`,
`sf_audio_ducker_attack_ms`, `sf_audio_ducker_release_ms`, and
`sf_audio_noise_gate_db`. Added a size guard for short
`IDX_INVALIDATE_REF_FRAMES` packets so malformed clients cannot trigger an
out-of-bounds read.


## 2026-07-13/14

### Morning sweep (Jul 13–14)

General cleanup batch post-release: fixed the CONFIGURATION.md drift caught by the docs-drift agent (tunable count, `virtual_display_resolution` claim, stale file refs), added a `release.sh` script as the single source of truth for version bumps, fixed an RTSP OOB-read in the frame parser (fuzzer find), and patched a GVariant-interned string double-free in the heap path. Also probed for ccache/mold/lld during cmake, added GPL license headers, released capture resources on teardown, and documented the linux resource cleanup.

Also fixed KDE headless detection when `XDG_CURRENT_DESKTOP=plasma`.


---
---

## 2026-07-12

### Security sweep

Seven fixes from a one-shot pentest of the network-reachable surfaces (HTTPS server, RTSP control stream, web UI auth, outbound fetches). All paired with tests except those guarded by the single-threaded HTTPS server, which would need an asio fixture to test in isolation.

- `src/stream.cpp` — bound the length-prefixed parse in `IDX_INPUT_DATA` and `IDX_LOSS_STATS` handlers so a paired client can't construct a `string_view` past the actual buffer (M-1, paired-client OOB-read / OOB-write).
- `src/crypto.cpp` — `PEM_read_bio_X509` / `PEM_read_bio_PrivateKey` return values now checked; malformed client certs during pairing produce a null smart pointer instead of an unwritten `X509`/`PKEY` (M-2, root-cause fix; all callers route through).
- `src/nvhttp.cpp` — cap concurrent TLS handshakes at 64 on the HTTPS server so a slow/abusive client can't stall the single-threaded io_context and DoS the whole `origin_web_ui_allowed` scope (M-3).
- `src/httpcommon.cpp` + `src/confighttp.cpp` — reject passwords shorter than 12 chars at write time; add per-IP token bucket (10 fails / 30s) before doing any hash work, reset on successful auth (M-4).
- `src/confighttp.cpp` — reject `/`, `..`, NUL in cover-upload key (L-1, path-traversal guard, admin-only endpoint).
- `src/confighttp.cpp` — strip CR/LF from API token name before logging to prevent log injection (L-2, admin-only; JSON response is auto-escaped).
- `src/httpcommon.cpp` — `download_file` now requires TLS verify, HTTPS-only via `CURLOPT_PROTOCOLS_STR`, and 10s/5s timeouts (L-3, admin-only belt-and-suspenders for the upstream host check).

### setcap on local builds

Local `cmake --install build` no longer ships a `sunshine` binary with no permitted capabilities. Added an `install(CODE)` hook in `cmake/packaging/linux.cmake` that runs `setcap cap_sys_admin,cap_sys_nice+p` on the installed binary, gated on non-AppImage/non-Flatpak installs so the package paths keep their existing behaviour. RPM/DEB still use the `%caps` spec.


### Binary release asset

SolarFlare v1.0.3 (`v2026.708.3-solarflare`) was the first release to ship a binary. It carries `sunshine-x86_64` (26 MB stripped ELF) at `releases/latest/download/`. The `latest/download` URL is version-independent, so README only needs to point at the alias. README quick-start now lists the binary path alongside the source build.


---

## 2026-07-11

### Morning sweep (Jul 11)

Test suite: 494 tests, 482 passed / 12 skipped / 0 failed
(all clean, ConfigConsistencyTest now passes after test binary rebuild).

Bug found: `third-party/inputtino` submodule pointed to a fork-local commit
(`64436f0`) that only exists on Hayden's local clone and was never pushed
to any remote. Both the new pointer and the old upstream pointer (`7e2bb5d`)
were unreachable from origin — a fresh Solar-Flare clone would fail during
submodule checkout. Fixed by repointing to `b887f6a` (upstream stable HEAD,
fetchable from games-on-whales/inputtino). Hayden's pure MT Type B fix needs
a published inputtino fork to live in (see commit message for recipe).

Other audit checks (doxygen, IPPROTO_IPV6/DSCP, hardcoded sample rates,
mutex-unlock mismatches, null-pointer derefs, test_config_fork_keys coverage):
clean — no new issues.


> **Historical note:** this repair was initially produced on an offline
> maintenance run and was pushed afterward. The referenced submodule pointer is
> present in the public repository.

---

## 2026-07-10

### Morning sweep (Jul 10)

Daily sweep pass: no source-code bugs found in recent changes (NVENC fix, cert
persistence, error system). Static analysis and syntax checks pass clean on all
recently modified sources. Fixed documentation drift in `docs/CONFIGURATION.md`:
added missing `skip_wayland_correlation` key, corrected table header from "five"
to "nine" tunables, and expanded the A/B test section to cover all fork keys.


---

## 2026-07-09

### Morning sweep (Jul 9)

Daily sweep pass over the in-flight `skip_wayland_correlation` feature
(uncommitted work): removed a stray duplicate `src/kmsgrab.cpp` accidentally
copied to the repo root; fixed a doxygen comment typo
(`won'''t` → `won't`) that would fail the doxygen build; replaced the
blocking `wl_display_roundtrip()` calls in `kwingrab.cpp` and `wayland.cpp`
with timeout-guarded dispatch loops so an unresponsive compositor can no
longer hang KMS enumeration forever; and fixed a real bug in the sysfs
resolution fallback where the largest width and height were taken
independently across different connectors, producing a corrupted
`WxH` (e.g. `1920x720`). The fallback now picks the single largest
connector mode by area. Extracted the parsing into
`platf::resolve_sysfs_desktop_size()` and added unit tests covering the
largest-mode pick, unparseable/non-connector entries, and a missing
directory. Extended `test_config_fork_keys.cpp` to snapshot/restore and
assert the new `skip_wayland_correlation` key across the existing default
and runtime-toggle tests.


### Fix build failure: missing wayland-protocols submodule (closes #9)

`scripts/cachyos-build.sh` checked out only a hardcoded list of required
submodules and `third-party/wayland-protocols` was not on it. When that
submodule was empty, cmake failed cryptically at `wayland-scanner` with
"Could not open input file: No such file or directory". The submodule is
now fetched by the build script, and `GEN_WAYLAND()` in
`cmake/macros/linux.cmake` resolves the protocol XML to an absolute path
(fixing the case where `CMAKE_SOURCE_DIR` is relative) and aborts with a
clear "initialise `third-party/wayland-protocols`" message when the file
is missing instead of letting `wayland-scanner` emit an opaque error.


### Version alignment + README rewrite

Aligned the internal build version with the GitHub release tag scheme (`YYYY.DDD.N-<n>-gdbf8232` instead of `<commit-count>-<sha>`). Rewrote the README preamble in conversational style; README no longer reads like a spec sheet.


### SUN_ERR tagged error log

New `src/error.h` + `src/error.cpp` expose `error_category_e`, `encode_error_e`, and the `SUN_ERR(cat, tag, msg)` macro. Every error log line is now self-correlated with `__FILE__:__LINE__:__func__`, a category tag, and a per-category atomic counter. `platf::video::encode()` and `encode_nvenc()` return `std::optional<encode_error_e>` so call sites can branch on the specific cause (`EMPTY_PACKET`, `FRAME_INDEX_MISMATCH`, `UNSUPPORTED_SESSION`) instead of the previous generic "Could not encode video packet" message.


### `/api/errors` HTTP endpoint

New `getErrors()` handler exposes `sunshine::counters()` as JSON via `GET /api/errors`. Gated behind `api_scope_t::LOGS_GET` (same scope as `/api/logs`). Response body has one field per category plus `total`. The Web UI diffs against a snapshot to compute recent-error rate.


### Standalone test runner

`tests/run_test_error.sh` bypasses the cmake test target (which OOMs the 16 GB box on full `test_sunshine` link) by compiling only `src/error.cpp` + `tests/unit/test_error.cpp` + `gtest_main.cc` against the static libs already on disk in `cmake-build-test/_deps/boost-build/libs/`. Peak RAM ~300 MB, runtime ~5 s. 4 unit tests assert the public contract (counter routing + stable string mappings) without requiring a boost log sink. The 3 tests that called `log_error()` → `BOOST_LOG(error)` were removed in favour of directly bumping the atomic counters, since the boost log plumbing isn't our code.


### Cert persistence on restart

Fixed the long-standing "I have to re-pair after every restart" bug. `http::init()` used to take the "empty cert in config" branch on every start and generate a random new `unique_id`, invalidating every paired client. The fix scans `appdata/credentials/` for existing `pkey-*` files, picks the newest by mtime, and adopts that pair. One re-pair required after upgrade (the client certs in `sunshine_state.json` don't match the adopted server cert). After that, every restart preserves the cert.


### CI cleanup: drop ci-copr.yml

The CachyOS-only COPR integration workflow had no secrets on the fork, failed on every release push, and was the source of release-time failure emails. Deleted the file (already `disabled_manually` on the GitHub UI). The watch on `vindeckyy/Solar-Flare` is also set to `ignored` via the API, so no further activity emails come from this repo.



---

## 2026-07-08

### Morning sweep (Jul 8)

Daily sweep pass: synced README version + test badges after the niri PR; added niri (Smithay) Wayland compositor support with auto-detect; fixed `search_path` false-positive in compositor detection (niri / gamescope / kwin_wayland presence on `$PATH` no longer implies a running session).


## 2026-07-07

### Morning sweep (Jul 7)

Daily sweep pass: removed PUSH-INSTRUCTIONS.md from the repo, gitignored `crush.db`, and fixed submodule drift.



## 2026-07-06

### Stream port alignment + audio socket

Aligned stream port offsets with what Moonlight actually expects relative to `https_port`. `VIDEO_STREAM_PORT` 9→16, `CONTROL_PORT` 10→26, `AUDIO_STREAM_PORT` 11→27, dropping the now-unneeded `HTTPS_PORT_OFFSET`. Empty `nvhttp.cert` defaults + removed the blocking `set_options(no_tlsv1, no_tlsv1_1)` on the TLS context (was preventing the SSL_CTX from completing a handshake).


### Pairing session + PIN handling

Reset stale pairing session on retry, safe `pin()` lookup, `not_found()` double-write fix, and held PIN response sets `close_connection_after_response`.


### Video capture threading

Dropped `SCHED_RR` from the video capture thread to reduce scheduling overhead on multi-core hosts.


### X11 touchscreen

Added a udev rule to ensure the touchscreen device is recognized on X11; reverted after it broke touch on real hardware; the touch input path no longer sets `INPUT_PROP_DIRECT` on the uinput device, restoring X11 pointer emulation.


### Process detach

Detached app sessions now stay alive indefinitely (removed the 5-second disconnect loop).


### README + issue #6

Added Solar-Flare update instructions to the README (closes #6), synced badges, fixed the install command, and documented the loading-screen workaround.


### Version bump to 2026.999.2

Bumped stale `2026.999.0` references.



## 2026-07-05

### Morning sweep (Jul 5)

Daily sweep pass two: dead-state removal, naming cleanups, diagnostic logs. Hermes-KMS fd-safety: don't close the FD if `acquire_hermes_kms()` failed; adaptive bitrate recovery arming via a "recovery mode" that ramps the bitrate back up after sustained low packet loss.


### Hermes-KMS kernel module

Vendored Hermes-KMS from `github.com/MrOz59/Hermes-KMS` at `third-party/hermes-kms` (git submodule, GPL-2.0+). `scripts/cachyos-build.sh` runs `packaging/linux/redesign/install-hermes-kms.sh` after `cmake --install`; the script DKMS-installs `hermes_kms.ko` and loads it with `initial_enabled=1` so `HERMES-1` appears in the source selector. Requires kernel-headers + dkms. Removed the duplicate `src/platform/linux/hermes_kms_drm.h`; the C++ capture backend now includes the upstream UAPI header directly.


### Hermes-KMS Web UI + README

Exposed Hermes-KMS in the Linux capture-backend dropdown, surfaced probe failure reason in `verify_hermes_kms`, and updated the README §19 capture-loop description to match the wired implementation (probe + WAIT_FRAME + ACQUIRE_FRAME + DMA-BUF push to encoder).


### Tier-1 low-latency encoder changes

Vulkan encoder perf work exposing `vk_min_qp` / `vk_max_qp` in the web UI; small `power_dpm_force_performance_level` value copy fix in KMS monitor correlation; `portalgrab` no longer hardcodes `cursor_mode=2`.


### Config HTTP API + auth refactor

Added scoped bearer tokens for the config HTTP API and an HTTP control surface for the adaptive bitrate controller. Moved `auth_result_t` full definition into `confighttp.h`. Updated `test_confighttp.cpp` for the new return type. Doxygen for all new public types to satisfy `BUILD_WERROR=ON`.


### Fork redesign services

Ship the boot-time tuning systemd units (`cpu-performance`, `nic-tuning`, `nvidia-clock-lock`) from the repo via an idempotent `install-redesign-services.sh` installer. Drops each `.sh` helper into `/usr/local/sbin/` and each `.service` into `/etc/systemd/system/`.


### screenshot helper script

New `scripts/screenshot-ui.sh` for capturing README screenshots from a live web UI session.


### README + changelog

Synced README badges, version, and changelog with the actual fork state. Updated the changelog entry for the July 4 bug-fix batch and the July 5 features batch.



## 2026-07-04

### Daily bug check (Jul 4)

Bug audit pass covering the Adaptive Bitrate, headless compositor, Steam/Lutris scanner, and KMS paths. All `test_sunshine` runs (464 tests) pass with zero failures. Also collapsed the duplicate `Quick install` block in the README into one.



## 2026-07-03

### Initial SolarFlare fork setup

Established the fork as a standalone build with its own branding, build script, and config keys. Includes the issue-template `LizardByte` → `Solar-Flare` link rewrite and a CPU microarch detection fix for Zen 2 and Zen 5.


### Headless streaming backends

Added three headless streaming backends for environments without a display server: labwc (default), krfb-virtualmonitor (KDE), and gamescope (Steam Deck). The adaptive bitrate section in the Web UI is relabelled as a SolarFlare feature so users can find it.


### Web UI for new config keys

Exposed all new config options in the Web UI and provided defaults for `/api/config`. Updated the README with accurate counts and clarified the headless sections.


### Issue template + build dep cleanup

Replaced LizardByte links in bug-report templates and added the libnuma cmake check.


### Daily bug check (Jul 3)

Bug audit pass on the Jul 3 feature batch plus the Reddit portal/KMS report. Fixed the adaptive bitrate first-sample clamp, the `get_target_bitrate` floor clobber, the headless teardown string-match, the `start_krfb` swallowed-output silent failure, the `stop_labwc` / `stop_gamescope` discarded exit status, the Steam scanner missing Flatpak and Snap installs, the Lutris `.yml`-as-launcher bug, the KMS non-numeric monitor-index garbage, the KMS segfault on hot-swap, and the XDG portal dropping caps before init.



## See also

- [docs/CONFIGURATION.md](CONFIGURATION.md) — the 10 fork-specific latency/display toggles (`busy_poll_us`, `rate_cap_pct`, `enet_4mib_buffer`, `pipewire_latency_ms`, `cpu_pinning`, `dscp_qos`, `gpu_governor`, `headless_virtual_display`, `skip_wayland_correlation`, `latency_mode`).
- [docs/PORTING.md](PORTING.md) — per-distro package translation table for the build script.
- [README.md](../README.md) — fork entry point.
- [cachyos-fastpath.patch](../cachyos-fastpath.patch) — the original 7-file latency-tuning patch (kept as a historical artifact).

## Full commit index (commits not in the curated sections)

### 2026-07-08
