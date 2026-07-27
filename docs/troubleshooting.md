# Troubleshooting

## General

### Forgotten credentials

Reset the Web UI username and password with the command for your package:

@tabs{
  @tab{General | ```bash
    sunshine --creds {new-username} {new-password}
    ```
  }
  @tab{AppImage | ```bash
    ./sunshine.AppImage --creds {new-username} {new-password}
    ```
  }
  @tab{Flatpak | ```bash
    flatpak run --command=sunshine dev.lizardbyte.app.Sunshine --creds {new-username} {new-password}
    ```
  }
}

> [!TIP]
> Replace `{new-username}` and `{new-password}` with the new credentials. Do not include the curly braces.

### Unusual mouse behavior

Attach a physical mouse to the SolarFlare host. Some systems do not report a usable pointer unless a mouse is connected.

### Web UI access

Check that the host firewall allows the Web UI port.

### Controller works in Steam but not in games

In Steam's controller settings, disable Xbox and PlayStation controller support
and leave Generic Gamepad support enabled.

Games may also select a physical controller before SolarFlare's virtual
controller. Disconnect or disable unused host controllers and try again. On
Linux, find the USB device under `/sys/bus/usb/devices/` and write `0` to its
`authorized` file.

### Network performance test

Game streaming needs a stable path between the host and client. Low latency,
low jitter, and little packet loss matter more than unused bandwidth.

