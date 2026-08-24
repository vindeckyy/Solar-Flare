# Maintainer Documentation

This directory contains documentation for **SolarFlare maintainers**: release
engineering, versioning policy, CI limitations, and operational procedures that
are not required for general contributors or end users.

> [!NOTE]
> These pages are **excluded from the public Doxygen documentation build** and
> are not published on the Read the Docs site. They live in the git tree for
> maintainer onboarding and agent automation only.

---

## Audience

| Reader | Start here |
|---|---|
| New fork maintainer | This page → [Release Process](release.md) → [CONTRIBUTING.md](../../CONTRIBUTING.md) |
| Release engineer | [Release Process](release.md) (full checklist) |
| CI/debugging maintainer | [Continuous integration](#continuous-integration) below |
| Security responder | [SECURITY.md](../../SECURITY.md) |

---

## Maintainer responsibilities

Maintainers are expected to:

1. **Triage** issues and PRs against fork scope (see
   [CONTRIBUTING.md](../../CONTRIBUTING.md#fork-vs-upstream)).
2. **Enforce** test coverage, Doxygen, and `clang-format` on merged C++ changes.
3. **Cut releases** locally per [release.md](release.md) - GitHub Actions does
   **not** produce release binaries for this fork.
4. **Keep dual versioning** synchronized across CMake, Python metadata, README,
   and Git tags.
5. **Never** publish releases, issues, or PRs under the LizardByte organization
   for SolarFlare work.

---

## Versioning model (summary)

SolarFlare intentionally carries **two** version identifiers:

| Identifier | Example | Where it appears |
|---|---|---|
| **Display version** (SemVer) | `1.2.2` | GitHub release **title** (`SolarFlare v1.2.2`), README badge, user-facing copy |
| **Build version** (chronological) | `2026.824.1` | `CMakeLists.txt`, `pyproject.toml`, `uv.lock`, embedded `sunshine --version`, compatibility **tag** |

Git tag format:

```text
v<build-version>-solarflare
```

Example: `v2026.824.1-solarflare`

The executable and Moonlight compatibility layer report the **build version**.
Users browsing GitHub releases see the **display version** in the title.

Full procedure: [Release Process](release.md).

---

## Release artifacts (summary)

Each published Linux x86-64 release ships exactly **three** files:

| Asset | Purpose |
|---|---|
| `sunshine-x86_64` | Stripped executable (compatibility filename) for in-place binary updates |
| `solarflare-linux-x86_64.tar.gz` | Executable + runtime/Web UI assets + icon + license (Web UI updater path) |
| `SHA256SUMS` | SHA-256 checksums for both payloads |

Asset names are **stable contracts** - the Web UI updater downloads the tarball
and checksum file from GitHub Releases. Do not rename without a coordinated
migration.

> [!CAUTION]
> Release binaries are for **updating an existing SolarFlare install** only.
> New users must build from source with
> [`scripts/linux-install.sh`](../../scripts/linux-install.sh). Every GitHub
> release notes file must include a red `CAUTION` callout stating this.

---

## `scripts/release.sh` (summary)

The release script synchronizes version metadata and creates the annotated tag:

```bash
./scripts/release.sh <build-version> <display-version> [--dry-run] [--no-push]
```

Example:

```bash
./scripts/release.sh 2026.824.2 1.2.3 --no-push
```

It updates:

- `CMakeLists.txt` (`project(Sunshine VERSION ...)`)
- `pyproject.toml` and `uv.lock`
- `README.md` (badge, release table, asset copy)
- `docs/CHANGELOG-SolarFlare.md` (prepends dated section)

Then commits, tags `v<build>-solarflare`, and optionally pushes.

Details, validation rules, and failure modes: [release.md](release.md#version-and-tag-script).

---

## Continuous integration

Fork CI is defined in [`.github/workflows/ci.yml`](../../.github/workflows/ci.yml).

| Job | Workflow | Purpose |
|---|---|---|
| Web bundle | `ci-bundle.yml` | `npm ci` + `npm run build` |
| Linux build and tests | `ci-linux.yml` | `linux_build.sh`, Xvfb, `test_sunshine`, gcovr upload |

**Not run by default in this fork:** macOS, Windows, FreeBSD, Arch, Homebrew,
Flatpak, Copr, and upstream release publishing pipelines. Those workflows remain
inherited but depend on LizardByte-only secrets and org-scoped lint (`_common-lint.yml`
runs only when `github.repository` starts with `LizardByte/`).

CI builds use `release_version: 0.0.0-ci`. They verify compile/test health - **not** release artifact fidelity.

---

## Documentation map

| Document | Maintainer use |
|---|---|
| [release.md](release.md) | End-to-end release checklist |
| [../CHANGELOG-SolarFlare.md](../CHANGELOG-SolarFlare.md) | Fork history; script prepends entries |
| [../CONFIGURATION.md](../CONFIGURATION.md) | Fork feature inventory |
| [../building.md](../building.md) | Cross-platform build reference |
| [../../SECURITY.md](../../SECURITY.md) | Advisory handling |
| [../../CONTRIBUTING.md](../../CONTRIBUTING.md) | Contributor policy enforced on merge |

---

## Available documentation

### [Release Process](release.md)

Step-by-step instructions for preparing versions with `scripts/release.sh`,
building and verifying Linux x86-64 artifacts locally, generating `SHA256SUMS`,
publishing GitHub releases with `gh`, and post-publish validation.

---

## Access and tooling

| Tool | Typical use |
|---|---|
| `gh` | Create GitHub releases, download assets, manage advisories |
| `git` | Tags, clean-tree verification, push to `vindeckyy/Solar-Flare` |
| `cmake` + `ninja` | Release binaries with embedded version metadata |
| `sha256sum` | Generate and verify `SHA256SUMS` |
| `ldd` | Confirm release ELF has no missing shared libraries |
| `setcap` | Document post-download capability setup for KMS capture |

Maintainers need push access to `vindeckyy/Solar-Flare` and permission to create
releases on that repository.

---

## Handoff checklist (new maintainer)

- [ ] Read [CONTRIBUTING.md](../../CONTRIBUTING.md) and [AGENTS.md](../../AGENTS.md)
- [ ] Build from source with `scripts/linux-install.sh` on a supported distro
- [ ] Run `test_sunshine` locally with `BUILD_TESTS=ON`
- [ ] Walk through [release.md](release.md) using `--dry-run` and `--no-push`
- [ ] Confirm `gh auth status` for `vindeckyy/Solar-Flare`
- [ ] Review [SECURITY.md](../../SECURITY.md) disclosure path
