# SolarFlare Release Process

SolarFlare publishes Linux x86-64 binaries built and verified on the maintainer's
system for updating existing installs. New users must build from source with
`./scripts/linux-install.sh`. GitHub Actions must not build release binaries.

Two version identifiers are retained intentionally. Examples below use the
*next* release identifiers so the commands stay copy-paste templates:

- The public release title uses chronological SemVer, such as
  `SolarFlare v1.0.9`.
- The compatibility build tag uses `v<YYYY>.<MDD>.<revision>-solarflare`, such
  as `v2026.729.1-solarflare`. The executable reports this build version.

The current published release is documented in the repository README.

Release commits and binaries must be produced from a clean tree so the embedded
version never carries a `-dirty` suffix.

## Prepare

1. Confirm `master` is synchronized with `origin/master`.
2. Choose the next display version and compatibility build version.
3. Run the Web UI production build and the relevant GoogleTest suite with a
   maximum of two parallel jobs.
4. Confirm `git status --short` is empty.
5. Prepare concise notes that list the exact published asset names.
   Include a red GitHub `CAUTION` callout stating that new users must build
   with `./scripts/linux-install.sh` and that release binaries are only for
   updating an already working SolarFlare install.

## Version and Tag

Preview the release transaction:

```bash
./scripts/release.sh 2026.729.1 1.0.9 --dry-run
```

Create the synchronized version commit and compatibility tag without pushing:

```bash
./scripts/release.sh 2026.729.1 1.0.9 --no-push
```

The script updates `CMakeLists.txt`, `pyproject.toml`, `uv.lock`, the README
release badge and table, and `docs/CHANGELOG-SolarFlare.md`. It then creates the
release commit and `v<build-version>-solarflare` tag.

## Build and Verify Locally

Build from the final tagged commit. Set `BRANCH`, `BUILD_VERSION`, and `COMMIT`
during CMake configuration so the executable embeds the exact build identity.
The maintained release contains only these Linux x86-64 files:

| Asset | Contents |
|---|---|
| `sunshine-x86_64` | Stripped compatibility-named executable |
| `solarflare-linux-x86_64.tar.gz` | Executable, runtime and Web UI assets, icon, and license |
| `SHA256SUMS` | SHA-256 checksums for the executable and runtime bundle |

The Web UI updater downloads the tarball and `SHA256SUMS` from the GitHub
release. Keep those asset names stable.

Verify at minimum:

```bash
git status --short
/path/to/sunshine-x86_64 --version
ldd /path/to/sunshine-x86_64
(cd /path/to/release-artifacts && sha256sum -c SHA256SUMS)
```

The version output must match the compatibility tag and final commit and must
identify the SolarFlare fork. `ldd` must report no missing libraries. Compare
the packaged Web UI logo and icon byte-for-byte with the repository sources.
A raw executable cannot retain Linux file capabilities in a GitHub release;
installation instructions must run
`setcap 'cap_sys_admin,cap_sys_nice+p'` after download.

## Publish Manually

Push the release commit and compatibility tag only after local verification:

```bash
git push origin master
git push origin v2026.729.1-solarflare
```

Create the GitHub release manually and upload the three verified local files:

```bash
gh release create v2026.729.1-solarflare \
  sunshine-x86_64 \
  solarflare-linux-x86_64.tar.gz \
  SHA256SUMS \
  --repo vindeckyy/Solar-Flare \
  --verify-tag \
  --latest \
  --title 'SolarFlare v1.0.9' \
  --notes-file release-notes.md
```

Download the published assets into a clean directory and rerun
`sha256sum -c SHA256SUMS`. Confirm the release is neither a draft nor a
prerelease and that all three assets are in the `uploaded` state.

Do not publish SolarFlare releases, issues, or pull requests in the LizardByte
GitHub organization.
