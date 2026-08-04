# API

SolarFlare exposes a REST API for host administration and automation. Route
names remain compatible with Sunshine integrations.

Unless otherwise specified, authentication is required for all API calls. You can authenticate using
basic authentication with the admin username and password.

## CSRF Protection

State-changing API endpoints (POST, DELETE) are protected against Cross-Site Request Forgery (CSRF) attacks.

**For Web Browsers:**
- Requests from same-origin (configured via `csrf_allowed_origins`) are automatically allowed
- Cross-origin requests require a CSRF token

**For Non-Browser Applications:**
- Non-browser clients (e.g. `curl`, scripts, custom apps) are **exempt** from CSRF protection
- CSRF attacks require a browser to silently attach credentials to a cross-origin request. That threat
  does not apply to non-browser clients that explicitly provide credentials with every request
- Requests with no `Origin` or `Referer` header (as is typical for non-browser clients) are automatically
  allowed without a CSRF token

**Example (browser-equivalent cross-origin request):**
```bash
# Get CSRF token
curl -u user:pass https://localhost:47990/api/csrf-token

# Use token in request
curl -u user:pass -H "X-CSRF-Token: your_token_here" \
  -X POST https://localhost:47990/api/restart
```

@htmlonly
<script src="api.js"></script>
@endhtmlonly

## GET /api/csrf-token
@copydoc confighttp::getCSRFToken()

## GET /api/apps
@copydoc confighttp::getApps()

## POST /api/apps
@copydoc confighttp::saveApp()

## POST /api/apps/close
@copydoc confighttp::closeApp()

## DELETE /api/apps/{index}
@copydoc confighttp::deleteApp()

## GET /api/browse
@copydoc confighttp::browseDirectory()

## GET /api/clients/list
@copydoc confighttp::getClients()

## POST /api/clients/unpair
@copydoc confighttp::unpair()

## POST /api/clients/unpair-all
@copydoc confighttp::unpairAll()

## POST /api/clients/update
@copydoc confighttp::updateClient()

## GET /api/config
@copydoc confighttp::getConfig()

## GET /api/configLocale
@copydoc confighttp::getLocale()

## POST /api/config
@copydoc confighttp::saveConfig()

## GET /api/covers/{index}
@copydoc confighttp::getCover()

## POST /api/covers/upload
@copydoc confighttp::uploadCover()

## GET /api/logs
@copydoc confighttp::getLogs()

## POST /api/password
@copydoc confighttp::savePassword()

## POST /api/pin
@copydoc confighttp::savePin()

## POST /api/reset-display-device-persistence
@copydoc confighttp::resetDisplayDevicePersistence()

## POST /api/restart
@copydoc confighttp::restart()

## GET /api/stream/latency
@copydoc confighttp::getStreamLatency()

Requires authentication. API tokens need the `logs:get` scope (same as
`/api/logs`). Basic auth / admin `*` covers it.

Each metric object is `{ "min", "max", "avg", "samples" }` in milliseconds.
Metrics: `capture_ms`, `convert_ms`, `encode_ms`, `network_total_ms`
(capture-to-send total), `network_queue_dwell_ms`, `network_fec_ms`,
`network_send_ms`, `rtt_ms`. `effective_settings` holds the last encoder
snapshot (`codec`, `hwdevice`, `vendor`, `va_entrypoint`, `rc_mode`,
`quality`, `slices`, `async_depth`, `qmin`, `qmax`, `rc_buffer_size`,
`bit_rate`, `framerate`). Accumulators reset when the last streaming
session tears down; idle polls return zeros when no samples remain.

## POST /api/update/cancel
@copydoc confighttp::cancelUpdate()

Requires authentication and CSRF validation for browser clients. API tokens
need the `config:set` scope.

Cancels a pending when-idle apply while phase is `ready` or `waiting_idle`.
Success returns the updater status JSON (same shape as `GET /api/update`).
Errors return HTTP 400 with a message, including when phase is not
`ready`/`waiting_idle` ("No update operation is in progress") or the build
is not Linux ("Updates are only available on Linux"). Cancelling
`waiting_idle` clears the wait flag; if no wait worker is alive, phase
returns to `ready` immediately. Cancel does not stop download, verify, or
install once those phases have started.

## GET /api/vigembus/status
@copydoc confighttp::getViGEmBusStatus()

## POST /api/vigembus/install
@copydoc confighttp::installViGEmBus()

<div class="section_buttons">

| Previous                                    |                                  Next |
|:--------------------------------------------|--------------------------------------:|
| [Performance Tuning](performance_tuning.md) | [Troubleshooting](troubleshooting.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
