# REST API Reference

SolarFlare exposes a JSON-over-HTTPS REST API on the **Web UI port** (default
`47990`, one above the GameStream `port` setting). Route names and payloads
remain compatible with Sunshine integrations so existing automation continues
to work.

**Source of truth:** handlers are registered in `src/confighttp.cpp` inside
`confighttp::start()`. This document inventories **37** `/api/*` endpoints
(registered methods and paths as of the current tree).

@htmlonly
<script src="api.js"></script>
@endhtmlonly

---

## Quick reference

| Method | Path | Auth scope | CSRF on mutating calls |
|--------|------|------------|------------------------|
| `GET` | `/api/health` | None | - |
| `GET` | `/api/configLocale` | None | - |
| `GET` | `/api/csrf-token` | Any authenticated | - |
| `GET` | `/api/apps` | `apps:get` | - |
| `POST` | `/api/apps` | `config:set` | Yes |
| `DELETE` | `/api/apps/{index}` | `config:set` | Yes |
| `POST` | `/api/apps/close` | `apps:close` | Yes |
| `GET` | `/api/games/scan` | `apps:get` | - |
| `GET` | `/api/browse` | `config:get` | - |
| `GET` | `/api/clients/list` | `clients:list` | - |
| `POST` | `/api/clients/unpair` | `clients:unpair` | Yes |
| `POST` | `/api/clients/unpair-all` | `clients:unpair` | Yes |
| `POST` | `/api/clients/update` | `*` (admin) | Yes |
| `GET` | `/api/config` | `config:get` | - |
| `POST` | `/api/config` | `config:set` | Yes |
| `GET` | `/api/covers/{index}` | `apps:get` | - |
| `POST` | `/api/covers/upload` | `config:set` | No |
| `GET` | `/api/logs` | `logs:get` | - |
| `POST` | `/api/password` | `*` (when creds exist) | Yes |
| `POST` | `/api/pin` | `clients:pair` | Yes |
| `GET` | `/api/stream/bitrate` | `config:get` | - |
| `GET` | `/api/stream/latency` | `logs:get` | - |
| `GET` | `/api/stream/telemetry` | `logs:get` | - |
| `POST` | `/api/stream/network-stats` | `logs:get` | No |
| `GET` | `/api/sessions` | `logs:get` | - |
| `GET` | `/api/errors` | `logs:get` | - |
| `GET` | `/api/tokens` | `tokens:manage` | - |
| `POST` | `/api/tokens` | `tokens:manage` | No |
| `DELETE` | `/api/tokens/{name}` | `tokens:manage` | No |
| `POST` | `/api/reset-display-device-persistence` | `display:reset` | Yes |
| `POST` | `/api/restart` | `*` (admin) | Yes |
| `GET` | `/api/update` | `config:get` | - |
| `POST` | `/api/update/start` | `*` (admin) | Yes |
| `POST` | `/api/update/apply` | `*` (admin) | Yes |
| `POST` | `/api/update/cancel` | `config:set` | Yes |
| `GET` | `/api/vigembus/status` | `config:get` | - |
| `POST` | `/api/vigembus/install` | `*` (admin) | Yes |

> **Admin** means HTTP Basic Auth with the Web UI username/password, or a Bearer
> token that includes the `*` scope. Basic Auth implicitly grants every scope.

---

## Base URL and transport

```
https://<host>:<web-ui-port>/api/...
```

| Setting | Default | Notes |
|---------|---------|-------|
| GameStream `port` | `47989` | Moonlight streaming port |
| Web UI port | `port + 1` → `47990` | `confighttp` HTTPS listener (`PORT_HTTPS = 1`) |
| TLS | Required | Self-signed certificate generated on first run (`cert` / `pkey` in config) |
| `Content-Type` | `application/json` | Required on POST bodies unless noted (plain text for `/api/logs` response) |
| Max body size | 1 MiB | Larger requests are rejected with HTTP 413 before handler logic runs |

