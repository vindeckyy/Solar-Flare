# Development Guide

The repository-level [contribution policy](../CONTRIBUTING.md) is the source of
truth for scope, review expectations, and fork boundaries. This page is the
**hands-on development reference**: toolchain setup, build conventions, testing,
documentation requirements, Web UI contracts, and the pre-submit workflow.

---

## Table of contents

1. [Fork and review expectations](#fork-and-review-expectations)
2. [Toolchain](#toolchain)
3. [Repository layout for contributors](#repository-layout-for-contributors)
4. [Configure and build](#configure-and-build)
5. [Running tests](#running-tests)
6. [Coverage](#coverage)
7. [C++ style and formatting](#c-style-and-formatting)
8. [Doxygen documentation](#doxygen-documentation)
9. [Localization](#localization)
10. [Web UI](#web-ui)
11. [Before submitting](#before-submitting)
12. [Troubleshooting the dev loop](#troubleshooting-the-dev-loop)

---

## Fork and review expectations

| Topic | Rule |
|---|---|
| Pull requests | Open against [`vindeckyy/Solar-Flare`](https://github.com/vindeckyy/Solar-Flare) |
| LizardByte org | Do **not** open SolarFlare issues or PRs there |
| Upstream bugs | Reproduce on stock Sunshine first; report at [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine/issues) |
| Tests | Required for all behavior changes; **100% coverage on changed code** |
| Docs | Doxygen for C++ API; user docs when operators see a difference |

Inherited cross-platform build docs remain in [Building](building.md) and
[Porting SolarFlare](PORTING.md). SolarFlare's maintained install path is
[`scripts/linux-install.sh`](../scripts/linux-install.sh).

---

## Toolchain

SolarFlare uses:

| Component | Version / notes |
|---|---|
| CMake | ≥ 3.20 required; ≥ 4.0 preferred for `linux_build.sh` / CI |
| Ninja | Recommended generator |
| C++ | C++23 (`CMAKE_CXX_STANDARD 23`) |
| Node.js | Web UI build (see root `package.json`) |
| Vue + Vite | Multi-entry Web UI in `src_assets/common/assets/web/` |
| GoogleTest | `third-party/googletest`; executable named `test_sunshine` |
| Doxygen + Graphviz | Required when `BUILD_DOCS=ON` |
| Python + uv | CI tooling and some scripts (`uv.lock` in repo root) |

### Build directory naming

Keep **all** local build trees under a **`cmake-build-` prefix**:

```text
cmake-build-dev
cmake-build-tests
cmake-build-release
cmake-build-docs
cmake-build-coverage
```

This matches [AGENTS.md](../AGENTS.md) and keeps `.gitignore` rules effective.

### Windows (MSYS2 UCRT64)

Upstream Windows builds expect the **UCRT64** environment. From PowerShell or
cmd, wrap commands:

```powershell
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "<command>"
```

Install dependencies per [Building - Windows](building.md#windows). Prefer the
official Windows Doxygen installer over the MSYS package when building docs
(GCC-built Doxygen can fail with Graphviz).

---

## Repository layout for contributors

| Path | What you touch |
|---|---|
| `src/` | Host core: streaming, capture, encode, config, platform glue |
| `src_assets/common/assets/web/` | Web UI (Vue, CSS, locale, EJS shells) |
| `tests/` | GoogleTest suites and regression contracts |
| `cmake/` | Build system modules and compile definitions |
| `docs/` | Operator and developer Markdown (this file) |
| `scripts/` | `linux-install.sh`, `release.sh`, `screenshot-ui.sh`, CI helpers |
| `packaging/` | Linux services, Flatpak, performance unit files |
| `third-party/` | Vendored deps - avoid drive-by version bumps |

Fork-specific configuration keys and semantics:
[CONFIGURATION.md](CONFIGURATION.md).

---

## Configure and build

### Initial clone

```bash
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
git submodule update --init --recursive
```

### Fast iteration (tests on, docs off)

```bash
cmake -S . -B cmake-build-dev -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=OFF

cmake --build cmake-build-dev --target test_sunshine -j2
```

### Release-shaped binary (no tests)

```bash
cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF \
  -DBUILD_DOCS=OFF

cmake --build cmake-build-release --target sunshine web-ui -j2
```

### Docs verification build

Run before merging C++ API or header changes:

```bash
cmake -S . -B cmake-build-docs -G Ninja \
  -DBUILD_DOCS=ON \
  -DBUILD_TESTS=OFF

cmake --build cmake-build-docs --target docs -j2
```

Doxygen uses `third-party/doxyconfig/doxyconfig-Doxyfile` with
`WARN_IF_UNDOCUMENTED = YES` and `WARN_AS_ERROR = FAIL_ON_WARNINGS`. Undocumented
or malformed comments **fail the build**.

### Embedding version metadata (advanced)

`cmake/prep/build_version.cmake` reads optional environment variables:

| Variable | Purpose |
|---|---|
| `BRANCH` | Git branch name (CI sets `github.ref_name`) |
| `BUILD_VERSION` | Chronological build version (`YYYY.MDD.REVISION`) |
| `COMMIT` | Full git commit hash embedded in the binary |

Maintainer release builds set these so `sunshine --version` matches the
compatibility tag. Local dev builds infer metadata from `git describe`.

---

## Running tests

The test project is `tests/`; CMake target and output binary: **`test_sunshine`**.

```text
<build-dir>/tests/test_sunshine
```

### Full suite

```bash
./cmake-build-dev/tests/test_sunshine --gtest_brief=1
```

### Filtered runs

```bash
./cmake-build-dev/tests/test_sunshine \
  --gtest_filter='NetworkTest.*' \
  --gtest_brief=1
```

### List tests without running

```bash
./cmake-build-dev/tests/test_sunshine --gtest_list_tests
```

### CI parity

GitHub Actions runs tests from `build/tests/` after `scripts/linux_build.sh`,
with `DISPLAY=:1` and Xvfb. If a test fails only on CI, reproduce headlessly:

```bash
export DISPLAY=:1
Xvfb ${DISPLAY} -screen 0 1024x768x24 &
sleep 2
./cmake-build-dev/tests/test_sunshine --gtest_brief=1
```

### Skips vs failures

Some tests skip when hardware (GPU encoder, capture device, audio device) is
absent. **Skips are acceptable** when documented. **Failures are not** - fix or
explain in the PR.

The `test_sunshine` filename is intentional upstream compatibility; do not rename
the target.

---

## Coverage

Policy: **100% coverage on changed code** (lines and decision branches).

### Local coverage build

```bash
cmake -S . -B cmake-build-coverage -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DENABLE_COVERAGE=ON \
  -DBUILD_DOCS=OFF

cmake --build cmake-build-coverage --target test_sunshine -j2
./cmake-build-coverage/tests/test_sunshine --gtest_brief=1
```

`ENABLE_COVERAGE` adds `-fprofile-arcs -ftest-coverage` (see
`tests/CMakeLists.txt`).

### Generate HTML report

From the build directory (install `gcovr` via pip/uv if needed):

```bash
cd cmake-build-coverage
gcovr . -r ../src \
  --exclude-noncode-lines \
  --exclude-throw-branches \
  --exclude-unreachable-branches \
  --html-details -o coverage.html
```

CI uploads XML using the same exclusion flags (see
`.github/workflows/ci-linux.yml`).

### What to test

| Change type | Minimum testing |
|---|---|
| New public function | Direct unit tests + edge cases |
| Bug fix | Regression test reproducing the bug |
| Config parsing | Table-driven tests for valid/invalid keys |
| Web UI logic | Build verification; add JS tests if logic is extracted |
| Documentation-only | N/A - no coverage required |

---

## C++ style and formatting

Apply [`.clang-format`](../.clang-format) to every touched `.c`, `.cpp`, `.h`,
and `.hpp` file. The style is LLVM-based with project-specific wrapping and
indent rules.

Check without writing:

```bash
clang-format --dry-run --Werror src/path/to/file.cpp
```

Apply:

```bash
clang-format -i src/path/to/file.cpp
```

Also verify:

```bash
git diff --check
```

Do not reformat unrelated files in the same commit.

---

## Doxygen documentation

**Every** new or modified function, class, struct, enum, typedef, namespace
function, and public member needs documentation when `BUILD_DOCS=ON`.

### Primary blocks

```cpp
/**
 * @brief Short imperative summary.
 *
 * Longer description when behavior is non-obvious.
 *
 * @param fd Connected socket descriptor.
 * @return Zero on success, negative errno on failure.
 */
int configure_socket(int fd);
```

### Inline members

```cpp
int retry_count = 3;  ///< Maximum reconnect attempts before giving up.
```

Use `///<` for inline documentation - **not** `/**< ... */`.

### Common failures

| Symptom | Fix |
|---|---|
| `warning: documented symbol 'foo' was not declared` | Comment attached to wrong declaration |
| Undocumented parameter | Add `@param` for each parameter |
| Undocumented return | Add `@return` (or `@retval` list) |
| Build warns on `@brief` alone for struct members | Document each public member |

Public headers live under `src/`. When adding fork-specific types, document
compatibility constraints (Moonlight, config paths) in the brief.

---

## Localization

English source strings belong **only** in:

```text
src_assets/common/assets/web/public/assets/locale/en.json
```

Rules:

1. **Do not** edit `en_US`, `en_GB`, or any non-`en` locale in fork PRs.
2. Use stable, descriptive keys (`settings.network.pacing.label`).
3. Product copy says **SolarFlare** in user-visible strings.
4. Keep internal keys like `sunshine_name` when renaming would break config or
   translation tooling.

The inherited `localize.yml` workflow targets upstream Crowdin integration and
is not part of the fork's en-only contribution path.

---

## Web UI

Source root: `src_assets/common/assets/web/`.

### Architecture contracts

| File / area | Contract |
|---|---|
| `Navbar.vue` | Shared desktop rail and compact mobile navigation |
| `sunshine.css` | Design tokens, layout, responsive breakpoints (name is legacy) |
| `init.js` | Theme + locale bootstrap on every HTML entry |
| `*.vue` pages | Feature UI; no embedded routing framework |
| `views/*.ejs` | Server-rendered shells; Vite bundles per page |

The UI **deliberately avoids a client-side router**. Each settings area is its
own entry point. Keep fetch URLs, CSRF handling, and form field names stable.

### Motion and accessibility

- Motion communicates state (panel open, save success) - not decoration.
- Respect `prefers-reduced-motion: reduce`.
- Maintain keyboard focus order in `Navbar.vue` overlays.

### Build paths

Production bundle via CMake (matches installed assets):

```bash
cmake --build cmake-build-dev --target web-ui -j2
```

Vite dev server (hot reload):

```bash
npm install
npm run dev
```

Production check (matches CI `ci-bundle.yml`):

```bash
npm ci --ignore-scripts
npm run build
```

### Screenshots

Navigation or layout changes require refreshing README screenshots:

```bash
./scripts/screenshot-ui.sh
```

Commit updated images under `docs/images/`.

---

## Before submitting

Run this sequence from the repository root:

```bash
# Whitespace / conflict markers
git diff --check

# C++ format (adjust file list)
clang-format --dry-run --Werror $(git diff --name-only -- '*.cpp' '*.h' '*.c' '*.hpp')

# Tests
./cmake-build-tests/tests/test_sunshine --gtest_brief=1

# Web UI (when src_assets/.../web changed)
npm run build

# Doxygen (when src/ headers or comments changed)
cmake --build cmake-build-docs --target docs -j2
```

Document user-visible behavior in the appropriate guide (`docs/CONFIGURATION.md`,
`docs/getting_started.md`, README, etc.).

Open the pull request against **`vindeckyy/Solar-Flare`**. Use the checklist in
[CONTRIBUTING.md](../CONTRIBUTING.md#pull-request-checklist).

---

## Troubleshooting the dev loop

| Problem | Likely cause | What to try |
|---|---|---|
| `test_sunshine` not found | Wrong build dir or `BUILD_TESTS=OFF` | Reconfigure with `-DBUILD_TESTS=ON`, build target `test_sunshine` |
| Doxygen fails on warning | Missing `@param` / `@return` | Fix comments; rebuild `docs` target |
| Web UI stale in browser | Served from old build tree | Rebuild `web-ui`; hard-refresh; check `cmake-build-*/src_assets` |
| Submodule errors | Shallow clone | `git submodule update --init --recursive` |
| Windows link errors | Wrong shell (not UCRT64) | Use `msys2_shell.cmd ... -ucrt64 -c` |
| Coverage lower than expected | Uncovered error branch | Add negative test case |

For platform packages and distro-specific deps, see [Porting](PORTING.md).

<div class="section_buttons">

| Previous                |                                                         Next |
|:------------------------|-------------------------------------------------------------:|
| [Building](building.md) | [Source Code](../third-party/doxyconfig/docs/source_code.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
