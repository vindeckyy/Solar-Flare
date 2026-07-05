#!/bin/bash
# Take screenshots of the SolarFlare web UI for docs/README.
# Must be run from the desktop session (not SSH) — Chrome headless
# needs the full user session for GPU/display access.
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/../docs/screenshots"
mkdir -p "$OUT_DIR"

CHROME="${CHROME:-google-chrome-stable}"
FLAGS="--headless --no-sandbox --ignore-certificate-errors --window-size=1280,800"
BASE="https://localhost:47990"

echo "Screenshots -> $OUT_DIR"
for page in "" config apps; do
  name="${page:-home}"
  echo "  $name"
  $CHROME $FLAGS --screenshot="$OUT_DIR/$name.png" "$BASE/$page"
done
echo "Done. Files:"
ls -la "$OUT_DIR/"
