# GameStream Migration
In February 2023, NVIDIA discontinued the GameStream service for NVIDIA Shield devices and GeForce Experience.
SolarFlare provides a modern, low-latency, Moonlight-compatible host replacement.

## Migration
The upstream GSMS tool can migrate legacy GameStream games and apps into SolarFlare's
compatible application format.
Please check out the [GSMS](https://github.com/LizardByte/GSMS) project if you're interested in an automated
migration option. GSMS offers the ability to migrate custom and auto-detected games and apps. The
working directory, command, and image are all populated into SolarFlare's `apps.json` configuration. The box-art image is also copied
to the target asset directory.

## Internet Streaming
If you are using the Moonlight Internet Hosting Tool, you can remove it from your system when you migrate to SolarFlare.
To stream over the Internet with SolarFlare and a UPnP-capable router, enable UPnP in the SolarFlare Web UI.

> [!NOTE]
> Running SolarFlare together with versions of the Moonlight Internet Hosting Tool prior to v5.6 will cause UPnP
> port forwarding to become unreliable. Either uninstall the tool entirely or update it to v5.6 or later.

## Differences and Considerations
SolarFlare includes automated game detection for Steam, Lutris, and Heroic via its built-in game scanner (`/api/games/scan` and Web UI App Manager). Note the following differences from legacy GameStream:

* SolarFlare does not automatically overwrite in-game graphics configuration files to adjust resolution/quality presets on launch.
* Applications and virtual displays are managed directly through SolarFlare configuration or per-client streaming profiles.

<div class="section_buttons">

| Previous                                        |              Next |
|:------------------------------------------------|------------------:|
| [Third-party Packages](third_party_packages.md) | [Legal](legal.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
