# GameStream Migration

In February 2023, NVIDIA discontinued the GameStream service for NVIDIA Shield
devices and GeForce Experience. SolarFlare provides a modern, low-latency,
Moonlight-compatible host replacement that preserves local-network streaming
without NVIDIA's cloud relay.

This guide covers automated migration tools, manual `apps.json` transfer,
pairing certificate behavior, internet streaming replacements, and behavioral
differences from legacy GameStream.

---

## Before you migrate

| Check | Action |
|---|---|
| Moonlight installed on clients | [moonlight-stream.org](https://moonlight-stream.org/) |
| SolarFlare host built | [README - Install](../README.md#install) |
| Backup GameStream data | Copy NVIDIA/Sunshine `apps.json`, box art, pairing state if applicable |
| Note encoder settings | GameStream quality presets do not map 1:1 - plan Moonlight bitrate/codec |

> [!IMPORTANT]
> SolarFlare keeps configuration under `~/.config/sunshine/` and the `sunshine`
> binary name for compatibility. Existing **Sunshine** pairings often survive a
> fork upgrade; **NVIDIA GameStream** pairings require fresh pairing with
> SolarFlare.

---

## Automated migration with GSMS

The upstream [GSMS](https://github.com/LizardByte/GSMS) (GameStream Migration
Service) tool migrates legacy GameStream games and applications into SolarFlare's
`apps.json` format.

**GSMS can:**

- Import custom and auto-detected games
- Populate working directory, launch command, and title
- Copy box-art images into SolarFlare asset directories
- Map fields compatible with Moonlight app lists

**Typical workflow:**

1. Install and run GSMS per its README on a machine with access to legacy data
2. Point output at SolarFlare's config directory (`~/.config/sunshine/`)
3. Restart SolarFlare or reload apps from the Web UI
4. Verify each imported entry launches from Moonlight

> [!NOTE]
> GSMS is maintained by LizardByte, not the SolarFlare fork. Report GSMS bugs
> upstream. SolarFlare-specific launch flags (encoder overrides, headless display)
> may require manual `apps.json` edits after import - see
> [Application examples](app_examples.md).

---

## Manual migration

### Application entries (`apps.json`)

GameStream and Sunshine use a JSON array of application objects. Minimal example:

```json
{
  "name": "Desktop",
  "cmd": "bash -c 'sleep infinity'",
  "image-path": "desktop.png"
}
```

SolarFlare-extended fields include `encoder-preset`, `prep-cmd`, `detached`,
`elevated`, and per-app environment overrides. Full schema:
[Application examples](app_examples.md).

**Manual steps:**

1. Export or copy your old `apps.json` from the previous host
2. Merge into `~/.config/sunshine/apps.json` (validate JSON with `jq .`)
3. Copy `assets/` or image paths referenced by `image-path`
4. Open Web UI → Applications → confirm list matches
5. Test launch from Moonlight for each critical title

### Pairing and certificates

| Source | Pairing state |
|---|---|
| NVIDIA GameStream (GeForce Experience) | **Not portable** - unpair clients, pair to SolarFlare |
| Sunshine → SolarFlare upgrade | Usually **portable** - same cert store under `~/.config/sunshine/` |
| Fresh SolarFlare install | New PIN pairing required |

If Moonlight shows "paired" but streams fail, unpair on **both** client and host,
delete stale certs only if you understand the implications (backup first), and
re-pair via Web UI PIN.

---

## Internet streaming

### Moonlight Internet Hosting Tool (deprecated path)

If you used the Moonlight Internet Hosting Tool with GameStream, you can remove
it after migrating to SolarFlare for LAN use.

For **internet** streaming with SolarFlare and a UPnP-capable router:

1. Enable UPnP in the SolarFlare Web UI (Network settings)
2. Ensure router supports UPnP and does not double-NAT
3. Forward ports manually if UPnP fails - see
   [Troubleshooting - ports](troubleshooting.md#discovery-firewall-and-ports)

> [!WARNING]
> Running SolarFlare together with Moonlight Internet Hosting Tool **before
> v5.6** causes unreliable UPnP port forwarding. Uninstall the old tool or
> update to v5.6+.

### VPN alternative

Many operators prefer WireGuard/Tailscale instead of exposing GameStream ports:

- Moonlight connects to the VPN IP of the host
- No UPnP required; reduces public attack surface
- Latency depends on VPN path - test before relying on it for gaming

---

## Behavioral differences from GameStream

| GameStream / GFE | SolarFlare |
|---|---|
| NVIDIA-optimized in-game settings injection | **No automatic** graphics config overwrite |
| Shield-optimized app list | Managed via `apps.json` + Web UI scanner |
| NVIDIA account pairing | Local PIN / trusted subnets |
| Automatic game detection (NVIDIA) | Built-in scanner: Steam, Lutris, Heroic (`/api/games/scan`) |
| Virtual display on Shield | `headless_virtual_display` + KMS (Linux) |
| Cloud-assisted remote play | Self-hosted only; no NVIDIA relay |

### Graphics settings

SolarFlare does **not** modify game config files on launch to force resolution
or quality. Set in-game options yourself or use per-title launch wrappers
(Gamescope, `gamescope -W 1920 -H 1080`, etc.) documented in
[Application examples](app_examples.md).

### Virtual displays and desktops

Legacy GameStream "GameStream Optimized" titles assumed NVIDIA driver hooks.
On SolarFlare Linux:

- Use **Desktop** app entry for full desktop streaming
- Headless servers: enable `headless_virtual_display` - see
  [CONFIGURATION](CONFIGURATION.md#headless_virtual_display)

---

## Post-migration checklist

- [ ] All Moonlight clients re-paired to SolarFlare host
- [ ] `apps.json` entries launch successfully
- [ ] Encoder selected (NVENC / VA-API) in Web UI Video tab
- [ ] Firewall allows TCP 47984–47990 and UDP 47998–48000
- [ ] Fork tunables reviewed - [CONFIGURATION](CONFIGURATION.md)
- [ ] Old GameStream / GFE services disabled to avoid port conflicts
- [ ] Internet access policy decided (UPnP vs VPN vs LAN-only)

---

## Troubleshooting migration issues

| Problem | Fix |
|---|---|
| App missing in Moonlight | Refresh app list; check `apps.json` syntax |
| Wrong resolution | Client bitrate/resolution; headless `headless_*` keys |
| GSMS images broken | Verify `image-path` relative to assets directory |
| Launch command works in shell but not app | Test as same user as systemd service; add `prep-cmd` |
| Steam titles fail | Use `steam steam://rungameid/<id>` form; see app examples |

Full diagnostics: [Troubleshooting](troubleshooting.md).

---

## See also

- [Getting started](getting_started.md) - fresh Linux install
- [Application examples](app_examples.md) - `apps.json` patterns
- [API - game scanner](api.md) - `/api/games/scan`
- [Third-party packages](third_party_packages.md) - community Sunshine packages (not SolarFlare)

<div class="section_buttons">

| Previous                                        |              Next |
|:------------------------------------------------|------------------:|
| [Third-party Packages](third_party_packages.md) | [Legal](legal.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
