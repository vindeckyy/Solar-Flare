#!/usr/bin/env bash
# SolarFlare release script — the ONLY thing that should edit version strings.
# ponytail: the two source version files (CMakeLists.txt and pyproject.toml)
# must stay in lockstep and must only change at release time. The README uses
# GitHub's dynamic latest-release badge, though older branches may still carry
# the legacy hard-coded version badge that this script knows how to update.
#
# Usage:
#   ./scripts/release.sh <version>            # e.g. 2026.719.1
#   ./scripts/release.sh <version> --dry-run # preview without writing
#
# Side effects (in order):
#   1. Updates CMakeLists.txt:7 PROJECT VERSION
#   2. Updates pyproject.toml:7 version
#   3. Updates a legacy README version badge, or validates the dynamic badge
#   4. Prepends a release section to docs/CHANGELOG-SolarFlare.md
#   5. Creates a git tag v<version>-solarflare
#   6. Pushes the tag + commit (skippable with --no-push)
#
# Bails if working tree is dirty (no untagged version drift allowed).

set -e

VERSION="${1:-}"
DRY_RUN=false
PUSH=true
for arg in "$@"; do
  case "$arg" in
    --dry-run) DRY_RUN=true ;;
    --no-push) PUSH=false ;;
  esac
done

if [ -z "$VERSION" ]; then
  echo "usage: $0 <version> [--dry-run] [--no-push]" >&2
  echo "  e.g. $0 2026.718.5" >&2
  exit 1
fi

# Validate version shape: YYYY.MDD.REVISION (dots only, all digits).
if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "bad version: '$VERSION' — expected YYYY.MDD.REVISION" >&2
  exit 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Refuse duplicate or out-of-order SolarFlare tags. Upstream-compatible tags
# without the -solarflare suffix are intentionally excluded from this check.
LATEST_TAG="$(git tag --list 'v*-solarflare' --sort=-version:refname | head -n 1)"
if [ -n "$LATEST_TAG" ]; then
  LATEST_VERSION="${LATEST_TAG#v}"
  LATEST_VERSION="${LATEST_VERSION%-solarflare}"
  if [ "$VERSION" = "$LATEST_VERSION" ] ||
     [ "$(printf '%s\n' "$LATEST_VERSION" "$VERSION" | sort -V | tail -n 1)" != "$VERSION" ]; then
    echo "version '$VERSION' must be newer than $LATEST_VERSION" >&2
    exit 1
  fi
fi

# Refuse tracked or untracked changes — release input must be reproducible.
if [ -n "$(git status --porcelain --untracked-files=normal)" ]; then
  echo "working tree dirty — commit or stash before releasing" >&2
  exit 1
fi

# Defensive: in a fresh clone, git identity may be unset. Borrow from
# ~/.hermes/.env GITHUB_TOKEN-author convention if available, else bail.
if [ -z "$(git config user.email)" ]; then
  if [ -f ~/.hermes/.env ] && grep -q '^GITHUB_TOKEN=' ~/.hermes/.env; then
    git config user.name "Hayden"
    git config user.email "hayden@users.noreply.github.com"
  else
    echo "git identity not set and ~/.hermes/.env has no GITHUB_TOKEN" >&2
    echo "run: git config user.name / user.email" >&2
    exit 1
  fi
fi

BADGE_URL="https://img.shields.io/badge/version-v${VERSION}--solarflare-orange?style=flat-square"

echo "→ setting version to $VERSION (badge: $BADGE_URL)"

# 1. CMakeLists.txt: PROJECT(Sunshine VERSION X.Y.Z ...)
# Match either "VERSION X.Y.Z" or "VERSION  X.Y.Z" (any whitespace)
if [ "$DRY_RUN" = false ]; then
  sed -i -E "s|^(project\(Sunshine[[:space:]]+VERSION[[:space:]]+)[0-9]+\.[0-9]+\.[0-9]+|\1$VERSION|" CMakeLists.txt
fi
echo "✓ CMakeLists.txt"

# 2. pyproject.toml: version = "X.Y.Z"
if [ "$DRY_RUN" = false ]; then
  sed -i -E "s|^(version[[:space:]]*=[[:space:]]*\")[0-9]+\.[0-9]+\.[0-9]+(\"[[:space:]]*$)|\1$VERSION\2|" pyproject.toml
fi
echo "✓ pyproject.toml"

# 3. README.md badge — update the legacy static badge when present. Newer
# READMEs use GitHub's dynamic latest-release badge and need no version edit.
if [ "$DRY_RUN" = false ]; then
  VERSION="$VERSION" python3 -c "
import os, re, pathlib
version = os.environ['VERSION']
p = pathlib.Path('README.md')
src = p.read_text()
new = re.sub(
  r'(badge/(?:version|release)-)(v[0-9]+\.[0-9]+\.[0-9]+--solarflare)',
  lambda m: m.group(1) + 'v' + version + '--solarflare',
  src
)
if new == src:
    dynamic_badge = 'img.shields.io/github/v/release/vindeckyy/Solar-Flare'
    if dynamic_badge not in src:
        raise SystemExit('README.md release badge not matched — update script or fix manually')
else:
    p.write_text(new)
"
fi
echo "✓ README.md"

# 4. CHANGELOG header — single Python call, no temp file
TODAY="$(date -u +%Y-%m-%d)"
if [ "$DRY_RUN" = false ] && ! grep -q "v${VERSION}-solarflare" docs/CHANGELOG-SolarFlare.md; then
  VERSION="$VERSION" TODAY="$TODAY" python3 -c "
import os, pathlib
version = os.environ['VERSION']
today = os.environ['TODAY']
p = pathlib.Path('docs/CHANGELOG-SolarFlare.md')
lines = p.read_text().splitlines(keepends=True)
for i, l in enumerate(lines):
    if l.startswith('## ') and i > 0:
        insert_at = i
        break
else:
    insert_at = len(lines)
lines.insert(insert_at, f'\n## {today} — v{version}-solarflare\n\nRelease notes are published with the corresponding GitHub release. Compare this tag with the previous SolarFlare release for the complete change set.\n\n')
p.write_text(''.join(lines))
"
fi
echo "✓ CHANGELOG"

# 5. Tag
if [ "$DRY_RUN" = false ]; then
  git add CMakeLists.txt pyproject.toml README.md docs/CHANGELOG-SolarFlare.md
  git commit -m "release: v${VERSION}-solarflare"
  git tag "v${VERSION}-solarflare"
fi
echo "✓ tag v${VERSION}-solarflare"

# 6. Push
if [ "$PUSH" = true ] && [ "$DRY_RUN" = false ]; then
  git push origin master
  git push origin "v${VERSION}-solarflare"
  echo "✓ pushed"
fi

echo
echo "release $VERSION complete."
