# SolarFlare Fork Changelog

All fork-specific changes to [vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare) that are **not** present in upstream [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine). Upstream changelog lives at [docs/changelog.md](changelog.md) (which inlines the upstream `changelog/CHANGELOG.md`).

Curated sections below group commits by feature and date, oldest commit first within each topic. The **Full commit index** at the bottom lists any commits that fell between the cracks — the curated sections above should cover nearly everything. Use `git show dbf8232` for the full diff.

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

- `68b0f26` feat(install): apply setcap on local cmake --install for cap_sys_admin,cap_sys_nice

### Binary release asset

First release to ship a binary. `v2026.708.3-solarflare` carries `sunshine-x86_64` (26 MB stripped ELF) at `releases/latest/download/`. The `latest/download` URL is version-independent, so README only needs to point at the alias. README quick-start now lists the binary path alongside the source build.

- `fc94c5e` docs(readme): add binary-download option to quick start
- `783b6e2` docs(readme): link to version-independent binary name

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

- `05f8c76` fix(submodule): point third-party/inputtino to upstream stable HEAD

> **Note:** Commit is local-only. Push requires a `.netrc`-hosting host — not
> on today's cron slot. Next cron on a populated host will push it.

---

## 2026-07-10

### Morning sweep (Jul 10)

Daily sweep pass: no source-code bugs found in recent changes (NVENC fix, cert
persistence, error system). Static analysis and syntax checks pass clean on all
recently modified sources. Fixed documentation drift in `docs/CONFIGURATION.md`:
added missing `skip_wayland_correlation` key, corrected table header from "five"
to "nine" tunables, and expanded the A/B test section to cover all fork keys.

- `dbf8232` docs: fix CONFIGURATION.md drift — add skip_wayland_correlation, fix tunable count, expand A/B test

---

## 2026-07-03

### Initial SolarFlare fork setup

Established the fork as a standalone build with its own branding, build script, and config keys. Includes the issue-template `LizardByte` → `Solar-Flare` link rewrite and a CPU microarch detection fix for Zen 2 and Zen 5.

- `d69aaf0` docs: changelog entry for July 3 2026 batch
- `3e05084` fix: CSS syntax error, PreProcessor test isolation, and config documentation drift
- `d29dea2` fix: AMD 9070 XT (RDNA 4) encoder selection and GL crash
- `3de3417` fix: broken to_bool() and pipewire dummy_img() memory leak
- `ea69a44` fix: replace LizardByte links with Solar-Flare, add KWin privilege-drop retry
- `23cadc3` fix: replace LizardByte pacman-repo link in bug-report issue template
- `9d18542` fix: correct CPU microarch detection for Zen 2 and Zen 5 in cmake

### Headless streaming backends

Added three headless streaming backends for environments without a display server: labwc (default), krfb-virtualmonitor (KDE), and gamescope (Steam Deck). The adaptive bitrate section in the Web UI is relabelled as a SolarFlare feature so users can find it.

- `66d0c20` feat: Polaris-inspired features - command palette, trusted subnet auto-pairing, adaptive bitrate
- `163b6ce` feat: headless stream with labwc compositor
- `04d3cb0` feat: Steam/Lutris/Heroic game import scanner
- `b1dc4fd` feat: add krfb-virtualmonitor backend for KDE headless streaming
- `52d85d5` feat: add gamescope virtual display backend for Steam Deck
- `229f457` ui: relabel Adaptive Bitrate section as SolarFlare feature

### Web UI for new config keys

Exposed all new config options in the Web UI and provided defaults for `/api/config`. Updated the README with accurate counts and clarified the headless sections.

- `89c2880` feat: add web UI controls for all new config options
- `8b8ab78` fix: emit defaults for new config options in /api/config
- `ef13ba1` docs: add July 3, 2026 features to README
- `ed1af0d` docs: update README with accurate counts and clarified headless sections

### Issue template + build dep cleanup

Replaced LizardByte links in bug-report templates and added the libnuma cmake check.

- `05b0f67` fix: replace remaining LizardByte links in issue templates, add libnuma cmake check

### Daily bug check (Jul 3)