Use `-k` / `--insecure` with curl when testing against the default self-signed
certificate, or install a trusted cert via the `cert` and `pkey` config keys.

```bash
export SF_HOST="https://localhost:47990"
export SF_USER="admin"
export SF_PASS="your_password"
```

---

## Authentication

Unless noted **Unauthenticated**, every endpoint requires valid credentials.

### HTTP Basic Auth (full admin)

```http
Authorization: Basic base64(username:password)
```

Basic Auth grants **all scopes** (`is_admin = true`). Use this for interactive
administration and one-off scripts that need unrestricted access.

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/config"
```

### Bearer API tokens (scoped automation)

```http
Authorization: Bearer <64-char-hex-plaintext-token>
```

Tokens are defined in `sunshine.conf` under `api_tokens` (stored as
SHA-256 hashes; plaintext is shown **once** when minted via `POST /api/tokens`).
Each token carries an explicit scope list. A token with `*` behaves like admin
for authorization checks.

```bash
curl -sk -H "Authorization: Bearer abc123...def" "$SF_HOST/api/config"
```

#### Scope strings

| Scope | Grants access to |
|-------|------------------|
| `config:get` | `GET /api/config`, `GET /api/browse`, `GET /api/stream/bitrate`, `GET /api/update`, `GET /api/vigembus/status` |
| `config:set` | `POST /api/config`, `POST /api/apps`, `DELETE /api/apps/{index}`, `POST /api/covers/upload`, `POST /api/update/cancel` |
| `apps:get` | `GET /api/apps`, `GET /api/games/scan`, `GET /api/covers/{index}` |
| `apps:launch` | *(reserved scope name; app create/update uses `config:set` in current code)* |
| `apps:close` | `POST /api/apps/close` |
| `clients:list` | `GET /api/clients/list` |
| `clients:pair` | `POST /api/pin` |
| `clients:unpair` | `POST /api/clients/unpair`, `POST /api/clients/unpair-all` |
| `logs:get` | `GET /api/logs`, `GET /api/stream/latency`, `GET /api/stream/telemetry`, `POST /api/stream/network-stats`, `GET /api/sessions`, `GET /api/errors` |
| `display:reset` | `POST /api/reset-display-device-persistence` |
| `tokens:manage` | `GET/POST/DELETE /api/tokens` |
| `*` | All endpoints that require admin (`clients/update`, `password`, `restart`, `update/start`, `update/apply`, `vigembus/install`, …) |

### Network origin restriction

Before credentials are checked, the client IP is compared against
`origin_web_ui_allowed` (`pc` | `lan` | `wan`, default `lan`). Requests from
disallowed network classes receive **HTTP 403** with an empty body (no JSON).

### Brute-force rate limiting

Failed authentication attempts are tracked per source IP: **10 failures per
30 seconds** triggers **HTTP 429** until the window expires. Successful auth
resets the counter for that IP.

### First-run / unset credentials

When no Web UI username is configured, authenticated endpoints redirect to
`/welcome` (HTTP 307) instead of returning JSON.

---

## CSRF protection

`POST` and `DELETE` handlers that call `validate_csrf_token()` require CSRF
defense **only for browser-like cross-origin requests** (an `Origin` or
`Referer` header that does not match an allowed origin).

| Client type | CSRF required? |
|-------------|----------------|
| `curl`, scripts, automation (no `Origin`/`Referer`) | **No** - exempt |
| Same-origin browser (matches built-in or `csrf_allowed_origins`) | **No** |
| Cross-origin browser | **Yes** - `X-CSRF-Token` header or `csrf_token` query param |

CSRF tokens are obtained from `GET /api/csrf-token`, valid for **1 hour**, keyed
by Basic Auth username or client IP.

```bash
# Browser-equivalent cross-origin flow
TOKEN=$(curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/csrf-token" | jq -r .csrf_token)
curl -sk -u "$SF_USER:$SF_PASS" \
  -H "Origin: https://evil.example" \
  -H "X-CSRF-Token: $TOKEN" \
  -X POST "$SF_HOST/api/restart"
```

---

## Common response envelope

### Success (typical JSON endpoints)

```json
{
  "status": true,
  "status_code": 200
}
```

Many endpoints add domain-specific keys alongside `status`.

### Error JSON

```json
{
  "status": false,
  "status_code": 400,
  "error": "Human-readable message"
}
```

| HTTP | When | `error` field |
|------|------|---------------|
| **400** | Malformed input, validation failure, CSRF failure | Present |
| **401** | Missing or invalid credentials | `"Unauthorized"` + `WWW-Authenticate: Basic …` |
| **403** | Authenticated but missing required scope | `"Token does not have the required scope for this endpoint"` |
| **403** | IP blocked by `origin_web_ui_allowed` | *(empty body)* |
| **404** | Missing resource (e.g. cover image, token name) | Present |
| **413** | Request body > 1 MiB | *(empty body)* |
| **415** | Wrong `Content-Type` on POST | `"Content type mismatch"` or `"Content type not provided"` |
| **429** | Login rate limit exceeded | *(empty body)* |

Security headers on JSON responses: `X-Frame-Options: DENY`,
`Content-Security-Policy: frame-ancestors 'none';`,
`Strict-Transport-Security: max-age=31536000; includeSubDomains`.

---

## Endpoints

### `GET /api/health`

**Auth:** None.

Liveness probe for load balancers and containers.

**Response `200`:**

```json
{
  "status": "ok",
  "status_code": 200,
  "version": "2026.824.1",
  "uptime": 3600
}
```

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Always `"ok"` when reachable |
| `version` | string | `PROJECT_VERSION` compile-time string |
| `uptime` | integer | Seconds since the confighttp server started |

```bash
curl -sk "$SF_HOST/api/health" | jq .
```

---

### `GET /api/configLocale`

**Auth:** None.

Returns the configured UI locale without requiring login (used before auth on
the welcome screen).

**Response `200`:**

```json
{
  "status": true,
  "locale": "en"
}
```

---

### `GET /api/csrf-token`

**Auth:** Any valid Basic or Bearer credentials.

**Response `200`:**

```json
{
  "csrf_token": "<random-256-bit-token>"
}
```

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/csrf-token"
```

---

### `GET /api/apps`

**Auth:** `apps:get`

Returns the full `apps.json` document. Legacy string booleans/integers are
normalized to JSON types in the response.

**Response `200`:**

```json
{
  "apps": [
    {
      "name": "Desktop",
      "cmd": "",
      "image-path": "/path/to/cover.png",
      "elevated": false,
      "auto-detach": true,
      "prep-cmd": [],
      "detached": []
    }
  ]
}
```

| Error | Cause |
|-------|-------|
| 400 | `apps.json` exists but is not valid JSON |

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/apps" | jq '.apps | length'
```

---

### `POST /api/apps`

**Auth:** `config:set` · **CSRF:** Yes · **Content-Type:** `application/json`

Create (`index: -1`) or update (`index: 0..n-1`) an application entry. Apps
are re-sorted alphabetically by `name` after save.

**Request body:**

```json
{
  "name": "My Game",
  "output": "",
  "cmd": "/usr/games/mygame",
  "index": -1,
  "exclude-global-prep-cmd": false,
  "elevated": false,
  "auto-detach": true,
  "wait-all": true,
  "exit-timeout": 5,
  "prep-cmd": [
    { "do": "", "undo": "", "elevated": false }
  ],
  "detached": [],
  "image-path": "/path/to/cover.png"
}
```

| Field | Required | Notes |
|-------|----------|-------|
| `index` | **Yes** | `-1` = append; `0..n-1` = replace at index |
| `name` | **Yes** | string |
| Other fields | No | Empty `prep-cmd` / `detached` arrays are stripped before save |

**Response `200`:** `{"status": true}`

| Error | Cause |
|-------|-------|
| 400 | Missing/invalid `index` or `name`, index out of range, JSON parse error |

```bash
curl -sk -u "$SF_USER:$SF_PASS" -H "Content-Type: application/json" \
  -d '{"name":"Test App","cmd":"echo hi","index":-1}' \
  -X POST "$SF_HOST/api/apps"
```

---

### `DELETE /api/apps/{index}`

**Auth:** `config:set` · **CSRF:** Yes

`{index}` is a non-negative integer path segment.

**Response `200`:**

```json
{
  "status": true,
  "result": "application 2 deleted"
}
```

| Error | Cause |
|-------|-------|
| 400 | Index out of range, no apps configured |

```bash
curl -sk -u "$SF_USER:$SF_PASS" -X DELETE "$SF_HOST/api/apps/0"
```

---

### `POST /api/apps/close`

**Auth:** `apps:close` · **CSRF:** Yes

Terminates the currently running application process.

**Response `200`:** `{"status": true}`

```bash
curl -sk -u "$SF_USER:$SF_PASS" -X POST "$SF_HOST/api/apps/close"
```

---

### `GET /api/games/scan`

**Auth:** `apps:get`

Scans the local host for Steam, Lutris, and Heroic installations (Linux).

**Response `200`:** JSON **array** (not wrapped in an object):

```json
[
  {
    "name": "Hades",
    "path": "/home/user/.steam/.../Hades.exe",
    "launcher": "steam",
    "cover_url": "https://..."
  }
]
```

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/games/scan" | jq 'length'
```

---

### `GET /api/browse`

**Auth:** `config:get`

Directory browser for the Web UI file picker.

**Query parameters:**

| Param | Default | Values |
|-------|---------|--------|
| `path` | `/` (Linux) or drive list (Windows) | Absolute directory path |
| `type` | `any` | `directory`, `executable`, `file`, `any` |

**Response `200`:**

```json
{
  "path": "/home/user",
  "parent": "/home",
  "entries": [
    { "name": "Documents", "path": "/home/user/Documents", "type": "directory" },
    { "name": "game.bin", "path": "/home/user/game.bin", "type": "file" }
  ]
}
```

| Error | Cause |
|-------|-------|
| 400 | Path is not a directory, filesystem error |

```bash
curl -sk -u "$SF_USER:$SF_PASS" \
  "$SF_HOST/api/browse?path=/home/user&type=executable"
```

---

### `GET /api/clients/list`

**Auth:** `clients:list`

**Response `200`:**

```json
{
  "status": true,
  "named_certs": [
    {
      "name": "Living Room TV",
      "uuid": "12345678-1234-1234-1234-123456789abc",
      "enabled": true
    }
  ]
}
```

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/clients/list" | jq '.named_certs'
```

---

### `POST /api/clients/unpair`

**Auth:** `clients:unpair` · **CSRF:** Yes · **Content-Type:** `application/json`

> **Note:** Registered path is `/api/clients/unpair` (not `/api/unpair`).

**Request body:**

```json
{ "uuid": "12345678-1234-1234-1234-123456789abc" }
```

UUID must match canonical hyphenated form (case-insensitive hex).

**Response `200`:** `{"status": true}` if the client was removed, `false` if not found.

| Error | Cause |
|-------|-------|
| 400 | Invalid UUID format |

```bash
curl -sk -u "$SF_USER:$SF_PASS" -H "Content-Type: application/json" \
  -d '{"uuid":"12345678-1234-1234-1234-123456789abc"}' \
  -X POST "$SF_HOST/api/clients/unpair"
```

---

### `POST /api/clients/unpair-all`

**Auth:** `clients:unpair` · **CSRF:** Yes

Removes every paired client and terminates any running app.

**Response `200`:** `{"status": true}`

---

### `POST /api/clients/update`

**Auth:** `*` (admin only) · **CSRF:** Yes · **Content-Type:** `application/json`

Enable or disable a paired client. Disabling terminates active sessions for
that certificate and may stop the running app.

**Request body:**

```json
{
  "uuid": "12345678-1234-1234-1234-123456789abc",
  "enabled": false
}
```

**Response `200`:** `{"status": true}` or `{"status": false}`

---

### `GET /api/config`

**Auth:** `config:get`

Returns all sunshine.conf key/value pairs plus runtime defaults for SolarFlare
extensions (audio FX, headless mode, adaptive bitrate, trusted subnets).

**Response `200`:**

```json
{
  "status": true,
  "platform": "Linux",
  "version": "2026.824.1",
  "locale": "en",
  "port": "47989",
  "adaptive_bitrate_enabled": "enabled"
}
```

Keys mirror on-disk config names; boolean options appear as `"enabled"` /
`"disabled"` strings when emitted as defaults.

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/config" | jq 'keys | length'
```

---

### `POST /api/config`

**Auth:** `config:set` · **CSRF:** Yes · **Content-Type:** `application/json`

Merge key/value pairs into `sunshine.conf`. Only include keys that differ from
defaults. Null or empty string values are skipped (not written).

**Request body:**

```json
{
  "locale": "en",
  "fps": "60"
}
```

**Response `200`:** `{"status": true}`

**Response `200` (write failure):**

```json
{
  "status": false,
  "error": "Failed to write config file to disk"
}
```

| Error | Cause |
|-------|-------|
| 400 | Body is not a JSON object, empty payload (would wipe config), parse error |

```bash
curl -sk -u "$SF_USER:$SF_PASS" -H "Content-Type: application/json" \
  -d '{"locale":"en"}' -X POST "$SF_HOST/api/config"
```

---

### `GET /api/covers/{index}`

**Auth:** `apps:get`

Returns the PNG cover image for application `{index}`.

**Response `200`:** `Content-Type: image/png` (binary stream)

| Error | Cause |
|-------|-------|
| 404 | No valid cover configured for this app |
| 400 | Index out of range, unreadable file |

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/covers/0" -o cover.png
```

---

### `POST /api/covers/upload`

**Auth:** `config:set` · **CSRF:** No · **Content-Type:** `application/json`

Download or upload a cover image into `<appdata>/covers/`.

**Request body (IGDB download):**

```json
{
  "key": "igdb_12345",
  "url": "https://images.igdb.com/igdb/image/upload/t_cover_big_2x/abc.png"
}
```

**Request body (base64 inline):**

```json
{
  "key": "custom_game",
  "data": "<base64-encoded-png>"
}
```

| Field | Notes |
|-------|-------|
| `key` | Filename stem; must not contain `/`, `..`, or NUL |
| `url` | Only `images.igdb.com` host allowed |
| `data` | Used when `url` is empty |

**Response `200`:**

```json
{
  "status": true,
  "path": "/home/user/.config/sunshine/covers/igdb_12345.png"
}
```

| Error | Cause |
|-------|-------|
| 400 | Invalid key, disallowed URL host, download failure |

---

### `GET /api/logs`

**Auth:** `logs:get`

**Response `200`:** `Content-Type: text/plain` - full contents of the log file.

```bash
curl -sk -u "$SF_USER:$SF_PASS" "$SF_HOST/api/logs" | tail -20
```

---

### `POST /api/password`

**Auth:** `*` when username already set; unrestricted body validation on first
setup · **CSRF:** Yes · **Content-Type:** `application/json`

Change Web UI credentials stored in the credentials file.

**Request body:**

```json
{
  "currentUsername": "admin",
  "currentPassword": "old",
  "newUsername": "admin",
  "newPassword": "newsecret",
  "confirmNewPassword": "newsecret"
}
```

**Response `200`:** `{"status": true}`

| Error | Cause |
|-------|-------|
| 400 | Invalid username, password mismatch, wrong current credentials |

---

### `POST /api/pin`

**Auth:** `clients:pair` · **CSRF:** Yes · **Content-Type:** `application/json`

Submit a Moonlight pairing PIN displayed on the client.

**Request body:**

```json
{
  "pin": "1234",
  "name": "My Phone"
}
```

PIN must be `0000`–`9999`.

**Response `200`:** `{"status": true}` or `{"status": false}` depending on pairing outcome.

---

### `GET /api/stream/bitrate`

**Auth:** `config:get`

**Response `200`:**

```json
{
  "status": true,
  "status_code": 200,
  "adaptive_bitrate_enabled": true,
  "adaptive_bitrate_min_kbps": 5000,
  "adaptive_bitrate_max_kbps": 50000
}
```

---

### `GET /api/stream/latency`

**Auth:** `logs:get`

Host-side latency statistics for the active or most recent streaming session.
Each metric is `{min, max, avg, samples}` in milliseconds. Accumulators reset
when the last session tears down.

**Response `200`:**

```json
{
  "status": true,
  "status_code": 200,
  "capture_ms": { "min": 1.2, "max": 3.4, "avg": 2.1, "samples": 120 },
  "convert_ms": { "min": 0.0, "max": 0.5, "avg": 0.1, "samples": 120 },
  "encode_ms": { "min": 4.0, "max": 8.0, "avg": 5.5, "samples": 120 },
  "network_total_ms": { "min": 0.0, "max": 0.0, "avg": 0.0, "samples": 0 },
  "network_queue_dwell_ms": { "min": 0.0, "max": 0.0, "avg": 0.0, "samples": 0 },
  "network_fec_ms": { "min": 0.0, "max": 0.0, "avg": 0.0, "samples": 0 },
  "network_send_ms": { "min": 0.0, "max": 0.0, "avg": 0.0, "samples": 0 },
  "rtt_ms": { "min": 0.0, "max": 0.0, "avg": 0.0, "samples": 0 },
  "effective_settings": {
    "codec": "hevc_vaapi",
    "hwdevice": "/dev/dri/renderD128",
    "vendor": "amd",
    "va_entrypoint": "encSlice",
    "rc_mode": "cqp",
    "quality": 28,
    "slices": 1,
    "async_depth": 2,
    "qmin": 20,
    "qmax": 35,
    "rc_buffer_size": 0,
    "bit_rate": 20000,
    "framerate": 60
  }
}
```

---

### `GET /api/stream/telemetry`

**Auth:** `logs:get`

Host CPU/RAM/GPU time series (Linux only; 10-minute window at 1 Hz).

**Response `200`:**

```json
{
  "status": true,
  "status_code": 200,
  "telemetry": {
    "window_s": 600,
    "host_cpu_pct": [12.5, 13.1, 14.0],
    "host_ram_used_mb": [4096, 4100, 4112],
    "host_gpu_pct": [45.0, 46.2, 47.1]
  }
}
```

On non-Linux platforms `telemetry` contains only `window_s`.

---

### `POST /api/stream/network-stats`

**Auth:** `logs:get` · **CSRF:** No

Ingest client-reported network feedback for adaptive bitrate control.

**Request body:**

```json
{
  "packet_loss_pct": 0.5,
  "rtt_ms": 23.4
}
```

| Field | Constraints |
|-------|-------------|
| `packet_loss_pct` | 0–100 |
| `rtt_ms` | ≥ 0 |

**Response `200`:** `{"status": true, "status_code": 200}`

```bash
curl -sk -u "$SF_USER:$SF_PASS" -H "Content-Type: application/json" \
  -d '{"packet_loss_pct":0.1,"rtt_ms":12.5}' \
  -X POST "$SF_HOST/api/stream/network-stats"
```

---

### `GET /api/sessions`

**Auth:** `logs:get`

Reads `<appdata>/session_history.jsonl` (oldest first within the returned window).

**Query parameters:**

| Param | Default | Description |
|-------|---------|-------------|
| `limit` | `100` | Max records |
| `app` | *(none)* | Substring filter on `app_name` |
| `client` | *(none)* | Substring filter on `client_name` |

**Response `200`:**

```json
{
  "status": true,
  "status_code": 200,
  "sessions": [
    {
      "t_start": 1710000000,
      "t_end": 1710003600,
      "app_name": "Desktop",
      "client_name": "Moonlight PC",
      "client_address": "192.168.1.50",
      "codec": "hevc_vaapi",
      "width": 1920,
      "height": 1080,
      "fps": 60,
      "avg_bitrate_kbps": 25000.0,
      "avg_rtt_ms": 5.2,
      "avg_encode_ms": 4.8,
      "dropped_frames": 0,
      "error": ""
    }
  ]
}
```

`error` is empty for a clean stop; otherwise contains an end reason (e.g.
`idle_timeout`).

```bash
curl -sk -u "$SF_USER:$SF_PASS" \
  "$SF_HOST/api/sessions?limit=10&app=Desktop"
```

---

### `GET /api/errors`

**Auth:** `logs:get`

Monotonic process-wide error counters since startup (incremented by `SUN_ERR()`).

**Response `200`:**

```json
{
  "status": true,
  "status_code": 200,
  "encoder": 0,
  "capture": 0,
  "network": 0,
  "session": 0,
  "process": 0,
  "config": 0,
  "crypto": 0,
  "unknown": 0,
  "total": 0
}
```

---

### `GET /api/tokens`

**Auth:** `tokens:manage`

**Response `200`:**

```json
{
  "status": true,
  "status_code": 200,
  "tokens": [
    {
      "name": "ci-bot",
      "scopes": ["config:get", "apps:close"]
    }
  ]
}
```

Hashes and salts are never returned.

---

### `POST /api/tokens`

**Auth:** `tokens:manage` · **CSRF:** No · **Content-Type:** `application/json`

Mint a new token. Plaintext is returned **once**; persist the `api_tokens`
line printed to the log into `sunshine.conf`.

**Request body:**

```json
{
  "name": "ci-bot",
  "scopes": ["config:get", "logs:get"]
}
```

**Response `200`:**

```json
{
  "status": true,
  "status_code": 200,
  "name": "ci-bot",
  "plaintext": "<64-char-hex>",
  "scopes": ["config:get", "logs:get"],
  "warning": "Add the api_tokens line printed to the sunshine log to sunshine.conf to persist."
}
```

| Error | Cause |
|-------|-------|
| 400 | Missing name, duplicate name, empty/invalid scopes, unknown scope string |

---

### `DELETE /api/tokens/{name}`

**Auth:** `tokens:manage` · **CSRF:** No

`{name}` matches `[\w-]+` (word chars and hyphen).

**Response `200` (found):** `{"status": true, "status_code": 200}`

**Response `200` (not found):**

```json
{
  "status": false,
  "status_code": 404,
  "error": "Token not found"
}
```

Deletion is in-memory until config is edited and the service restarted.

---

### `POST /api/reset-display-device-persistence`

**Auth:** `display:reset` · **CSRF:** Yes

Clears display-device persistence state (multi-monitor layout memory).

**Response `200`:** `{"status": true}` or `{"status": false}`

---

### `POST /api/restart`

**Auth:** `*` · **CSRF:** Yes

Restarts the host process. **May not return a response** if restart succeeds.

```bash
curl -sk -u "$SF_USER:$SF_PASS" -X POST "$SF_HOST/api/restart"
```

---

### `GET /api/update`

**Auth:** `config:get`

SolarFlare Linux self-update status.

**Response `200`:**

```json
{
  "phase": "idle",
  "percent": -1,
  "message": "",
  "latest_tag": "",
  "html_url": "",
  "log": [],
  "outdated": false,
  "can_apply": false,
  "busy": false,
  "helper_path": "/usr/local/bin/solarflare-apply-helper"
}
```

| `phase` values | Meaning |
|----------------|---------|
| `idle` | No update activity |
| `checking` | Resolving latest GitHub release |
| `downloading` | Fetching release artifacts |
| `verifying` | SHA-256 verification / extraction |
| `ready` | Staged and ready to apply |
| `waiting_idle` | Apply queued until streams end |
| `applying` | Installing |
| `restarting` | Host restart after apply |
| `error` | Last operation failed - see `message` and `log` |
| `unsupported` | Non-Linux platform |

---

### `POST /api/update/start`

**Auth:** `*` · **CSRF:** Yes

Begin background download and verification of the latest Linux release.

**Response `200`:** Updater status object (same shape as `GET /api/update`).

| Error | Cause |
|-------|-------|
| 400 | Update already in progress, platform unsupported, network error (see `message`) |

---

### `POST /api/update/apply`

**Auth:** `*` · **CSRF:** Yes

Apply a staged update.

**Request body (optional):**

```json
{ "when_idle": true }
```

| `when_idle` | Behavior |
|-------------|----------|
| `false` (default) | Apply immediately |
| `true` | Wait until no active streaming sessions |

**Response `200`:** Updater status object.

| Error | Cause |
|-------|-------|
| 400 | Nothing staged, wrong phase, install failure |

---

### `POST /api/update/cancel`

**Auth:** `config:set` · **CSRF:** Yes

Cancel a pending when-idle apply. Accepted only when `phase` is `ready` or
`waiting_idle`. Does not stop an in-progress download, verify, or install.

**Response `200`:** Updater status object.

| Error | Cause |
|-------|-------|
| 400 | Wrong phase, non-Linux platform |

---

### `GET /api/vigembus/status`

**Auth:** `config:get`

Windows virtual gamepad driver status. On Linux, returns `installed: false` with
an explanatory `error` field.

**Response `200` (Windows):**

```json
{
  "installed": true,
  "version": "1.22.0.0",
  "version_compatible": true,
  "packaged_version": "1.22.0.0"
}
```

Compatibility requires ViGEmBus ≥ 1.17.

---

### `POST /api/vigembus/install`

**Auth:** `*` · **CSRF:** Yes

Runs the bundled ViGEmBus installer elevated (Windows only).

**Response `200`:**

```json
{
  "status": true,
  "exit_code": 0
}
```

| Error in body | Cause |
|---------------|-------|
| `"ViGEmBus installation is only available on Windows"` | Linux/macOS |
| `"ViGEmBus installer not found"` | Missing packaged installer |
| `"Installer exited with code N"` | Non-zero installer exit |

---

## Related interfaces (not `/api/*`)

SolarFlare also serves:

| Prefix | Purpose |
|--------|---------|
| `/` | Web UI HTML pages (`index.html`, `apps.html`, …) |
| `/assets/*` | Bundled CSS/JS (content-hashed filenames) |
| `/images/*`, `/sw.js`, `/manifest.webmanifest` | PWA static assets |
| GameStream HTTPS (`nvhttp`, default port `47989`) | `/serverinfo`, `/pair`, `/applist`, `/launch`, … for Moonlight clients |

Moonlight pairing and streaming do **not** use the `/api/*` REST surface;
they use the GameStream protocol on the main HTTPS/HTTP ports documented in
[Configuration](configuration.md).

Outbound **webhooks** (`webhook_url_*` config keys) are server-initiated POSTs
to your URLs on stream start/stop - not inbound API routes. See
[SECURITY.md](../SECURITY.md) for signature verification (`X-Solarflare-Signature`).

---

<div class="section_buttons">

| Previous                                    |                                  Next |
|:--------------------------------------------|--------------------------------------:|
| [Performance Tuning](performance_tuning.md) | [Troubleshooting](troubleshooting.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
