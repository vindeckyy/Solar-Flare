#!/usr/bin/env bash
#
# @file
# @brief Verify synchronized SolarFlare build and display release metadata.

set -euo pipefail

release_script="$1"
test_root="$(mktemp -d)"
trap 'rm -rf "$test_root"' EXIT
mkdir -p "$test_root/scripts" "$test_root/docs"
cp "$release_script" "$test_root/scripts/release.sh"

cat > "$test_root/CMakeLists.txt" <<'EOF'
project(Sunshine VERSION 2026.726.1)
EOF
cat > "$test_root/pyproject.toml" <<'EOF'
[project]
name = "solarflare"
version = "2026.726.1"
EOF
cat > "$test_root/uv.lock" <<'EOF'
version = 1

[[package]]
name = "solarflare"
version = "2026.726.1"
source = { editable = "." }
EOF
cat > "$test_root/README.md" <<'EOF'
<img src="https://img.shields.io/badge/release-v1.0.7-f97316?style=for-the-badge">

| **Current release** | [`v1.0.7`](https://github.com/vindeckyy/Solar-Flare/releases/latest) |
| **Build tag** | [`v2026.726.1-solarflare`](https://github.com/vindeckyy/Solar-Flare/releases/tag/v2026.726.1-solarflare) |

SolarFlare v1.0.7 publishes three Linux x86-64 files:
EOF
cat > "$test_root/docs/CHANGELOG-SolarFlare.md" <<'EOF'
# SolarFlare Changelog

## 2026-07-26 — SolarFlare v1.0.7 (`v2026.726.1-solarflare`)
EOF

cd "$test_root"
git init -q
git config user.name "SolarFlare Test"
git config user.email "solarflare-test@example.invalid"
git add .
git commit -qm "test fixture"
git tag -a v2026.726.1-solarflare -m "previous release"

if ./scripts/release.sh 2026.727.1 invalid --dry-run >/dev/null 2>&1; then
  echo "invalid display version was accepted" >&2
  exit 1
fi
./scripts/release.sh 2026.727.1 1.0.8 --no-push >/dev/null

## @brief Assert that a file contains an exact fixed string.
## @param $1 File to inspect.
## @param $2 Expected text.
assert_contains() {
  if ! grep -Fq "$2" "$1"; then
    printf 'expected %s to contain: %s\n' "$1" "$2" >&2
    exit 1
  fi
}

assert_contains CMakeLists.txt "VERSION 2026.727.1"
assert_contains pyproject.toml 'version = "2026.727.1"'
assert_contains uv.lock 'version = "2026.727.1"'
assert_contains README.md 'badge/release-v1.0.8-f97316'
assert_contains README.md '| **Current release** | [`v1.0.8`](https://github.com/vindeckyy/Solar-Flare/releases/latest) |'
assert_contains README.md 'v2026.727.1-solarflare'
assert_contains README.md 'SolarFlare v1.0.8 publishes three Linux x86-64 files:'
assert_contains docs/CHANGELOG-SolarFlare.md 'SolarFlare v1.0.8 (`v2026.727.1-solarflare`)'
git rev-parse -q --verify refs/tags/v2026.727.1-solarflare >/dev/null
