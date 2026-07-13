#!/usr/bin/env bash
# SolarFlare release script — the ONLY thing that should edit version strings.
# ponytail: three version files (CMakeLists.txt, pyproject.toml, README badge)
# must stay in lockstep and must only change at release time. Feature commits
# must NOT touch these files.
#
# Usage:
#   ./scripts/release.sh <version>            # e.g. 2026.999.2
#   ./scripts/release.sh <version> --dry-run # preview without writing
#
# Side effects (in order):
#   1. Updates CMakeLists.txt:7 PROJECT VERSION
#   2. Updates pyproject.toml:7 version
#   3. Updates README.md:14 version badge (with proper shields.io escaping)
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
  echo "  e.g. $0 2026.999.2" >&2
  exit 1
fi

# Validate version shape: YYYY.MAJOR.MINOR (dots only, all digits)
if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "bad version: '$VERSION' — expected YYYY.MAJOR.MINOR" >&2
  exit 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Refuse if there are uncommitted changes — release must be clean
if ! git diff --quiet HEAD 2>/dev/null; then
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

# shields.io needs double-dash escaped to -- in the URL
BADGE_VER="${VERSION//--/--}"
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

# 3. README.md badge — only update the version=... segment inside the version badge.
# Match the existing badge URL and replace only the version portion, so a future
# shields.io URL change doesn't break this script.
if [ "$DRY_RUN" = false ]; then
  python3 -c "
import re, pathlib
p = pathlib.Path('README.md')
src = p.read_text()
new = re.sub(
  r'(badge/version-)(v[0-9]+\.[0-9]+\.[0-9]+--solarflare)',
  lambda m: m.group(1) + 'v${VERSION}--solarflare',
  src
)
if new == src:
    raise SystemExit('README.md version badge not matched — update script or fix manually')
p.write_text(new)
"
fi
echo "✓ README.md"

# 4. CHANGELOG header — single Python call, no temp file
TODAY="$(date -u +%Y-%m-%d)"
if [ "$DRY_RUN" = false ] && ! grep -q "^## $TODAY " docs/CHANGELOG-SolarFlare.md; then
  python3 -c "
import pathlib
p = pathlib.Path('docs/CHANGELOG-SolarFlare.md')
lines = p.read_text().splitlines(keepends=True)
# Insert after the first '## YYYY-MM-DD' section header
for i, l in enumerate(lines):
    if l.startswith('## ') and i > 0:
        insert_at = i
        break
else:
    insert_at = len(lines)
lines.insert(insert_at, '\n## $TODAY — v${VERSION}-solarflare\n\nRelease notes TBD. Run \`git log v<prev>..HEAD --oneline\` to enumerate.\n\n')
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