Test the path with [iPerf3](https://iperf.fr).

Start `iperf3` in server mode on the SolarFlare host:

```bash
iperf3 -s
```

Run a 60-second reverse UDP test from the client at the bitrate you plan to stream, such as 50 Mbps:

```bash
iperf3 -c {HostIpAddress} -t 60 -u -R -b 50M
```

Check the client output for packet loss and jitter. Aim for less than 5% packet loss and less than 1 ms of jitter.

Android clients can use
[PingMaster](https://play.google.com/store/apps/details?id=com.appplanex.pingmasternetworktools).
iOS clients can use
[HE.NET Network Tools](https://apps.apple.com/us/app/he-net-network-tools/id858241710).

Testing across the internet requires forwarding TCP and UDP port 5201 to the host.

### Packet loss from buffer overruns

Packet loss can occur when the host link is faster than the slowest link to the
client. At 60 FPS, SolarFlare sends a burst about every 16 ms. A slower
downstream link must buffer each burst, and a full buffer drops packets.

Common examples include a 2.5 Gbit/s host feeding a 1 Gbit/s or Wi-Fi client,
and a 1 Gbit/s host feeding a 100 Mbit/s client.

You can reduce the host NIC speed to match the slower link. On Linux, traffic
shaping can limit only the SolarFlare stream:

```bash
# 1) Remove existing qdisc (pfifo_fast)
sudo tc qdisc del dev <NIC> root

# 2) Add HTB root qdisc with default class 1:1
sudo tc qdisc add dev <NIC> root handle 1: htb default 1

# 3) Create class 1:1 for full 10 Gbit/s (all other traffic)
sudo tc class add dev <NIC> parent 1: classid 1:1 htb \
    rate 10000mbit ceil 10000mbit burst 32k

# 4) Create class 1:10 for the SolarFlare stream at 1 Gbit/s
sudo tc class add dev <NIC> parent 1: classid 1:10 htb \
    rate 1000mbit ceil 1000mbit burst 32k

# 5) Filter UDP source port 47998 into class 1:10
sudo tc filter add dev <NIC> protocol ip parent 1: prio 1 \
    u32 match ip protocol 17 0xff \
    match ip sport 47998 0xffff flowid 1:10
```

These rules limit UDP source port 47998 to 1 Gbit/s and do not persist after a
reboot. Change the last command if your stream uses a different port.

SolarFlare includes the networking improvements added after Sunshine 0.23.1,
which may prevent this problem without NIC speed limits.

### Packet loss from MTU

A lower [MTU](https://en.wikipedia.org/wiki/Maximum_transmission_unit) may help
some clients. One LG TV showed 30-60% packet loss with host MTU values of 1500
and 1472, and no packet loss at 1428. The cause was not confirmed, so try this
only after ruling out other network problems.

## Linux

### Hardware encoding fails

Some Mesa packages disable hardware decoding and encoding because of patent restrictions.

```txt
Error: Could not open codec [h264_vaapi]: Function not implemented
```

If this error appears in the SolarFlare logs, you may need to compile *Mesa*.
Follow the official Mesa3D
[Compiling and Installing](https://docs.mesa3d.org/install.html) instructions.

> [!IMPORTANT]
> Pass the following option to the build system to enable the encoders.
> SolarFlare does not require the matching decoders.
> ```bash
> -Dvideo-codecs=h264enc,h265enc
> ```

> [!NOTE]
> Other build options are listed in the
> [meson options](https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/meson_options.txt) file.

### Input not working

The installer reloads the `udev` rules. Restart the host if the reload failed.

If input still does not work, add your user to the `input` group:

```bash
sudo usermod -aG input $USER
```

#### Multiseat

A compositor may ignore injected input when concurrent Wayland sessions run on
separate logind seats, such as `seat0` and `seat1`, unless SolarFlare's virtual
devices use the correct seat.

SolarFlare reads the target seat from `XDG_SEAT`, which the display manager
usually sets. You can override it in the systemd service or shell environment
before starting SolarFlare.

For seats other than `seat0`, SolarFlare appends the seat name to each virtual device name:

- Keyboard passthrough (seat1)
- Sunshine PS5 (virtual) pad (seat1)

SolarFlare creates one relative mouse device and one absolute mouse device.

Create `/etc/udev/rules.d/72-sunshine-virtual-seat.rules` to assign the virtual devices to the correct seat:
```udev
SUBSYSTEM=="input", KERNEL=="input*", ATTR{name}=="*(seat1)*", TAG+="seat", ENV{ID_SEAT}="seat1"
```

Then reload udev:

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger -s input
```

### KMS streaming fails

KMS capture needs privileges unavailable to Flatpak and AppImage packages.
Install SolarFlare with your distribution's native package format, if
available. XDG Portal Capture works with all package types and is replacing
KMS capture.

### Windows flicker or disappear with KMS on KDE Plasma 6.5+

KWin overlays can interfere with KMS capture. Disable them with the
[`KWIN_USE_OVERLAYS`](https://invent.kde.org/plasma/kwin/-/wikis/Environment-Variables#kwin_use_overlays)
environment variable:

```bash
export KWIN_USE_OVERLAYS=0
```

> [!NOTE]
> Disabling overlays will reduce KWin's rendering efficiency. Consider using XDG Portal Capture instead.

### KMS streaming fails on Nvidia GPUs

If KMS capture produces a black stream, enable modesetting for the Nvidia
kernel module by placing this directive on the kernel command line:

```bash
nvidia_drm.modeset=1
```

Follow your distribution's instructions for changing the kernel command line.
GRUB is the most common bootloader for this setup.

### AMD encoding latency issues

High or unstable encoding latency in Moonlight's performance overlay can
indicate Mesa older than 24.2, especially at 4K.

Mesa 24.2 added a
[low-latency encoding mode](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/30039),
enabled through the
[`AMD_DEBUG`](https://docs.mesa3d.org/envvars.html#envvar-AMD_DEBUG) environment
variable:
```bash
export AMD_DEBUG=lowlatencyenc
```
SolarFlare sets this variable automatically.

Use `amdgpu_top` to check VCLK and DCLK. Both clocks should remain high while
low-latency encoding is active; without it, they may fluctuate.

### Gamescope compatibility

Games running inside Gamescope may stutter while streaming.

## macOS

### Dynamic session lookup failed

This error means SolarFlare could not find the D-Bus session socket:

> Dynamic session lookup supported but failed: launchd did not provide a socket path, verify that
> org.freedesktop.dbus-session.plist is loaded!

Load the launch agent:

```bash
launchctl load -w /Library/LaunchAgents/org.freedesktop.dbus-session.plist
```

## Windows

### No gamepad detected

Virtual gamepads require ViGEmBus 1.17 or newer. Install it from the Web UI's
Troubleshooting page or download it from
[ViGEmBus releases](https://github.com/nefarius/ViGEmBus/releases/latest), then
restart Windows.

### Permission denied

SolarFlare runs as the `SYSTEM` account on Windows. A game or application on
another drive may fail to launch if that account cannot read its files.

Grant the `SYSTEM` principal read and execute access to the application's files
and directories. Grant write access only where the application needs it.

### Stuttering

For NVIDIA GPUs, disable `vsync:fast` in the NVIDIA Control Panel.

<div class="section_buttons">

| Previous      |                    Next |
|:--------------|------------------------:|
| [API](api.md) | [Building](building.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
