#!/usr/bin/env bash
#
# @file
# @brief Verify NixOS and NixOS-derived distribution detection.

set -euo pipefail

installer="$1"
test_root="$(mktemp -d)"
trap 'rm -rf "$test_root"' EXIT

cat > "$test_root/nixos" <<'EOF'
NAME=NixOS
ID=nixos
PRETTY_NAME="NixOS 25.11"
EOF

cat > "$test_root/derived" <<'EOF'
NAME=ExampleOS
ID=example
ID_LIKE="nixos linux"
PRETTY_NAME="ExampleOS"
EOF

cat > "$test_root/ubuntu" <<'EOF'
NAME=Ubuntu
ID=ubuntu
ID_LIKE=debian
PRETTY_NAME="Ubuntu 24.04"
EOF

## @brief Assert the distribution detected from an os-release fixture.
## @param $1 Path to the os-release fixture.
## @param $2 Expected canonical distribution identifier.
assert_distro() {
  local os_release_file="$1"
  local expected="$2"
  local actual
  actual="$(SOLARFLARE_OS_RELEASE_FILE="$os_release_file" "$installer" --print-distro-id)"
  if [[ "$actual" != "$expected" ]]; then
    printf 'expected distro %s, got %s\n' "$expected" "$actual" >&2
    exit 1
  fi
}

assert_distro "$test_root/nixos" nixos
assert_distro "$test_root/derived" nixos
assert_distro "$test_root/ubuntu" ubuntu
