# SolarFlare Release Process

SolarFlare publishes **Linux x86-64 binaries** built and verified on the
maintainer's system for updating existing installs. **New users must build from
source** with [`scripts/linux-install.sh`](../../scripts/linux-install.sh).

> [!CAUTION]
> **GitHub Actions must not build release binaries.** Fork CI
> ([`.github/workflows/ci.yml`](../../.github/workflows/ci.yml)) compiles and
> tests on Linux for health checks only. Release artifacts are produced
> locally from a clean, tagged tree so embedded versions never carry a `-dirty`
> suffix and binary contents match maintainer verification.

---

## Table of contents

1. [Dual versioning](#dual-versioning)
2. [Release artifacts](#release-artifacts)
3. [Prerequisites](#prerequisites)
4. [Prepare](#prepare)
5. [Version and tag (`scripts/release.sh`)](#version-and-tag-scriptsreleasesh)
6. [Build and verify locally](#build-and-verify-locally)
7. [Generate SHA256SUMS](#generate-sha256sums)
8. [Publish manually](#publish-manually)
9. [Post-publish validation](#post-publish-validation)
10. [Rollback and hotfix](#rollback-and-hotfix)
11. [Release notes template](#release-notes-template)

---

## Dual versioning

SolarFlare intentionally maintains **two** synchronized version identifiers.
Examples below use placeholder *next* values so commands stay copy-paste templates
 -  replace with your actual versions.

| Role | Format | Example | Used for |
|---|---|---|---|
| **Display version** | SemVer `MAJOR.MINOR.PATCH` | `1.2.2` | GitHub release **title**, README shield, user-facing changelog headings |
| **Build version** | Chronological `YYYY.MDD.REVISION` | `2026.824.1` | CMake `PROJECT_VERSION`, Python package version, `sunshine --version`, compatibility **git tag** |

### Git tag

```text
v<build-version>-solarflare
```

Example: `v2026.824.1-solarflare`

- Tags **with** the `-solarflare` suffix are fork compatibility releases.
- Upstream-style tags without the suffix are **excluded** from
  `scripts/release.sh` duplicate detection.

### Where each version is stored

| File | Field updated by `release.sh` |
|---|---|
| `CMakeLists.txt` | `project(Sunshine VERSION <build>)` |
| `pyproject.toml` | `version = "<build>"` |
| `uv.lock` | `solarflare` package version |
| `README.md` | Release badge, current release table, asset section heading |
| `docs/CHANGELOG-SolarFlare.md` | Dated section with both versions |
| Git annotated tag | `v<build>-solarflare` message uses display version |

The **executable** reports the build version at runtime (`PROJECT_VERSION` in
logs and `--version`). Moonlight clients and update tooling depend on that
chronological identifier remaining monotonic.

### Choosing the next build version

Pattern: `YYYY.MDD.REVISION`

- `YYYY` - four-digit year
- `MDD` - month without leading zero on the month, concatenated with day
  (August 24 → `824`)
- `REVISION` - incrementing patch within that calendar day (`1`, `2`, …)

`scripts/release.sh` refuses:

- Duplicate build versions already tagged as `v*-solarflare`
- Build versions **older than** the latest `v*-solarflare` tag (via `sort -V`)

Display version follows normal SemVer semantics for user communication only.

The current published release is documented in the repository
[README.md](../../README.md).

---

## Release artifacts

Each GitHub release publishes exactly **three** Linux x86-64 files:

| Asset | Contents | Consumer |
|---|---|---|
| `sunshine-x86_64` | Stripped ELF executable (compatibility name) | Manual binary swap / `latest/download/sunshine-x86_64` |
| `solarflare-linux-x86_64.tar.gz` | Executable, runtime libs layout, Web UI assets, icon, license | Web UI in-app updater |
| `SHA256SUMS` | Checksums for **both** files above | Updater integrity verification |

### Filename contracts

- `sunshine-x86_64` is **intentional** - SolarFlare keeps the Sunshine
  executable name for Moonlight and scripting compatibility.
- `solarflare-linux-x86_64.tar.gz` identifies the fork payload.
- **Do not rename** assets without updating the Web UI download logic and README.

### Capabilities note

A raw executable uploaded to GitHub **cannot** retain Linux file capabilities.
Release notes and install docs must instruct users to run after download:

```bash
sudo setcap 'cap_sys_admin,cap_sys_nice+p' /path/to/sunshine
```

KMS capture requires this; the tarball install path via
`solarflare-update-apply` (from `linux-install.sh`) handles capabilities when
installed through the maintained updater.

---

## Prerequisites

| Requirement | Why |
|---|---|
| Clean working tree | `release.sh` exits if `git status --porcelain` is non-empty |
| `master` synced with `origin/master` | Avoids tagging the wrong commit |
| Git identity configured | Commit and annotated tag authorship |
| `gh` CLI authenticated to `vindeckyy/Solar-Flare` | Manual release upload |
| Local Linux x86-64 toolchain matching supported distros | Artifact compatibility |
| Web UI + tests verified locally | Quality gate before version bump |

`release.sh` can auto-set `git user.name` / `user.email` only when
`~/.hermes/.env` contains `GITHUB_TOKEN` (Hermes automation path). Otherwise
configure git identity manually.

---

## Prepare

Complete this checklist **before** running `release.sh` without `--dry-run`:

1. **Sync branch**

   ```bash
   git checkout master
   git pull origin master
   ```

2. **Choose versions**
   - Next display SemVer (e.g. `1.2.3`)
   - Next chronological build version (e.g. `2026.824.2`)

3. **Verify quality gates**
   - Web UI production build succeeds (`npm run build` or CMake `web-ui` target)
   - GoogleTest suite passes with at most **two** parallel build jobs (`-j2`)

   ```bash
   cmake -S . -B cmake-build-release-prep -G Ninja \
     -DCMAKE_BUILD_TYPE=Release \
     -DBUILD_TESTS=ON \
     -DBUILD_DOCS=OFF
   cmake --build cmake-build-release-prep --target test_sunshine -j2
   ./cmake-build-release-prep/tests/test_sunshine --gtest_brief=1
   ```

4. **Confirm clean tree**

   ```bash
   git status --short
   ```

   Output must be empty.

5. **Draft release notes** (`release-notes.md`)
   - List exact asset filenames
   - Include red GitHub `CAUTION` callout: new users → `./scripts/linux-install.sh`;
     binaries are **update-only**
   - Summarize user-visible changes since the previous SolarFlare tag

---

## Version and tag (`scripts/release.sh`)

Script path: [`scripts/release.sh`](../../scripts/release.sh)

### Usage

```bash
./scripts/release.sh <build-version> <display-version> [--dry-run] [--no-push]
```

| Flag | Behavior |
|---|---|
| *(none)* | Apply file edits, commit, tag, **push** `master` and tag |
| `--dry-run` | Validate versions and print plan; **no** file edits, commit, tag, or push |
| `--no-push` | Apply edits, commit, and tag locally; **do not** push |

### Recommended flow

**1. Preview**

```bash
./scripts/release.sh 2026.824.2 1.2.3 --dry-run
```

**2. Create local commit + tag**

```bash
./scripts/release.sh 2026.824.2 1.2.3 --no-push
```

**3. Build and verify artifacts** (next section) from the tagged commit.

**4. Push** only after verification:

```bash
git push origin master
git push origin v2026.824.2-solarflare
```

Or re-run without `--no-push` if the tree is already committed and tagged
(usually prefer explicit push after verification).

### What the script does

When not in `--dry-run`:

1. Validates build/display version regex (`N.N.N`)
2. Ensures build version is newer than latest `v*-solarflare` tag
3. Requires clean working tree
4. Updates `CMakeLists.txt`, `pyproject.toml`, `uv.lock`
5. Updates README release metadata (badge, table, tarball section heading)
6. Prepends changelog section to `docs/CHANGELOG-SolarFlare.md` if absent
7. Commits: `release: SolarFlare v<display> (build <build>)`
8. Creates annotated tag: `v<build>-solarflare` with message `SolarFlare v<display>`
9. Pushes `master` and tag when `PUSH=true` (default unless `--no-push`)

### Script failure modes

| Error | Resolution |
|---|---|
| `bad build version` | Use `YYYY.MDD.REVISION` three-part numeric form |
| `must be newer than` | Bump revision or date component |
| `working tree dirty` | Commit, stash, or discard local changes |
| `uv.lock SolarFlare package version not matched` | Ensure lockfile still names package `solarflare` |
| `README.md release metadata not matched` | README structure changed - update script patterns |

---

## Build and verify locally

Build from the **final tagged commit** after `release.sh --no-push`.

### Embed exact build identity

Export metadata so `cmake/prep/build_version.cmake` stamps the binary:

```bash
export BRANCH=master
export BUILD_VERSION=2026.824.2
export COMMIT=$(git rev-parse HEAD)
```

### Configure release build

Use a dedicated tree; keep the `cmake-build-` prefix:

```bash
cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF \
  -DBUILD_DOCS=OFF

cmake --build cmake-build-release --target sunshine web-ui -j2
```

Strip and stage the executable:

```bash
mkdir -p release-artifacts
strip -o release-artifacts/sunshine-x86_64 cmake-build-release/sunshine
chmod +x release-artifacts/sunshine-x86_64
```

Assemble the tarball (layout must match what the Web UI updater expects - mirror
the previous release structure: binary, bundled assets, icon, license):

```bash
# Example - adjust paths to match your install tree / packaging script output
tar -czf release-artifacts/solarflare-linux-x86_64.tar.gz \
  -C <staging-root> .
```

> [!TIP]
> When in doubt, extract the previous release tarball and diff directory layout
> against your staging tree before publishing.

### Verification checklist

```bash
# Still clean aside from release-artifacts/
git status --short

# Version string matches tag and identifies SolarFlare fork
./release-artifacts/sunshine-x86_64 --version

# No missing shared libraries
ldd ./release-artifacts/sunshine-x86_64

# Packaged Web UI branding matches repository sources (spot-check hashes)
sha256sum docs/images/solarflare-mark-1024.png
# compare against packaged icon path inside tarball
```

Expected `--version` output must align with `BUILD_VERSION` and the
`v<build>-solarflare` tag. The log line in `src/main.cpp` prints
`PROJECT_VERSION` and `PROJECT_VERSION_COMMIT`.

---

## Generate SHA256SUMS

From the directory containing the two payloads:

```bash
cd release-artifacts
sha256sum sunshine-x86_64 solarflare-linux-x86_64.tar.gz > SHA256SUMS
cat SHA256SUMS
```

Verify before upload:

```bash
sha256sum -c SHA256SUMS
```

The Web UI updater downloads `solarflare-linux-x86_64.tar.gz` and validates
against this file. **Include both artifacts** in `SHA256SUMS` - not the sums
file itself.

---

## Publish manually

Push only after local verification succeeds.

### Push commit and tag

If you used `--no-push`:

```bash
git push origin master
git push origin v<build-version>-solarflare
# Example:
git push origin v2026.824.2-solarflare
```

### Create GitHub release

Use the compatibility tag as the release tag; use display version in the title:

```bash
gh release create v<build-version>-solarflare \
  sunshine-x86_64 \
  solarflare-linux-x86_64.tar.gz \
  SHA256SUMS \
  --repo vindeckyy/Solar-Flare \
  --verify-tag \
  --latest \
  --title 'SolarFlare v<semver>' \
  --notes-file release-notes.md
```

Concrete example:

```bash
gh release create v2026.824.2-solarflare \
  release-artifacts/sunshine-x86_64 \
  release-artifacts/solarflare-linux-x86_64.tar.gz \
  release-artifacts/SHA256SUMS \
  --repo vindeckyy/Solar-Flare \
  --verify-tag \
  --latest \
  --title 'SolarFlare v1.2.3' \
  --notes-file release-notes.md
```

| Flag | Purpose |
|---|---|
| `--verify-tag` | Ensures the tag exists locally before creating release |
| `--latest` | Marks this release as "Latest" on GitHub |
| `--repo vindeckyy/Solar-Flare` | **Required** - never publish to LizardByte |

### Policy reminder

Do **not** publish SolarFlare releases, issues, or pull requests in the
LizardByte GitHub organization.

---

## Post-publish validation

1. Open the release page on GitHub - confirm **not** draft/prerelease.
2. Confirm all **three** assets show uploaded state.
3. Download assets into a **clean directory** (not your build tree):

   ```bash
   gh release download v2026.824.2-solarflare \
     --repo vindeckyy/Solar-Flare \
     -D /tmp/sf-release-verify
   cd /tmp/sf-release-verify
   sha256sum -c SHA256SUMS
   ```

4. Smoke-test `sunshine-x86_64 --version` from the download directory.
5. Confirm README badge and release table on `master` match the published
   display/build versions.
6. Optional: trigger in-app update on a staging host pointed at the new release.

---

## Rollback and hotfix

GitHub Releases are immutable from a user-trust perspective - prefer **forward**
fixes:

1. Identify the defect.
2. Land fix on `master` with tests.
3. Cut a **new** build version (never reuse a published build version).
4. Publish new assets; mark release notes with superseded version if needed.

Do **not** force-push tags that users may already have downloaded. If a release
must be withdrawn, delete the GitHub release and tag only when no users could
have consumed it; otherwise publish a corrective release.

---

## Release notes template

Save as `release-notes.md` before `gh release create`:

```markdown
## SolarFlare v1.2.3

> [!CAUTION]
> **New users:** build from source with `./scripts/linux-install.sh`.
> This release ships binaries for **updating an existing SolarFlare install**
> only.

### Assets

| File | Purpose |
|---|---|
| `sunshine-x86_64` | Stripped executable update |
| `solarflare-linux-x86_64.tar.gz` | Full runtime + Web UI bundle (in-app updater) |
| `SHA256SUMS` | Integrity checksums |

After replacing the binary, restore capabilities if you use KMS capture:

```bash
sudo setcap 'cap_sys_admin,cap_sys_nice+p' /path/to/sunshine
```

### Changes

- …
- …

**Full changelog:** compare `v2026.824.1-solarflare`…`v2026.824.2-solarflare` on GitHub.
```

---

## Quick reference

```bash
# 1. Preview
./scripts/release.sh <build> <display> --dry-run

# 2. Commit + tag locally
./scripts/release.sh <build> <display> --no-push

# 3. Build with embedded version
export BRANCH=master BUILD_VERSION=<build> COMMIT=$(git rev-parse HEAD)
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF
cmake --build cmake-build-release --target sunshine web-ui -j2

# 4. Stage artifacts + SHA256SUMS (see sections above)

# 5. Push + publish
git push origin master && git push origin v<build>-solarflare
gh release create v<build>-solarflare ... --title 'SolarFlare v<display>' --notes-file release-notes.md
```

See also [Maintainer Documentation](README.md) for CI scope and onboarding.
