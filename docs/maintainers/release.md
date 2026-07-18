# SolarFlare Release Process

SolarFlare releases use chronological tags in the form
`v<YYYY>.<MDD>.<revision>-solarflare`, for example
`v2026.718.5-solarflare`. Release commits and binaries must be produced from a
clean tree so the embedded version never carries a `-dirty` suffix.

## Prepare

1. Confirm `master` is synchronized with the SolarFlare fork remote.
2. Run the Web UI production build and the relevant GoogleTest suite with a
   maximum of two parallel jobs.
3. Confirm `git status --short` is empty.
4. Review the changes since the previous SolarFlare tag and prepare concise
   release notes, calling out security and upgrade-impacting changes first.

## Version and Tag

Preview the release transaction:

```bash
./scripts/release.sh 2026.719.1 --dry-run
```

Create the synchronized version commit and tag without pushing:

```bash
./scripts/release.sh 2026.719.1 --no-push
```

The script updates `CMakeLists.txt`, `pyproject.toml`, the SolarFlare
changelog, and any legacy static README version badge. It then creates the
release commit and `-solarflare` tag.

## Build and Verify

Build release artifacts from the tagged, clean commit. For the maintained
Linux profile, verify at minimum:

```bash
git status --short
/path/to/staged/sunshine --version
sha256sum /path/to/staged/sunshine
```

The version output must match the tag and must not contain `dirty`. A raw
executable cannot retain Linux file capabilities in a GitHub release asset;
installation instructions must explicitly run `setcap` after download.

## Publish

Push the release commit and tag only after local verification:

```bash
git push origin master
git push origin v2026.719.1-solarflare
```

The tag triggers `.github/workflows/release.yml`, which creates the GitHub
release and uploads its artifacts. Verify every uploaded artifact name, size,
SHA-256 digest, and embedded version before marking the release complete.

Do not publish SolarFlare releases, issues, or pull requests in the LizardByte
GitHub organization.
