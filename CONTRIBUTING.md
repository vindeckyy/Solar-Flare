# Contributing to SolarFlare

SolarFlare accepts focused, testable changes that improve the Linux streaming
path, host operations, or Web UI without breaking Moonlight compatibility.

This document is the **policy layer**: scope, fork boundaries, review
expectations, and where to send work. The **implementation layer** - toolchain
setup, C++ style, Doxygen, localization, test commands, and pre-submit
checks - lives in [docs/contributing.md](docs/contributing.md).

---

## Table of contents

1. [Fork vs upstream](#fork-vs-upstream)
2. [Before opening a change](#before-opening-a-change)
3. [Areas maintained by SolarFlare](#areas-maintained-by-solarflare)
4. [Development workflow](#development-workflow)
5. [Configure and build](#configure-and-build)
6. [Test and coverage](#test-and-coverage)
7. [Format and document](#format-and-document)
8. [Web UI changes](#web-ui-changes)
9. [Continuous integration](#continuous-integration)
10. [Pull request checklist](#pull-request-checklist)
11. [Reporting problems](#reporting-problems)

---

## Fork vs upstream

SolarFlare is a **Linux-focused fork** of
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine). The fork
preserves Moonlight compatibility identifiers (`sunshine`, ports, config paths,
service names) while shipping SolarFlare-specific transport tuning, capture
behavior, security hardening, and Web UI work.

| Question | Where it belongs |
|---|---|
| SolarFlare-specific behavior, regressions, or integrations | This repository - [`vindeckyy/Solar-Flare`](https://github.com/vindeckyy/Solar-Flare) |
| Defect reproduced in an **unmodified** upstream Sunshine build | [LizardByte/Sunshine issues](https://github.com/LizardByte/Sunshine/issues) |
| Pull requests for SolarFlare work | Open against **`vindeckyy/Solar-Flare`**, never the LizardByte organization |
| Issues or PRs in LizardByte repos for fork work | **Do not create them** - agents and contributors must use the fork |

**Practical rule:** If the bug exists in stock Sunshine and is not caused by a
SolarFlare-only code path, report or fix it upstream first. If the change is
fork-specific - or you are unsure but the reproduction uses a SolarFlare
build - work here.

Upstream-derived platform workflows (macOS, Windows, FreeBSD, Homebrew,
Flatpak, Copr, and similar) remain in the tree for inheritance but are not the
fork's primary maintenance surface. The default CI for this repository runs the
**Web bundle** job and **Linux build and tests** only (see
[`.github/workflows/ci.yml`](.github/workflows/ci.yml)).

---

## Before opening a change

1. **Search** existing issues and recent commits for overlapping work.
2. **Separate** SolarFlare-specific behavior from inherited Sunshine behavior.
3. **Preserve** protocol identifiers, configuration paths, and executable/service
   compatibility unless the change includes an explicit migration plan.
4. **Add or update tests** for every modified behavior (see
   [Test and coverage](#test-and-coverage)).
5. **Update Doxygen and user documentation** in the same change when symbols or
   operator-visible behavior change.

SolarFlare-specific work belongs in this repository. A defect that reproduces
unchanged in upstream Sunshine should normally be reported upstream first; fork
integrations or regressions should be reported here.

---

## Areas maintained by SolarFlare

- **Linux transport tuning:** pacing, busy polling, socket buffers, QoS, and
  adaptive bitrate.
- **Capture and scheduling:** CPU affinity, GPU governor behavior, headless
  paths, native compiler tuning, and optional performance services.
- **Video and audio controls:** NVENC presets, per-application overrides,
  Audio FX, and Opus configuration.
- **Host access:** scoped API tokens, trusted-subnet pairing, and security
  hardening around network-reachable surfaces.
- **The SolarFlare Web UI** and its responsive layout.
- **Multi-distribution build, install, release, and verification tooling.**
- **Fork documentation and regression contracts.**

The authoritative fork-control inventory is
[docs/CONFIGURATION.md](docs/CONFIGURATION.md); do not copy hard-coded setting
counts into new documents.

---

## Development workflow

Create a topic branch from `master` and keep each commit scoped to one logical
change. Use conventional commit subjects where practical, for example:

```text
feat(web-ui): add encoder health telemetry
fix(network): preserve pacing fallback on unknown links
docs(readme): clarify binary-only updates
test(config): cover fork-specific key parsing
```

### Branch hygiene

- Rebase or merge `master` before opening a PR if your branch is stale.
- Avoid mixing unrelated refactors with functional fixes.
- Do not commit generated build trees, credentials, local state, or unrelated
  submodule bumps.

---

## Configure and build

### Build directory naming

**All local build directories must use the `cmake-build-` prefix** (for example
`cmake-build-dev`, `cmake-build-tests`, `cmake-build-release`). This keeps
artifacts out of version control and matches project conventions documented in
[AGENTS.md](AGENTS.md).

### Linux and macOS

```bash
git submodule update --init --recursive

cmake -S . -B cmake-build-dev -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=OFF
cmake --build cmake-build-dev --target test_sunshine -j2
```

Use `-j2` (or another small parallelism value) when running tests on
memory-constrained hosts; the maintainer release path also caps parallel jobs
at two for reproducibility.

### Windows (MSYS2 UCRT64)

On Windows, compile from **MSYS2 UCRT64**. Prefix every shell command:

```powershell
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "<command>"
```

Example configure and test build inside that environment:

```powershell
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "cmake -S . -B cmake-build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_DOCS=OFF && cmake --build cmake-build-dev --target test_sunshine -j2"
```

The test executable on Windows is
`cmake-build-dev\tests\test_sunshine.exe`.

Platform-specific options and dependencies are documented in
[docs/building.md](docs/building.md) and [docs/PORTING.md](docs/PORTING.md).

---

## Test and coverage

SolarFlare uses **GoogleTest** (`gtest`). The test executable is always named
**`test_sunshine`** (retained for upstream build compatibility) and is produced
under the **`tests/` subdirectory of the build directory**:

```text
<cmake-build-dir>/tests/test_sunshine
```

### Running tests

```bash
./cmake-build-dev/tests/test_sunshine --gtest_brief=1
```

While iterating, filter to the narrowest relevant suite:

```bash
./cmake-build-dev/tests/test_sunshine --gtest_filter='ConfigTest.*' --gtest_brief=1
```

Before requesting review, run the **complete** suite:

```bash
./cmake-build-dev/tests/test_sunshine --gtest_brief=1
```

Hardware-dependent tests may **skip** when capture, encoder, audio, or input
devices are unavailable. **Failures** must be resolved or explicitly explained
in the PR description.

### Coverage expectation

**Target 100% coverage on changed code.** CI uploads `gcovr` XML from the Linux
workflow (see [`.github/workflows/ci-linux.yml`](.github/workflows/ci-linux.yml)).
Locally, configure with coverage flags when validating branch coverage:

```bash
cmake -S . -B cmake-build-coverage -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DENABLE_COVERAGE=ON \
  -DBUILD_DOCS=OFF
cmake --build cmake-build-coverage --target test_sunshine -j2
./cmake-build-coverage/tests/test_sunshine --gtest_brief=1
```

Then generate a report from the build tree (requires `gcovr`):

```bash
cd cmake-build-coverage
gcovr . -r ../src --exclude-noncode-lines --exclude-throw-branches \
  --exclude-unreachable-branches --html-details -o coverage.html
```

Every new or modified function, branch, and error path should have a test unless
the code is provably unreachable in the fork's supported configurations - and
that exception must be justified in the PR.

---

## Format and document

### C++ style (`.clang-format`)

All changed C and C++ files must conform to the repository
[`.clang-format`](.clang-format). Verify without modifying files:

```bash
clang-format --dry-run --Werror src/path/to/changed_file.cpp
```

Apply formatting when needed:

```bash
clang-format -i src/path/to/changed_file.cpp
```

Also run:

```bash
git diff --check
```

to catch trailing whitespace and conflict markers.

### Doxygen (required - build fails without docs)

When `BUILD_DOCS=ON`, the Doxygen configuration treats undocumented symbols and
warnings as **errors** (`WARN_IF_UNDOCUMENTED = YES`,
`WARN_AS_ERROR = FAIL_ON_WARNINGS` in
`third-party/doxyconfig/doxyconfig-Doxyfile`). **Every new or modified
function, type, member, enum value, and public constant needs documentation** or
the docs target fails.

Primary block comments:

```cpp
/**
 * @brief Describe the function, structure, etc.
 *
 * @param my_param Describe the parameter.
 * @return Describe the return.
 */
```

Inline member documentation uses `///< ...` (not `/**< ... */`).

Verify locally when touching documented surfaces:

```bash
cmake -S . -B cmake-build-docs -G Ninja -DBUILD_DOCS=ON -DBUILD_TESTS=OFF
cmake --build cmake-build-docs --target docs -j2
```

### Localization (`en` only)

When changing user-visible Web UI strings, update **only** the English catalog:

`src_assets/common/assets/web/public/assets/locale/en.json`

Do **not** edit `en_US`, `en_GB`, or any other locale file as part of a fork
change. Other languages are managed upstream or by separate localization
workflows.

---

## Web UI changes

The Web UI is a multi-entry Vite application rather than a client-side routed
SPA. Preserve these contracts:

| Component | Responsibility |
|---|---|
| `Navbar.vue` | Semantic navigation shell (desktop rail + compact mobile nav) |
| `init.js` | Theme and locale initialization for every entry point |
| `sunshine.css` | Shared responsive design system (filename is a compatibility path) |
| Vue SFCs | Interactive configuration surfaces |
| EJS templates | Static page shells |

Additional rules:

- Network endpoints and form serialization must remain **independent** from the
  visual layer.
- Motion must communicate interaction or state; avoid ambient looping effects.
- Every transition needs a usable `prefers-reduced-motion` fallback.
- User-facing product copy should say **SolarFlare**. Compatibility identifiers
  and upstream links may retain Sunshine where technically required.

Build the Web UI through CMake so assets land in the correct build tree:

```bash
cmake --build cmake-build-dev --target web-ui -j2
```

For local Vite iteration:

```bash
npm install
npm run dev
```

Before submit, also run the production bundle check:

```bash
npm run build
```

If navigation or page layout changes, refresh all six README screenshots with
`scripts/screenshot-ui.sh`.

---

## Continuous integration

Pull requests and pushes to `master` trigger [`.github/workflows/ci.yml`](.github/workflows/ci.yml):

| Job | What it verifies |
|---|---|
| **Web bundle** (`ci-bundle.yml`) | `npm ci` and `npm run build` for the Web UI |
| **Linux build and tests** (`ci-linux.yml`) | Full Linux compile via `scripts/linux_build.sh`, then `test_sunshine` under Xvfb; uploads coverage XML |

CI uses `release_version: 0.0.0-ci` for fork builds. **Release binaries are not
produced in GitHub Actions**; see
[docs/maintainers/release.md](docs/maintainers/release.md).

The inherited `_common-lint.yml` workflow only runs for repositories under the
`LizardByte/` organization and is intentionally inert in this fork.

---

## Pull request checklist

Copy this list into your PR description and check each item:

- [ ] Clear problem statement and bounded scope
- [ ] New or changed behavior has **GoogleTest** coverage; **100% on changed code**
- [ ] `test_sunshine` passes locally (full suite, not only a filter)
- [ ] Doxygen updated; `BUILD_DOCS=ON` build succeeds when C++ API/docs change
- [ ] User/operator docs updated when behavior or configuration changes
- [ ] C/C++ files pass `clang-format --dry-run --Werror`
- [ ] `git diff --check` passes
- [ ] Web UI: `npm run build` succeeds when frontend files change
- [ ] Localization: only `en.json` touched for new strings
- [ ] No generated build output, credentials, local state, or unrelated submodule
  changes
- [ ] Upstream commits or issues linked when relevant
- [ ] PR opened against **`vindeckyy/Solar-Flare`**, not LizardByte

---

## Reporting problems

| Kind | Where |
|---|---|
| Product defects and feature requests | [SolarFlare issues](https://github.com/vindeckyy/Solar-Flare/issues) |
| SolarFlare-specific vulnerabilities | [Private security advisory](https://github.com/vindeckyy/Solar-Flare/security/advisories/new) |
| Defects reproduced in unmodified upstream Sunshine | [LizardByte/Sunshine issues](https://github.com/LizardByte/Sunshine/issues) |

Be precise. Include:

- Reproduction steps and logs (**secrets removed**)
- Host distribution and desktop session
- GPU, capture backend, and encoder
- Moonlight client version
- Whether the build is from source (`linux-install.sh`), a release binary, or CI

See also [SECURITY.md](SECURITY.md) for supported versions and disclosure
expectations.
