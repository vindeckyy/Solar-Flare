# Agent and automation instructions

This file defines **non-negotiable project conventions** for Cursor agents,
CI bots, and other automation working in the SolarFlare tree. Human
contributors should also follow these rules; the full contributor narrative is
in [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/contributing.md](docs/contributing.md).

---

## Repository identity

- **Fork home:** [`vindeckyy/Solar-Flare`](https://github.com/vindeckyy/Solar-Flare)
- **Upstream:** [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) (compatibility baseline, not the PR target for fork work)

---

## Fork vs upstream - issues and pull requests

**Never create issues or pull requests in the LizardByte GitHub organization.**

| Action | Correct target |
|---|---|
| SolarFlare bug, feature, or PR | `vindeckyy/Solar-Flare` |
| Issue reproduced in stock Sunshine | `LizardByte/Sunshine` (report only - do not open SolarFlare PRs there) |
| Agent asked to "open a PR" | User's fork or `vindeckyy/Solar-Flare` per task context |

If asked to create an issue or pull request for SolarFlare work, do so in the
**fork**, not LizardByte.

---

## Build directories

**Prefix all CMake build directories with `cmake-build-`.**

Examples: `cmake-build-dev`, `cmake-build-tests`, `cmake-build-release`,
`cmake-build-docs`.

Do not use bare `build/`, `out/`, or other prefixes for local developer trees
unless a script explicitly requires it (CI may use `build/` internally).

---

## Windows compilation (MSYS2 UCRT64)

On Windows, compile inside **MSYS2 UCRT64**. Prefix commands with:

```text
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c
```

Example:

```powershell
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "cmake -S . -B cmake-build-dev -G Ninja -DBUILD_TESTS=ON -DBUILD_DOCS=OFF && cmake --build cmake-build-dev --target test_sunshine -j2"
```

---

## Tests (GoogleTest)

- **Framework:** [GoogleTest](https://github.com/google/googletest) (`gtest` / `gmock`)
- **Executable name:** `test_sunshine` (compatibility name - do not rename)
- **Location:** inside the `tests/` directory **within the build directory**

```text
<cmake-build-dir>/tests/test_sunshine          # Linux / macOS
<cmake-build-dir>/tests/test_sunshine.exe      # Windows
```

Typical workflow:

```bash
cmake -S . -B cmake-build-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=OFF
cmake --build cmake-build-tests --target test_sunshine -j2
./cmake-build-tests/tests/test_sunshine --gtest_brief=1
```

### Coverage requirement

**Add or update tests for all new or modified methods and code. Target 100%
coverage on changed code** (lines and branches where feasible).

Use `--gtest_filter` while developing; run the full suite before claiming
completion.

---

## C++ style

Always follow the style guidelines in [`.clang-format`](.clang-format).

Verify:

```bash
clang-format --dry-run --Werror path/to/changed.cpp
```

---

## Doxygen documentation

**The project requires that everything be documented in Doxygen or the build will
fail** when `BUILD_DOCS=ON`.

Doxygen settings (`third-party/doxyconfig/doxyconfig-Doxyfile`):

- `WARN_IF_UNDOCUMENTED = YES`
- `WARN_AS_ERROR = FAIL_ON_WARNINGS`

**Always add or update Doxygen** for new or modified functions, types, members,
enums, and public constants.

### Primary block comments

```cpp
/**
 * @brief Describe the function, structure, etc.
 *
 * @param my_param Describe the parameter.
 * @return Describe the return.
 */
```

### Inline member comments

Use `///< ...` - **not** `/**< ... */`.

---

## Localization

When adding or changing user-visible strings in the Web UI:

- Update **only** `src_assets/common/assets/web/public/assets/locale/en.json`
- Do **not** update any other language
- Do **not** add `en-US`, `en_GB`, or other English variants

---

## Documentation edits

- Add or update **Doxygen** for C/C++ API changes.
- Add or update **user/operator docs** under `docs/` when behavior or
  configuration changes.
- Maintainer-only procedures live under `docs/maintainers/` (excluded from the
  public Doxygen site).
- When adding localization, follow the **en-only** rule above.

---

## Common CMake flags

| Flag | Typical local value | Notes |
|---|---|---|
| `BUILD_TESTS` | `ON` when developing C++ | Builds `test_sunshine` |
| `BUILD_DOCS` | `OFF` for fast iteration; `ON` before merge if API touched | Enables Doxygen target |
| `ENABLE_COVERAGE` | `ON` when validating coverage | Used with `gcovr` |
| `CMAKE_BUILD_TYPE` | `Debug` for tests; `Release` for perf binaries | |

---

## CI awareness

Default fork CI (`.github/workflows/ci.yml`):

1. Web UI production bundle (`npm run build`)
2. Linux compile + `test_sunshine` under Xvfb + coverage artifact upload

Release binaries are **not** built in CI. See `docs/maintainers/release.md`.

---

## Quick pre-submit commands

```bash
git diff --check
clang-format --dry-run --Werror $(git diff --name-only -- '*.cpp' '*.h' '*.c' '*.hpp')
./cmake-build-tests/tests/test_sunshine --gtest_brief=1
npm run build   # when Web UI files changed
```

---

## Related documents

| Document | Purpose |
|---|---|
| [CONTRIBUTING.md](CONTRIBUTING.md) | Policy, PR checklist, fork boundaries |
| [docs/contributing.md](docs/contributing.md) | Detailed development guide |
| [docs/building.md](docs/building.md) | Platform dependencies |
| [docs/maintainers/release.md](docs/maintainers/release.md) | Release process (maintainers) |
