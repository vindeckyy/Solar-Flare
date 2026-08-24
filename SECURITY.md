# Security Policy

This document describes the SolarFlare fork's security posture, supported
versions, vulnerability reporting process, and threat model. For upstream
Sunshine security policy and advisories, see
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) and
[LizardByte's organization security policy](https://github.com/LizardByte/.github/blob/master/SECURITY.md).

---

## Supported versions

SolarFlare does **not** maintain long-term support (LTS) branches. Security
fixes land on `master` and are included in the next tagged release.

| Version / branch | Supported | Notes |
|------------------|-----------|-------|
| Latest `1.2.x` release ([`v1.2.2`](https://github.com/vindeckyy/Solar-Flare/releases/latest) at time of writing) | **Yes** | Recommended for production |
| `master` | **Yes** | Rolling development; may include unreleased fixes |
| Older `1.x` releases | Best effort | Upgrade when practical; no backport guarantee |
| Pre-`1.0` tags | **No** | Unsupported |
| Upstream Sunshine tags consumed by the fork | Inherited | Cherry-picked fixes tracked in [docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md) |

### Build identifiers

Release artifacts carry two version strings:

| Identifier | Example | Use |
|------------|---------|-----|
| User-facing release | `v1.2.2` | GitHub Releases, install scripts |
| Build tag (`PROJECT_VERSION`) | `2026.824.1` | `/api/health`, logs, updater comparisons |

When verifying whether a host is patched, check **both** the installed package
version and the `version` field from `GET /api/health`.

### Obtaining fixes before the next tag

1. Pull the latest `master` from this repository.
2. Rebuild or run `./scripts/linux-install.sh`.
3. Confirm `GET /api/health` reports the expected `version`.

There is no separate security-only branch or hotfix channel.

---

## Reporting a vulnerability

### Fork-specific issues

Report security issues **specific to SolarFlare** privately via
[GitHub Security Advisories](https://github.com/vindeckyy/Solar-Flare/security/advisories/new).

**Do not** open a public GitHub issue with vulnerability details, proof-of-concept
exploits, or credential material.

If private advisory creation is unavailable, open a **public** issue that asks
for a private contact channel only - include no technical details or reproducer
in that public thread.

### Upstream Sunshine issues

Vulnerabilities that affect stock
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) should be reported
to
[upstream security advisories](https://github.com/LizardByte/Sunshine/security/advisories/new).
The fork cherry-picks upstream fixes once they ship; see the changelog for the
pick log.

### Unsure where it belongs?

Report privately to this fork first. Maintainers will coordinate with upstream
when the issue spans inherited code.

### What to include

| Item | Why it helps |
|------|--------------|
| Affected version / commit | Determines scope and backport need |
| Component (Web UI, streaming, installer, …) | Routes to the right code path |
| Step-by-step reproducer | Confirms and regression-tests the fix |
| Impact assessment | Drives advisory severity and disclosure timing |
| Suggested fix (optional) | Speeds resolution |

### What to expect

1. **Acknowledgement** within ~7 days (single-maintainer project; times vary).
2. **Reproducer request** if one was not provided.
3. **Fix on `master`**, cherry-picked from upstream when applicable.
4. **Changelog entry** in `docs/CHANGELOG-SolarFlare.md`.
5. **Advisory publication** when impact warrants coordinated disclosure.

The fork does **not** ship backports to older release branches. Whether a
fork-specific report becomes a published GitHub Security Advisory depends on
impact and the coordinated-disclosure process.

---

## Scope of fork-specific security surface

| Component | Security relevance |
|-----------|-------------------|
| `src/confighttp.cpp` Web UI + `/api/*` | Authentication, authorization, CSRF, rate limiting, filesystem browse |
| `src/nvhttp.cpp` GameStream HTTPS | Client pairing, certificate pinning, streaming session setup |
| `src/config.*` | API tokens, webhooks, trusted subnets, origin restrictions |
| `scripts/linux-install.sh`, packaging | Install integrity, file permissions, capabilities |
| Fork Web UI (`src_assets/web/`) | Browser-side credential handling, API calls |
| Compiler / linker flags | Binary hardening, reproducibility |
| Update metadata + `src/update.cpp` | Supply chain for self-update payloads |
| `src/webhooks.cpp` | Outbound signed notifications |

Inherited upstream code may interact with fork-specific changes. Treat the
**combined** deployment as in scope when assessing risk.

---

## Threat model

SolarFlare is designed as a **self-hosted game-streaming server for trusted
local networks**. The baseline assumption is that the LAN is more trustworthy
than the public Internet, but **not** that every device on the LAN is benign.

### Assets

| Asset | Location | Impact if compromised |
|-------|----------|----------------------|
| Web UI credentials | Credentials file | Full host administration via `/api/*` |
| API tokens | `sunshine.conf` (`api_tokens`) | Scoped automation access |
| TLS private key | `sunshine.conf` / appdata | MITM of Web UI and GameStream HTTPS |
| Paired client certificates | Client cert store | Impersonate a Moonlight client to the host |
| `sunshine.conf` | Config directory | Change encoder, network, pairing, and webhook settings |
| Session video/audio stream | RTSP / ENet path | Confidentiality of desktop content |
| Host filesystem (via browse API) | `GET /api/browse` | Path disclosure; aids further attacks |
| Webhook URLs | Config | SSRF-style outbound requests (limited by URL validation) |

### Trust boundaries

```
┌─────────────────────────────────────────────────────────────┐
│  Internet (untrusted)                                       │
└───────────────────────────┬─────────────────────────────────┘
                            │  origin_web_ui_allowed = wan (discouraged)
┌───────────────────────────▼─────────────────────────────────┐
│  LAN / home network (semi-trusted)                          │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │ Moonlight   │  │ Web browser  │  │ Automation/script│   │
│  │ clients     │  │ (Web UI)     │  │ (curl + token)   │   │
│  └──────┬──────┘  └──────┬───────┘  └────────┬─────────┘   │
│         │ GameStream      │ /api/* + CSRF     │ Bearer       │
│         ▼                 ▼                   ▼              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              SolarFlare host process                  │  │
│  │  nvhttp (47989)  │  confighttp (47990)  │  stream   │  │
│  └──────────────────────────────────────────────────────┘  │
└───────────────────────────┬─────────────────────────────────┘
                            │  webhook POST (optional)
┌───────────────────────────▼─────────────────────────────────┐
│  User-configured webhook endpoints                          │
└─────────────────────────────────────────────────────────────┘
```

### LAN trust (`origin_web_ui_allowed`)

The Web UI enforces a network-class gate **before** authentication:

| Value | Allows |
|-------|--------|
| `pc` | Loopback only |
| `lan` (default) | Private / local addresses |
| `wan` | Any source IP |

Clients outside the allowed class receive **HTTP 403** with no JSON body.
This is a coarse network ACL, not a substitute for strong credentials.

**Recommendation:** Keep the default `lan`. Use `pc` on multi-user hosts
where only local administrators should reach the UI. Avoid `wan` unless the UI
is behind a separate reverse proxy with its own access controls.

Configuration: [origin_web_ui_allowed](docs/configuration.md#origin_web_ui_allowed).

### TLS

| Surface | Port | Certificate |
|---------|------|-------------|
| GameStream HTTPS | `port` (default 47989) | `cert` / `pkey` in config |
| Web UI HTTPS | `port + 1` (default 47990) | Same cert/key pair |

Both listeners use the configured X.509 certificate. Moonlight clients pin the
server certificate at pairing time. Browsers will warn on the self-signed
default until a trusted cert is installed.

**Threats mitigated:** passive eavesdropping on the control plane and stream
setup when TLS is used end-to-end.

**Residual risk:** A LAN attacker who obtains the private key or mounts a
successful MITM **before** pairing can impersonate the host to new clients.
Already-paired clients reject certificate changes unless re-paired.

Security headers on JSON API responses include `Strict-Transport-Security`
(HSTS) and `X-Frame-Options: DENY`.

### Web UI authentication and tokens

| Mechanism | Properties |
|-----------|------------|
| HTTP Basic Auth | Full admin; password stored as salted SHA-256 hash |
| Bearer API tokens | Scoped least-privilege; 64-char hex plaintext; stored as `SHA-256(token:salt)` |
| Login rate limit | 10 failures / 30 s / IP → HTTP 429 |
| Max request body | 1 MiB → HTTP 413 |

**Token hygiene:**

- Mint tokens with the minimum scopes required (`POST /api/tokens`).
- Persist `api_tokens` lines from the log immediately; plaintext cannot be recovered.
- Rotate tokens by deleting (`DELETE /api/tokens/{name}`) and minting anew.
- Prefer Bearer tokens over embedding the admin password in scripts.

**Scope enforcement:** Authenticated requests missing the required scope
receive **HTTP 403** with
`"Token does not have the required scope for this endpoint"`.

See [docs/api.md](docs/api.md) for the full scope-to-endpoint matrix.

### CSRF (browser sessions)

State-changing `/api/*` endpoints validate CSRF tokens when the request looks
like a cross-origin browser call (`Origin` / `Referer` present and not on the
allow list). Non-browser clients without those headers are exempt.

Allowed origins include localhost variants plus `csrf_allowed_origins`.
Misconfiguration can weaken CSRF defenses - only add origins you fully trust.

### Trusted subnets and auto-pairing

When `trusted_subnet_auto_pairing` is enabled, Moonlight clients whose source
IP falls within `trusted_subnets` (CIDR list) are **paired without a PIN**.

| Setting | Risk |
|---------|------|
| `trusted_subnet_auto_pairing = enabled` | Any host on listed subnets can pair silently |
| Overly broad CIDRs (e.g. `10.0.0.0/8`) | Expands silent-pairing surface |

**Recommendation:** Disable auto-pairing unless you control every device on the
listed subnets. Prefer explicit PIN pairing for guest networks and IoT VLANs.

Configuration:
[trusted_subnet_auto_pairing](docs/configuration.md#trusted_subnet_auto_pairing),
[trusted_subnets](docs/configuration.md#trusted_subnets).

### Webhooks

On stream start/end, SolarFlare POSTs JSON to every `webhook_url_*` entry
(fire-and-forget, short timeout, up to 2 retries per URL).

| Control | Purpose |
|---------|---------|
| HTTPS URLs only | Non-`http(s)` schemes are skipped |
| `webhook_secret` | When set, adds `X-Solarflare-Signature: sha256=<hmac>` over the body |

**Receiver obligations:** Verify the HMAC before acting on webhook payloads.
Treat webhook endpoints as authenticated callbacks, not public APIs.

**Payload fields:** `event` (`stream.start` / `stream.end`), session timing,
client address, codec, resolution, bitrate/RTT averages, `error` end reason.
Same shape as `/api/sessions` records.

**Threats:** A network observer who learns an unsigned webhook URL could spoof
events. Use `webhook_secret` and TLS on the receiver.

### GameStream / Moonlight path (non-REST)

Pairing, app launch, and streaming use `nvhttp` routes (`/pair`, `/launch`, …)
on the main HTTPS port, not `/api/*`. Compromise of a paired client certificate
allows session initiation subject to host app configuration.

TLS handshake concurrency on the GameStream HTTPS server is capped (64
concurrent handshakes) to reduce DoS via slow handshakes.

### Self-update supply chain

Linux self-update (`/api/update/*`) downloads release artifacts from GitHub,
verifies SHA-256 checksums, and stages before apply. Protect admin credentials
 -  `POST /api/update/apply` with `when_idle: false` can replace the running
binary immediately.

### Filesystem exposure

`GET /api/browse` lists directories accessible to the SolarFlare process.
Combine strong auth, `origin_web_ui_allowed = pc` or `lan`, and OS-level
permissions to limit readable paths.

---

## Security hardening checklist

| Priority | Action |
|----------|--------|
| High | Set a strong Web UI password; avoid default credentials |
| High | Keep `origin_web_ui_allowed` at `lan` or `pc` |
| High | Do not expose port 47990 to the Internet without a hardened reverse proxy |
| Medium | Use scoped API tokens instead of admin Basic Auth in automation |
| Medium | Enable `webhook_secret` and verify signatures on receivers |
| Medium | Disable `trusted_subnet_auto_pairing` unless strictly needed |
| Medium | Install a CA-signed TLS cert for the Web UI if browsers access it remotely |
| Low | Review `csrf_allowed_origins` - remove stale entries |
| Low | Prune unused paired clients periodically |

---

## Update policy

Critical security fixes from upstream Sunshine are cherry-picked onto this fork
as soon as upstream ships them. Monitor **both** repositories for security
advisories affecting inherited components (OpenSSL, FFmpeg, libcurl, etc. are
bundled or linked per platform).

Release notes: [docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md).

---

## Legal

Security research conducted in good faith on software you own or have permission
to test is welcome. Do not access systems or networks without authorization.
See [docs/legal.md](docs/legal.md) for licensing and trademark notices.