Bug audit pass on the Jul 3 feature batch plus the Reddit portal/KMS report. Fixed the adaptive bitrate first-sample clamp, the `get_target_bitrate` floor clobber, the headless teardown string-match, the `start_krfb` swallowed-output silent failure, the `stop_labwc` / `stop_gamescope` discarded exit status, the Steam scanner missing Flatpak and Snap installs, the Lutris `.yml`-as-launcher bug, the KMS non-numeric monitor-index garbage, the KMS segfault on hot-swap, and the XDG portal dropping caps before init.

- `04a029e` fix: daily bug check on Jul 3 features + Reddit portal/KMS bug report


## 2026-07-04

### Daily bug check (Jul 4)

Bug audit pass covering the Adaptive Bitrate, headless compositor, Steam/Lutris scanner, and KMS paths. All `test_sunshine` runs (464 tests) pass with zero failures. Also collapsed the duplicate `Quick install` block in the README into one.

- `27f8604` docs: changelog entry for July 4 2026 bug-fix batch
- `f5d2500` docs: collapse duplicate Quick install blocks into one


## 2026-07-05

### Morning sweep (Jul 5)

Daily sweep pass two: dead-state removal, naming cleanups, diagnostic logs. Hermes-KMS fd-safety: don't close the FD if `acquire_hermes_kms()` failed; adaptive bitrate recovery arming via a "recovery mode" that ramps the bitrate back up after sustained low packet loss.

- `49e3924` chore: daily sweep pass two — dead-state removal + naming + diagnostic logs
- `554a6dd` fix: daily sweep — adaptive bitrate recovery arming + Hermes-KMS fd safety
- `fb9d410` fix: morning sweep 2026-07-05
- `8d57027` fix: morning sweep 2026-07-05 (batch 2)
- `532adef` chore: gitignore *.deb installer artifacts

### Hermes-KMS kernel module

Vendored Hermes-KMS from `github.com/MrOz59/Hermes-KMS` at `third-party/hermes-kms` (git submodule, GPL-2.0+). `scripts/cachyos-build.sh` runs `packaging/linux/redesign/install-hermes-kms.sh` after `cmake --install`; the script DKMS-installs `hermes_kms.ko` and loads it with `initial_enabled=1` so `HERMES-1` appears in the source selector. Requires kernel-headers + dkms. Removed the duplicate `src/platform/linux/hermes_kms_drm.h`; the C++ capture backend now includes the upstream UAPI header directly.

- `b41f01e` feat(packaging): ship Hermes-KMS kernel module, vendored + DKMS-installed
- `4041552` fix(packaging): make install-hermes-kms.sh idempotent on re-run
- `f9b932e` feat(platform/linux): Hermes-KMS capture backend stub
- `aa02709` feat(hermes-kms): wire capture loop via WAIT_FRAME → ACQUIRE_FRAME → DMA-BUF
- `cfa64a5` fix(hermes-kms): scan card nodes not render nodes
- `df4b979` fix(hermes-kms): wire display_names() dispatch + drop stale stub comments
- `c3cc2c9` fix(hermes-kms): wire auto-detect, enforce UAPI version, fail on missing caps
- `703e895` fix(hermes-kms): surface probe failure reason in verify_hermes_kms
- `b966939` fix: add hermes_kms forward declarations to misc.cpp
- `08dbf3d` feat(hermes-kms): real ioctl probing, always compiled in
- `db768a2` fix(hermes-kms): dispatch encode devices for vaapi/vulkan/cuda

### Hermes-KMS Web UI + README

Exposed Hermes-KMS in the Linux capture-backend dropdown, surfaced probe failure reason in `verify_hermes_kms`, and updated the README §19 capture-loop description to match the wired implementation (probe + WAIT_FRAME + ACQUIRE_FRAME + DMA-BUF push to encoder).

- `27eaba1` feat(ui): expose Hermes-KMS in the Linux capture-backend dropdown
- `b27f9e6` docs: README Hermes-KMS section reflects wired capture loop
- `75bfc04` docs: add Hermes-KMS section to README (probe works, capture loop TBD)
- `5383187` docs: remove Hermes-KMS stub from README feature list

### Tier-1 low-latency encoder changes

Vulkan encoder perf work exposing `vk_min_qp` / `vk_max_qp` in the web UI; small `power_dpm_force_performance_level` value copy fix in KMS monitor correlation; `portalgrab` no longer hardcodes `cursor_mode=2`.

