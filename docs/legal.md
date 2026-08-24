# Licensing and Legal Considerations

> [!CAUTION]
> This documentation is for **informational purposes only** and is not legal
> advice. Consult a qualified attorney for questions about using, modifying, or
> distributing SolarFlare in your jurisdiction or business context.

SolarFlare is distributed under the
[GNU General Public License v3.0](../LICENSE) (GPL-3.0-only). The repository
contains work derived from [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine)
and other third-party components. Each component retains its own copyright
notices and license terms where applicable.

---

## SolarFlare and Sunshine relationship

| Aspect | Detail |
|---|---|
| **Copyright** | SolarFlare fork copyright holders per `LICENSE` and commit history |
| **Upstream** | Substantial code lineage from Sunshine (GPL-3.0) |
| **Trademarks** | "SolarFlare" is the fork product name; "Sunshine" and "Moonlight" are third-party marks |
| **Compatibility** | Binary name `sunshine`, config path `~/.config/sunshine`, and service IDs retained for client compatibility - not a trademark grant |

Fork-specific features (Web UI, network tuning, audio FX, API scopes) are
SolarFlare additions under the same GPL-3.0 license unless a file carries a
different SPDX header.

---

## GPL-3.0 summary for operators

The GPL permits:

- Running SolarFlare privately without distributing copies
- Modifying the source for personal use
- **Commercial use** of the software itself

The GPL requires when you **convey** (distribute) the program:

- Provide corresponding source or a written offer for source
- Include license and copyright notices
- Document changes in modified files (for derivative works)
- Use GPL-3.0 for derivative works you convey

Read the full [LICENSE](../LICENSE) text. This summary is not exhaustive.

---

## Third-party components and patents

SolarFlare links against and may bundle:

| Category | Examples | Separate terms |
|---|---|---|
| Video codecs | H.264, HEVC, AV1 (via FFmpeg, hardware encoders) | Patent pools, regional restrictions |
| GPU SDKs | NVENC, VA-API, AMF, VideoToolbox | Vendor SDK / driver EULAs |
| Audio | Opus, PipeWire, PulseAudio | Codec and library licenses |
| Networking | ENet, Moonlight protocol stack | Component licenses in `third-party/` |
| Fonts / assets | Web UI resources | Per-file notices in `src_assets/` |

**Deploying** SolarFlare does not automatically grant patent licenses from
codec patent holders or GPU vendors. Organizations distributing binaries or
appliances must evaluate encoder licensing for their markets.

Some Linux distributions ship Mesa builds **without** hardware encoders due to
patent policy. See [Troubleshooting - Hardware encoding](troubleshooting.md#hardware-encoding-fails).

---

## Redistribution checklist

If you redistribute SolarFlare binaries or derivative works:

1. **Source offer** - Provide source matching the binary (or link to exact tag)
2. **LICENSE** - Include `LICENSE` and preserve copyright notices
3. **NOTICE** - Aggregate third-party licenses from `third-party/` where required
4. **Trademarks** - Do not imply LizardByte, NVIDIA, or Moonlight endorsement
5. **Name** - Clearly label forks; do not pass modified builds as official Sunshine
6. **Patents / codecs** - Assess H.264/HEVC distribution rules in target regions

SolarFlare maintainers publish release artifacts on GitHub with checksums;
third-party repackagers should verify `SHA256SUMS` and state their changes.

---

## Commercial and SaaS scenarios

| Scenario | Typical consideration |
|---|---|
| Internal IT deployment on company LAN | Usually no conveyance if binaries stay internal; verify with counsel |
| Selling pre-built gaming appliances with SolarFlare | GPL source obligations + hardware codec patent review |
| Hosted "cloud gaming" using SolarFlare | GPL may apply to users receiving software; infrastructure-only use differs from conveying images |
| Web UI exposed to Internet | Security policy, not license - see [SECURITY.md](../SECURITY.md) |

GPL does **not** prohibit charging for distribution or support.

---

## Privacy and data handling

SolarFlare is self-hosted:

- No mandatory cloud telemetry or account system in the streaming path
- Pairing certificates and configuration remain on the host (`~/.config/sunshine/`)
- Optional webhooks (`webhook_url_*`) send events you configure to third-party URLs
- API tokens are stored hashed in configuration; protect `sunshine.conf` file permissions

Operators are responsible for GDPR, logging policy, and LAN exposure of the
HTTPS Web UI (default port 47990).

---

## Security disclosures

Report SolarFlare-specific vulnerabilities privately via
[GitHub Security Advisories](https://github.com/vindeckyy/Solar-Flare/security/advisories/new).
Do not file public issues for exploitable security bugs. See [SECURITY.md](../SECURITY.md).

---

## Upstream attribution

SolarFlare acknowledges:

- [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) - GameStream host foundation
- [Moonlight](https://moonlight-stream.org/) - client ecosystem
- Design inspiration noted in [README](../README.md#project-policy) (e.g. polaris review)

When publishing derivative works, maintain accurate attribution per GPL and
good open-source practice.

---

## See also

- [LICENSE](../LICENSE) - full GPL-3.0 text
- [SECURITY.md](../SECURITY.md) - threat model and reporting
- [CONTRIBUTING.md](../CONTRIBUTING.md) - contribution licensing (you grant rights to merge)
- [Third-party packages](third_party_packages.md) - bundled dependency licenses

<div class="section_buttons">

| Previous                                        |                              Next |
|:------------------------------------------------|----------------------------------:|
| [Gamestream Migration](gamestream_migration.md) | [Configuration](configuration.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
