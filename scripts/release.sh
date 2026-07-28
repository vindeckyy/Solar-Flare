#!/usr/bin/env bash
#
# @file
# @brief Prepare a synchronized SolarFlare build version and public release.
#
# SolarFlare keeps an internal chronological build version for compatibility
# tags and a simple SemVer display version for GitHub release titles.
#
# Usage:
#   ./scripts/release.sh <build-version> <display-version>
#   ./scripts/release.sh <build-version> <display-version> --dry-run
#
# Example:
#   ./scripts/release.sh 2026.729.1 1.0.9 --no-push
#
# Side effects:
#   1. Updates CMakeLists.txt, pyproject.toml, and uv.lock.
#   2. Updates the README display version and compatibility build tag.
#   3. Prepends a release section to docs/CHANGELOG-SolarFlare.md.
#   4. Creates a release commit and annotated compatibility tag.
#   5. Pushes the commit and tag unless --no-push is supplied.

set -euo pipefail

BUILD_VERSION="${1:-}"
DISPLAY_VERSION="${2:-}"
DRY_RUN=false
PUSH=true
for arg in "${@:3}"; do
  case "$arg" in
    --dry-run) DRY_RUN=true ;;
    --no-push) PUSH=false ;;
    *)
      echo "unknown argument: $arg" >&2
      exit 1
      ;;
  esac
done

if [[ -z "$BUILD_VERSION" || -z "$DISPLAY_VERSION" ]]; then
  echo "usage: $0 <build-version> <display-version> [--dry-run] [--no-push]" >&2
  echo "  e.g. $0 2026.729.1 1.0.9" >&2
  exit 1
fi

