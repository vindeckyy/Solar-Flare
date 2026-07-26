#!/usr/bin/env bash
#
# @brief Compatibility wrapper for the SolarFlare Linux installer.
#
# Historical name kept so existing bookmarks, Web UI upgrade commands,
# and documentation that still mention `cachyos-build.sh` keep working.
# New instructions should call `scripts/linux-install.sh` directly.
#
# @param $@ Forwarded unchanged to linux-install.sh.

set -euo pipefail

exec "$(cd "$(dirname "$0")" && pwd)/linux-install.sh" "$@"