- `e585039` feat(vulkan): expose vk_min_qp / vk_max_qp in web UI
- `c4b2ec4` perf(vulkan): Tier 1 low-latency encoder changes
- `dcfe619` fix(kms): copy missing viewport.width/height in monitor correlation
- `fbfb698` fix(portalgrab): stop hardcoding cursor_mode=2 (EMBEDDED)

### Config HTTP API + auth refactor

Added scoped bearer tokens for the config HTTP API and an HTTP control surface for the adaptive bitrate controller. Moved `auth_result_t` full definition into `confighttp.h`. Updated `test_confighttp.cpp` for the new return type. Doxygen for all new public types to satisfy `BUILD_WERROR=ON`.

- `f35cc6a` feat(api): scoped bearer tokens for the config HTTP API
- `04a806d` feat(api): HTTP control surface for adaptive bitrate
- `a8b2767` fix: move auth_result_t full definition into confighttp.h
- `a6ac96a` fix: update test_confighttp.cpp for new auth_result_t return type
- `560d9c2` fix: doxygen for new public types + ifdef guard for hermes_kms call site
- `65014c1` fix: doxygen param name matching for hermes_kms functions

### Fork redesign services

Ship the boot-time tuning systemd units (`cpu-performance`, `nic-tuning`, `nvidia-clock-lock`) from the repo via an idempotent `install-redesign-services.sh` installer. Drops each `.sh` helper into `/usr/local/sbin/` and each `.service` into `/etc/systemd/system/`.

- `6a7a5f2` feat(packaging): ship fork redesign services from the repo
- `36b5ea6` refactor(build): make Bazzite support more robust
- `40fb2ce` fix: Quick Install service file uses graphical-session.target, not default.target
- `be96330` docs(security): fix broken link to fork changelog
- `66bbd81` chore: gitignore third-party-check broken symlink
- `e1cb1dd` fix(build): trim Bazzite/Fedora package lists to only what cmake needs

### screenshot helper script

New `scripts/screenshot-ui.sh` for capturing README screenshots from a live web UI session.

- `857e7e3` scripts: add screenshot-ui.sh for README screenshots

### README + changelog

Synced README badges, version, and changelog with the actual fork state. Updated the changelog entry for the July 4 bug-fix batch and the July 5 features batch.

- `46a490e` docs: update README badges with latest version and commit count
- `1dc43af` docs: fix README version badge alt text
- `cb6a623` docs: fix README version badge URL and alt text for shields.io compatibility
- `8a57820` docs: update changelog for July 4, 2026
- `c09aa0c` docs: update README and CHANGELOG for July 5 features


## 2026-07-06

### Stream port alignment + audio socket

Aligned stream port offsets with what Moonlight actually expects relative to `https_port`. `VIDEO_STREAM_PORT` 9→16, `CONTROL_PORT` 10→26, `AUDIO_STREAM_PORT` 11→27, dropping the now-unneeded `HTTPS_PORT_OFFSET`. Empty `nvhttp.cert` defaults + removed the blocking `set_options(no_tlsv1, no_tlsv1_1)` on the TLS context (was preventing the SSL_CTX from completing a handshake).

- `e7cee2c` fix(stream): align stream ports with Moonlight client's https_port+offset math
- `e90671b` fix(certs+stream): empty nvhttp.cert defaults; remove blocking set_options; align stream ports
- `30ed0f0` fix(certs): persist TLS cert/key in appdata/credentials/, survive reboots
- `bc4de22` fix(stream): give audio its own UDP port instead of colliding with control

### Pairing session + PIN handling

Reset stale pairing session on retry, safe `pin()` lookup, `not_found()` double-write fix, and held PIN response sets `close_connection_after_response`.

- `a30b780` fix(nvhttp): reset stale pairing session on retry + safe pin() lookup
- `c91852b` fix(nvhttp): set close_connection_after_response on held PIN response

### Video capture threading

Dropped `SCHED_RR` from the video capture thread to reduce scheduling overhead on multi-core hosts.

- `d6f7040` fix(platform/linux): drop SCHED_RR from the video capture thread

### X11 touchscreen

Added a udev rule to ensure the touchscreen device is recognized on X11; reverted after it broke touch on real hardware; the touch input path no longer sets `INPUT_PROP_DIRECT` on the uinput device, restoring X11 pointer emulation.