if ! [[ "$BUILD_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "bad build version: '$BUILD_VERSION' (expected YYYY.MDD.REVISION)" >&2
  exit 1
fi
if ! [[ "$DISPLAY_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "bad display version: '$DISPLAY_VERSION' (expected MAJOR.MINOR.PATCH)" >&2
  exit 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Refuse duplicate or out-of-order compatibility tags. Upstream-compatible tags
# without the -solarflare suffix are intentionally excluded from this check.
LATEST_TAG="$(git tag --list 'v*-solarflare' --sort=-version:refname | head -n 1)"
if [[ -n "$LATEST_TAG" ]]; then
  LATEST_VERSION="${LATEST_TAG#v}"
  LATEST_VERSION="${LATEST_VERSION%-solarflare}"
  if [[ "$BUILD_VERSION" == "$LATEST_VERSION" ]] ||
     [[ "$(printf '%s\n' "$LATEST_VERSION" "$BUILD_VERSION" | sort -V | tail -n 1)" != "$BUILD_VERSION" ]]; then
    echo "build version '$BUILD_VERSION' must be newer than $LATEST_VERSION" >&2
    exit 1
  fi
fi

if [[ -n "$(git status --porcelain --untracked-files=normal)" ]]; then
  echo "working tree dirty (commit or stash before releasing)" >&2
  exit 1
fi

if [[ -z "$(git config user.email)" ]]; then
  if [[ -f ~/.hermes/.env ]] && grep -q '^GITHUB_TOKEN=' ~/.hermes/.env; then
    git config user.name "Hayden"
    git config user.email "hayden@users.noreply.github.com"
  else
    echo "git identity not set and ~/.hermes/.env has no GITHUB_TOKEN" >&2
    echo "run: git config user.name / user.email" >&2
    exit 1
  fi
fi

BADGE_URL="https://img.shields.io/badge/release-v${DISPLAY_VERSION}-f97316?style=for-the-badge"
echo "→ build $BUILD_VERSION, release v$DISPLAY_VERSION"
echo "→ badge $BADGE_URL"

if [[ "$DRY_RUN" == false ]]; then
  sed -i -E "s|^(project\(Sunshine[[:space:]]+VERSION[[:space:]]+)[0-9]+\.[0-9]+\.[0-9]+|\1$BUILD_VERSION|" CMakeLists.txt
  sed -i -E "s|^(version[[:space:]]*=[[:space:]]*\")[0-9]+\.[0-9]+\.[0-9]+(\"[[:space:]]*$)|\1$BUILD_VERSION\2|" pyproject.toml

  BUILD_VERSION="$BUILD_VERSION" python3 - <<'PY'
import os
import pathlib
import re

path = pathlib.Path("uv.lock")
source = path.read_text()
updated, count = re.subn(
    r'(\[\[package\]\]\nname = "solarflare"\nversion = ")[^"]+',
    lambda match: match.group(1) + os.environ["BUILD_VERSION"],
    source,
    count=1,
)
if count != 1:
    raise SystemExit("uv.lock SolarFlare package version not matched")
path.write_text(updated)
PY
fi
echo "✓ build versions"

if [[ "$DRY_RUN" == false ]]; then
  BUILD_VERSION="$BUILD_VERSION" DISPLAY_VERSION="$DISPLAY_VERSION" python3 - <<'PY'
import os
import pathlib
import re

build = os.environ["BUILD_VERSION"]
display = os.environ["DISPLAY_VERSION"]
path = pathlib.Path("README.md")
source = path.read_text()
replacements = (
    (
        r'(https://img\.shields\.io/badge/release-)v[0-9]+\.[0-9]+\.[0-9]+(?:--solarflare)?(-f97316\?style=for-the-badge)',
        rf'\g<1>v{display}\g<2>',
    ),
    (
        r'^\| \*\*Current release\*\* \| .+ \|$',
        f'| **Current release** | [`v{display}`](https://github.com/vindeckyy/Solar-Flare/releases/latest) |',
    ),
    (
        r'^\| \*\*Build tag\*\* \| .+ \|$',
        f'| **Build tag** | [`v{build}-solarflare`](https://github.com/vindeckyy/Solar-Flare/releases/tag/v{build}-solarflare) |',
    ),
    (
        r'^SolarFlare v[0-9]+\.[0-9]+\.[0-9]+ publishes three Linux x86-64 files:$',
        f'SolarFlare v{display} publishes three Linux x86-64 files:',
    ),
)
for pattern, replacement in replacements:
    source, count = re.subn(pattern, replacement, source, count=1, flags=re.MULTILINE)
    if count != 1:
        raise SystemExit(f"README.md release metadata not matched: {pattern}")
path.write_text(source)
PY
fi
echo "✓ README.md"

TODAY="$(date -u +%Y-%m-%d)"
if [[ "$DRY_RUN" == false ]] && ! grep -q "v${BUILD_VERSION}-solarflare" docs/CHANGELOG-SolarFlare.md; then
  BUILD_VERSION="$BUILD_VERSION" DISPLAY_VERSION="$DISPLAY_VERSION" TODAY="$TODAY" python3 - <<'PY'
import os
import pathlib

build = os.environ["BUILD_VERSION"]
display = os.environ["DISPLAY_VERSION"]
today = os.environ["TODAY"]
path = pathlib.Path("docs/CHANGELOG-SolarFlare.md")
lines = path.read_text().splitlines(keepends=True)
insert_at = next((i for i, line in enumerate(lines) if i > 0 and line.startswith("## ")), len(lines))
lines.insert(
    insert_at,
    f"\n## {today}: SolarFlare v{display} (`v{build}-solarflare`)\n\n"
    "Release notes are published with the corresponding GitHub release. "
    "Compare this tag with the previous SolarFlare release for the complete change set.\n\n",
)
path.write_text("".join(lines))
PY
fi
echo "✓ CHANGELOG"

TAG="v${BUILD_VERSION}-solarflare"
if [[ "$DRY_RUN" == false ]]; then
  git add CMakeLists.txt pyproject.toml uv.lock README.md docs/CHANGELOG-SolarFlare.md
  git commit -m "release: SolarFlare v${DISPLAY_VERSION} (build ${BUILD_VERSION})"
  git tag -a "$TAG" -m "SolarFlare v${DISPLAY_VERSION}"
fi
echo "✓ tag $TAG"

if [[ "$PUSH" == true && "$DRY_RUN" == false ]]; then
  git push origin master
  git push origin "$TAG"
  echo "✓ pushed"
fi

echo
echo "SolarFlare v${DISPLAY_VERSION} (build ${BUILD_VERSION}) prepared."
