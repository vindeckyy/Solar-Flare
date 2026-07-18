# GameStream Migration
Nvidia announced that their GameStream service for Nvidia Games clients will be discontinued in February 2023.
SolarFlare provides a Moonlight-compatible replacement for Nvidia GameStream.

## Migration
The upstream GSMS tool can migrate GameStream games and apps into SolarFlare's
compatible application format.
Please check out our [GSMS](https://github.com/LizardByte/GSMS) project if you're interested in an automated
migration option. GSMS offers the ability to migrate your custom and auto-detected games and apps. The
working directory, command, and image are all set in SolarFlare's compatible `apps.json` file. The box-art image is also copied
to a specified directory.

## Internet Streaming
If you are using the Moonlight Internet Hosting Tool, you can remove it from your system when you migrate to SolarFlare.
To stream over the Internet with SolarFlare and a UPnP-capable router, enable UPnP in the SolarFlare Web UI.

> [!NOTE]
> Running SolarFlare together with versions of the Moonlight Internet Hosting Tool prior to v5.6 will cause UPnP
> port forwarding to become unreliable. Either uninstall the tool entirely or update it to v5.6 or later.

## Limitations
SolarFlare has some limitations compared with Nvidia GameStream.

* Automatic game/application list.
* Changing game settings automatically to optimize streaming.

<div class="section_buttons">

| Previous                                        |              Next |
|:------------------------------------------------|------------------:|
| [Third-party Packages](third_party_packages.md) | [Legal](legal.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
