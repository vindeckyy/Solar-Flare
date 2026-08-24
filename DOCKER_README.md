# Docker

> [!IMPORTANT]
> **SolarFlare does not publish a container image.** This document describes
> inherited upstream Sunshine container images and experimental patterns for
> building your own. Upstream images **do not** include SolarFlare fork tuning,
> the SolarFlare Web UI, network pacing, audio FX, API tokens, webhooks, or
> other fork-specific features.
>
> For the supported SolarFlare experience on Linux, build from source with
> [`./scripts/linux-install.sh`](scripts/linux-install.sh). See the
> [README install section](README.md#install).

> [!CAUTION]
> Containerized game streaming is **experimental** for Sunshine and unsupported
> as a primary SolarFlare deployment path. GPU passthrough, input injection,
> KMS capture, and `cap_sys_admin`/`cap_sys_nice` requirements conflict with
> typical container security models.

---

## When containers make sense (and when they do not)

| Scenario | Recommendation |
|---|---|
| Quick upstream Sunshine smoke test | Upstream image OK with caveats below |
| Production SolarFlare host on Linux | **Source install** - not Docker |
| Homelab media server with GPU passthrough | Custom Dockerfile possible; expect manual tuning |
| Kubernetes / orchestrated gaming | See [Games on Whales](https://games-on-whales.github.io); not SolarFlare-maintained |
| CI compile-only builds | Use `scripts/linux_build.sh` Docker builder, not runtime image |

---

## Image tags

Container tags combine a **version channel** and **operating-system suffix**.
Bare tags such as `latest`, `master`, or `vX.X.X` are **not** complete image
tags. Always use the form `<SUNSHINE_VERSION>-<SUNSHINE_OS>`.

**Examples:**

- `latest-ubuntu-24.04`
- `v2025.924.154138-ubuntu-22.04`
- `<commit-sha>-debian-bookworm`

Browse tags:

- [Docker Hub - lizardbyte/sunshine](https://hub.docker.com/r/lizardbyte/sunshine/tags)
- [GitHub Container Registry](https://github.com/LizardByte/Sunshine/pkgs/container/sunshine/versions)

---

## Build your own containers

The upstream image is intended as a **base layer** for other Docker projects,
not as a polished standalone streaming appliance. Treat all examples as
starting points.

### Minimal Dockerfile (upstream Sunshine)

```dockerfile
ARG SUNSHINE_VERSION=latest
ARG SUNSHINE_OS=ubuntu-24.04
FROM lizardbyte/sunshine:${SUNSHINE_VERSION}-${SUNSHINE_OS}

# Install games, Steam, Wayland compositor, etc.
# RUN apt-get update && apt-get install -y ...

# NOT recommended for production - illustrative only
ENTRYPOINT ["sunshine"]
```

### Building a SolarFlare-like image (advanced)

There is no official Dockerfile in this repository for runtime containers.
To approximate SolarFlare inside a container you would need to:

1. Multi-stage build: compile SolarFlare from this repository (see [Building](docs/building.md))
2. Copy `sunshine` binary, Web UI assets, and shaders into a runtime stage
3. Grant capabilities at runtime (`--cap-add=SYS_ADMIN --cap-add=SYS_NICE`) - weakens isolation
4. Pass through `/dev/dri`, input devices, and often `--ipc=host`
5. Mount `~/.config/sunshine` for state and pairings

Even then, KMS capture, `cpu_pinning` (`SCHED_RR`), and GPU governor hooks may
fail silently inside namespaces. Expect to use portal/X11 capture instead of KMS.

---

## Build arguments

### `SUNSHINE_VERSION`

| Value | Meaning |
|---|---|
| `latest` | Latest stable release channel |
| `master` | Rolling development builds (unstable) |
| `vX.X.X` | Specific upstream tag |
| `<commit>` | Pin to exact commit hash |

### `SUNSHINE_OS`

Base image suffix:

| Suffix | Base | amd64 | arm64 |
|---|---|---|---|
| `debian-bookworm` | Debian 12 | ✅ | ✅ |
| `ubuntu-22.04` | Ubuntu 22.04 LTS | ✅ | ✅ |
| `ubuntu-24.04` | Ubuntu 24.04 LTS | ✅ | ✅ |

Combined tag format: `<SUNSHINE_VERSION>-<SUNSHINE_OS>` → `latest-ubuntu-24.04`.

---

## Port and volume mappings

SolarFlare/ Sunshine require fixed ports for Moonlight compatibility.

| Port | Protocol | Required | Purpose |
|---|---|---|---|
| 47984–47990 | TCP | Yes | Control, RTSP, HTTPS Web UI on 47990 |
| 48010 | TCP | Yes | Additional control |
| 47998–48000 | UDP | Yes | Video/audio stream |

The **internal** container port for the Web UI must remain **47990**. The
**external** host port may be mapped differently (e.g. `-p 8080:47990`), but
Moonlight clients expect standard ports unless configured otherwise.

### Volume: configuration state

Persist pairing certificates, `sunshine.conf`, and `apps.json`:

```bash
-v /path/on/host/sunshine-config:/config
```

Inside the container, configuration typically maps to `/config`. On a native
SolarFlare install the same files live in `~/.config/sunshine/`.

---

## Using `docker run`

Substitute `<values>` with your environment.

```bash
docker run -d \
  --name sunshine \
  --restart=unless-stopped \
  --ipc=host \
  --device /dev/dri/ \
  --device /dev/uinput \
  --cap-add=SYS_ADMIN \
  --cap-add=SYS_NICE \
  -e PUID=1000 \
  -e PGID=1000 \
  -e TZ=America/New_York \
  -v /home/user/sunshine-config:/config \
  -p 47984-47990:47984-47990/tcp \
  -p 48010:48010/tcp \
  -p 47998-48000:47998-48000/udp \
  lizardbyte/sunshine:latest-ubuntu-24.04
```

> [!WARNING]
> `--device /dev/dri/` alone does not guarantee working hardware encode inside
> the container. Vendor drivers, render nodes, and VA-API/NVENC visibility must
> be validated in logs.

### Parameter reference

| Flag | Function | Example | Required |
|---|---|---|---|
| `-p <ext>:47990/tcp` | Web UI HTTPS | `47990:47990` | Yes (internal 47990) |
| `-p 47984-47990:47984-47990/tcp` | Full TCP range | as shown | Yes |
| `-p 47998-48000:47998-48000/udp` | Stream UDP | as shown | Yes |
| `-v <host>:/config` | Config + certs | `/home/user/sf-config` | Strongly recommended |
| `-e PUID` / `-e PGID` | File ownership | `1000` | Recommended |
| `-e TZ` | Timezone | `Europe/Berlin` | Optional |
| `--ipc=host` | Shared IPC for some GPU paths | - | Often required |
| `--device /dev/dri/` | GPU render nodes | - | Required for HW encode |

For additional compose patterns, see
[Games on Whales sunshine.yml](https://github.com/games-on-whales/gow/blob/2e442292d79b9d996f886b8a03d22b6eb6bddf7b/compose/streamers/sunshine.yml).

---

## Using `docker-compose`

```yaml
services:
  sunshine:
    image: lizardbyte/sunshine:latest-ubuntu-24.04
    container_name: sunshine
    restart: unless-stopped
    ipc: host
    devices:
      - /dev/dri/
      - /dev/uinput
    cap_add:
      - SYS_ADMIN
      - SYS_NICE
    environment:
      PUID: 1000
      PGID: 1000
      TZ: America/New_York
    volumes:
      - ./sunshine-config:/config
    ports:
      - "47984-47990:47984-47990/tcp"
      - "48010:48010/tcp"
      - "47998-48000:47998-48000/udp"
```

Start: `docker compose up -d`

---

## Using `podman run`

Podman equivalent with rootless-friendly user namespace:

```bash
podman run -d \
  --name sunshine \
  --restart=unless-stopped \
  --userns=keep-id \
  --device /dev/dri/ \
  -e PUID=1000 \
  -e PGID=1000 \
  -e TZ=America/New_York \
  -v /home/user/sunshine-config:/config \
  -p 47984-47990:47984-47990/tcp \
  -p 48010:48010/tcp \
  -p 47998-48000:47998-48000/udp \
  lizardbyte/sunshine:latest-ubuntu-24.04
```

Rootless Podman may not expose all devices or capabilities required for KMS
capture. Expect software encoding or portal capture only.

---

## User / group identifiers (PUID/PGID)

Volume mounts can cause permission mismatches between host files and container
process user. Set `PUID`/`PGID` to match the owner of the host config directory:

```bash
id sunshine-user
# uid=1001(sunshine-user) gid=1001(sunshine-group)
```

Ensure `/path/on/host/sunshine-config` is owned by that uid/gid before first run.

Changing PUID/PGID after data is created may require `chown -R` on the host volume.

---

## Supported architectures

Tags `latest-<SUNSHINE_OS>` on Docker Hub and `ghcr.io/lizardbyte/sunshine`
provide multi-arch manifests. The build selects amd64 or arm64 automatically.

| OS suffix | amd64/x86_64 | arm64/aarch64 |
|---|---|---|
| debian-bookworm | ✅ | ✅ |
| ubuntu-22.04 | ✅ | ✅ |
| ubuntu-24.04 | ✅ | ✅ |

SolarFlare release binaries on GitHub are **Linux x86-64 only**; arm64 users
must build from source regardless of container base arch.

---

## Common failure modes

| Symptom | Likely cause | Mitigation |
|---|---|---|
| Black screen in Moonlight | No GPU in container | Pass `/dev/dri`, correct driver on host |
| Web UI unreachable | Wrong port map | Map host port to container **47990** |
| Pairing lost on recreate | Ephemeral `/config` | Persist volume mount |
| Encoder `Function not implemented` | VA-API/NVENC not visible | Use host driver-matched image; check `vainfo`/`nvidia-smi` in container |
| Input not working | Missing `uinput` | `--device /dev/uinput`, correct groups |
| High latency vs native | No fork tunables in upstream image | Build SolarFlare from source on host |

---

## Where Sunshine containers are used

Community projects (not SolarFlare-maintained):

- [Games on Whales](https://games-on-whales.github.io) - orchestrated cloud gaming stacks

Missing your project? Upstream Sunshine docs welcome community links; SolarFlare
does not maintain this list.

---

## See also

- [README - Install](README.md#install) - supported SolarFlare path
- [Getting started](docs/getting_started.md) - inherited platform reference
- [Building](docs/building.md) - compile SolarFlare for native install
- [Troubleshooting](docs/troubleshooting.md) - native host diagnostics
- [Third-party packages](docs/third_party_packages.md) - distro packages vs containers

<div class="section_buttons">

| Previous                       |                                                 Next |
|:-------------------------------|-----------------------------------------------------:|
| [Changelog](docs/changelog.md) | [Third-Party Packages](docs/third_party_packages.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
