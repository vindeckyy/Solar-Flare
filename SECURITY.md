# Security

This document covers the SolarFlare fork's security policy. For Sunshine
itself, see upstream's
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) and the
[upstream security policy](https://github.com/LizardByte/.github/blob/master/SECURITY.md)
(if one is published).

## Reporting a vulnerability in the fork

If you find a security issue that is **specific to the SolarFlare fork**,
report it privately through
[GitHub Security Advisories](https://github.com/vindeckyy/Solar-Flare/security/advisories/new).
Do not open a public issue containing vulnerability details. If private
reporting is unavailable, open an issue that asks the maintainer for a private
contact channel without including technical details or a reproducer.

For **upstream Sunshine security issues** (anything that would also
affect a stock LizardByte/Sunshine build), please report them at
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine/security/advisories/new)
instead. Fixes are cherry-picked onto the fork once upstream ships
them.

## Scope

Fork-specific changes include:

| Component                  | Security impact                                    |
|----------------------------|----------------------------------------------------|
| Fork configuration and API behavior | May affect authentication, authorization, networking, or process behavior |
| `scripts/cachyos-build.sh` and packaging | May affect build integrity, installation, permissions, and bundled files |
| Fork-specific web UI code | May affect browser-side authentication, API calls, or content handling |
| Native compiler and linker flags | May affect binary compatibility and hardening |
| Fork publisher and update metadata | May affect where users obtain builds and report problems |

Code inherited from upstream may still interact with fork-specific changes. If
you are unsure where a vulnerability belongs, report it privately here and the
maintainer will coordinate with upstream as needed.

## Supported versions

| Version | Supported |
|---|---|
| Latest `1.0.x` release | Yes |
| `master` | Yes |
| Older `1.0.x` releases | Best effort until the next release |
| Pre-`1.0` tags | No |

There is no LTS branch and no `release/X` long-term-support branches. If you
need a patched build before the next tagged release, pull the latest `master`
and run `./scripts/cachyos-build.sh`.

## Update policy

The fork follows upstream's release cadence. Critical security fixes
are cherry-picked onto the fork as soon as upstream ships them (see
[docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md) for the
cherry-pick log). For CVE-tracked notifications affecting inherited Sunshine
code, watch upstream as well as this repository.

## What to expect when you report

1. A maintainer acknowledgement within ~7 days (the fork is a
   single-maintainer project, so response time is variable).
2. A reproducer (we'll ask you for one if you didn't provide it).
3. A fix committed to `master` and cherry-picked from upstream if
   it's a Sunshine issue.
4. A note in `docs/CHANGELOG-SolarFlare.md` mentioning the fix.

The fork does **not** ship backports to a separate LTS branch. Whether a
fork-specific report becomes a published advisory depends on its impact and
the coordinated-disclosure process.