- `9568df1` fix(x11): add udev rule to ensure touchscreen device is recognized
- `db76492` revert: remove udev rule that was breaking touch on X11
- `43ec7fc` fix(x11): remove INPUT_PROP_DIRECT from uinput touch device

### Process detach

Detached app sessions now stay alive indefinitely (removed the 5-second disconnect loop).

- `e976415` fix(proc): keep detached app sessions alive indefinitely

### README + issue #6

Added Solar-Flare update instructions to the README (closes #6), synced badges, fixed the install command, and documented the loading-screen workaround.

- `e763d84` fix: closes #6 add Solar-Flare update instructions to README
- `567c3ab` docs: fill in 8060cf3 SHA in issue #8 FAQ entry
- `e8e54e6` docs: sync README test counts and commit badge with current state
- `c8f99e6` docs: fix README uninstall command and document loading-screen workaround
- `c21920c` chore: sync README badges, version, and URLs with actual fork state

### Version bump to 2026.999.2

Bumped stale `2026.999.0` references.

- `c02fa27` chore: bump stale 2026.999.0 references to 2026.999.2


## 2026-07-07

### Morning sweep (Jul 7)

Daily sweep pass: removed PUSH-INSTRUCTIONS.md from the repo, gitignored `crush.db`, and fixed submodule drift.

- `d143cb1` chore(readme): bump commits badge to 213, sync version badge to 80-91d5a71-dirty
- `95497d4` fix: morning sweep 2026-07-07
- `8c4d1f3` chore(sweep): remove PUSH-INSTRUCTIONS.md, gitignore crush.db, fix submodules


## 2026-07-08

### Morning sweep (Jul 8)

Daily sweep pass: synced README version + test badges after the niri PR; added niri (Smithay) Wayland compositor support with auto-detect; fixed `search_path` false-positive in compositor detection (niri / gamescope / kwin_wayland presence on `$PATH` no longer implies a running session).

- `62716a1` chore(readme): sync version + tests badges after niri PR
- `543eed3` chore(readme): update commits badge to actual count
- `ad88c4b` feat(headless): add niri (Smithay) Wayland compositor support
- `ab0efcc` fix: morning sweep 2026-07-08
- `89d6f79` fix(headless): drop search_path false-positive in compositor detect

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

- `05cb358` fix: morning sweep 2026-07-09

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

- `6331098` fix: issue #9 build fails on missing wayland-protocols submodule

### Version alignment + README rewrite

Aligned the internal build version with the GitHub release tag scheme (`YYYY.DDD.N-<n>-gdbf8232` instead of `<commit-count>-<sha>`). Rewrote the README preamble in conversational style; README no longer reads like a spec sheet.

- `e67d79e` fix(version): align internal build version with GitHub release tag
- `ce0697e` docs(readme): rewrite preamble in conversational style

### SUN_ERR tagged error log

New `src/error.h` + `src/error.cpp` expose `error_category_e`, `encode_error_e`, and the `SUN_ERR(cat, tag, msg)` macro. Every error log line is now self-correlated with `__FILE__:__LINE__:__func__`, a category tag, and a per-category atomic counter. `platf::video::encode()` and `encode_nvenc()` return `std::optional<encode_error_e>` so call sites can branch on the specific cause (`EMPTY_PACKET`, `FRAME_INDEX_MISMATCH`, `UNSUPPORTED_SESSION`) instead of the previous generic "Could not encode video packet" message.

- `5d113c3` feat(error): SUN_ERR() tagged error log + process-wide counters
- `6cd3046` feat(error): expose /api/errors, encode_error_e on every encoder path

### `/api/errors` HTTP endpoint

New `getErrors()` handler exposes `sunshine::counters()` as JSON via `GET /api/errors`. Gated behind `api_scope_t::LOGS_GET` (same scope as `/api/logs`). Response body has one field per category plus `total`. The Web UI diffs against a snapshot to compute recent-error rate.

- `6cd3046` feat(error): expose /api/errors, encode_error_e on every encoder path

### Standalone test runner

`tests/run_test_error.sh` bypasses the cmake test target (which OOMs the 16 GB box on full `test_sunshine` link) by compiling only `src/error.cpp` + `tests/unit/test_error.cpp` + `gtest_main.cc` against the static libs already on disk in `cmake-build-test/_deps/boost-build/libs/`. Peak RAM ~300 MB, runtime ~5 s. 4 unit tests assert the public contract (counter routing + stable string mappings) without requiring a boost log sink. The 3 tests that called `log_error()` → `BOOST_LOG(error)` were removed in favour of directly bumping the atomic counters, since the boost log plumbing isn't our code.

- `718fb65` test(error): standalone runner + counter-only tests

### Cert persistence on restart

Fixed the long-standing "I have to re-pair after every restart" bug. `http::init()` used to take the "empty cert in config" branch on every start and generate a random new `unique_id`, invalidating every paired client. The fix scans `appdata/credentials/` for existing `pkey-*` files, picks the newest by mtime, and adopts that pair. One re-pair required after upgrade (the client certs in `sunshine_state.json` don't match the adopted server cert). After that, every restart preserves the cert.

- `7fde0ef` fix(certs): adopt existing pkey/cert on restart instead of generating

### CI cleanup: drop ci-copr.yml

The CachyOS-only COPR integration workflow had no secrets on the fork, failed on every release push, and was the source of release-time failure emails. Deleted the file (already `disabled_manually` on the GitHub UI). The watch on `vindeckyy/Solar-Flare` is also set to `ignored` via the API, so no further activity emails come from this repo.

- `a957ed1` chore(ci): delete ci-copr.yml, the source of release-time failure emails


---

## 2026-07-13/14

### Morning sweep (Jul 13–14)

General cleanup batch post-release: fixed the CONFIGURATION.md drift caught by the docs-drift agent (tunable count, `virtual_display_resolution` claim, stale file refs), added a `release.sh` script as the single source of truth for version bumps, fixed an RTSP OOB-read in the frame parser (fuzzer find), and patched a GVariant-interned string double-free in the heap path. Also probed for ccache/mold/lld during cmake, added GPL license headers, released capture resources on teardown, and documented the linux resource cleanup.

- `c45af00` fix(ci): pin action SHAs in release.yml for supply-chain security
- `7819d10` docs: add doxygen briefs for AdaptiveBitrate::config_t, ctor, reset(), and adaptive_bitrate_net_stats mail
- `75c83f9` fix(heap): double-free on GVariant-interned strings from g_autofree
- `af2dfea` fix(build): probe for ccache and mold/lld; fall back when missing
- `fa67cdf` fix(docs): correct CONFIGURATION.md drift (virtual_display_resolution, key count, file refs)
- `8b62aab` fix(ci): pin actions/checkout@v4 to SHA in release.yml
- `1bbcff3` chore(gitignore): cover editor backups, .env*, and broader *.log
- `f38f9d4` fix(headless): detect KDE when XDG_CURRENT_DESKTOP=plasma
- `8e52adf` fix(license): add GPL headers to project sources
- `b5c1525` docs(linux): document resource cleanup
- `c799f07` fix(rtsp): reject unterminated frames before parser (closes fuzzer OOB)
- `3c11889` fix(linux): release capture resources
- `8e3e1fd` feat(scripts): add release.sh — single source of truth for version bumps
- `1cdcf52` fix(docs): align README version badge with latest release
- `9c97ec9` fix(tests): resolve src/file_handler CWD leak + add doxygen briefs
- `8ff492c` docs(readme): sync version badge with CMakeLists.txt 2026.999.2
- `73b592f` docs: scrub stale info; catch cmake scratch in .gitignore

---

## See also

- [docs/CONFIGURATION.md](CONFIGURATION.md) — the 9 fork-specific latency/display toggles (`busy_poll_us`, `rate_cap_pct`, `enet_4mib_buffer`, `pipewire_latency_ms`, `cpu_pinning`, `dscp_qos`, `gpu_governor`, `headless_virtual_display`, `skip_wayland_correlation`).
- [docs/PORTING.md](PORTING.md) — per-distro package translation table for the build script.
- [README.md](../README.md) — fork entry point.
- [cachyos-fastpath.patch](../cachyos-fastpath.patch) — the original 7-file latency-tuning patch (kept as a historical artifact).

## Full commit index (commits not in the curated sections)

### 2026-07-08

- `4939947` docs(changelog): append full commit index (95 commits, by date)
- `be2ede5` docs(changelog): order full commit index chronologically (oldest first)
- `ad885b9` docs(changelog): reorder curated release sections to chronological order
