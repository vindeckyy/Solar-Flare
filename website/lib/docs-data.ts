export interface DocCallout {
  type: 'note' | 'tip' | 'important' | 'warning' | 'caution'
  text: string
}

export interface DocCodeBlock {
  language: string
  code: string
}

export interface DocParam {
  name: string
  type: string
  defaultVal: string
  range?: string
  description: string
  example?: string
  note?: string
}

export interface DocEndpoint {
  method: 'GET' | 'POST' | 'DELETE' | 'PUT'
  path: string
  auth: string
  scopes?: string[]
  description: string
  requestBody?: string
  responseBody?: string
  notes?: string
}

export interface DocTable {
  headers: string[]
  rows: string[][]
}

/**
 * @brief A screenshot or figure embedded in a section.
 * @remarks `src` is resolved relative to the website `public/` directory;
 * the site base path is prepended automatically at render time.
 */
export interface DocImage {
  src: string
  alt: string
  caption?: string
}

/**
 * @brief One panel inside a DocsTabs group.
 */
export interface DocTab {
  id: string
  label: string
  content?: string
  code?: DocCodeBlock
  codeTabs?: { label: string; language: string; code: string }[]
  callout?: DocCallout
  params?: DocParam[]
  endpoints?: DocEndpoint[]
  table?: DocTable
  image?: DocImage
}

export interface DocSection {
  id: string
  title: string
  content?: string
  code?: DocCodeBlock
  /** Mutually exclusive labeled code samples (for example distro or shell). */
  codeTabs?: { label: string; language: string; code: string }[]
  callout?: DocCallout
  params?: DocParam[]
  endpoints?: DocEndpoint[]
  table?: DocTable
  /** Named content panels rendered as a tab strip. */
  tabs?: DocTab[]
  image?: DocImage
}

export interface DocArticle {
  slug: string
  title: string
  category: string
  badge?: string
  description: string
  readTime: string
  lastUpdated: string
  sections: DocSection[]
}

export interface DocCategory {
  name: string
  description: string
  iconName: string
  items: {
    slug: string
    title: string
    badge?: string
    description: string
  }[]
}

export const DOC_CATEGORIES: DocCategory[] = [
  {
    name: 'Getting Started',
    description: 'Installation, quickstart guides, setup, and client pairing.',
    iconName: 'Rocket',
    items: [
      {
        slug: 'getting-started',
        title: 'Installation & Quickstart',
        badge: 'Guide',
        description: 'Complete guide to installing SolarFlare on Linux, configuring firewalls, and pairing Moonlight.',
      },
      {
        slug: 'gamestream-migration',
        title: 'GameStream Migration',
        description: 'Migrate existing NVIDIA GameStream setups and library configurations to SolarFlare.',
      },
      {
        slug: 'guides',
        title: 'Operator Guides',
        badge: 'How-to',
        description: 'Curated LAN baseline, low-latency, headless, multi-GPU, HDR, per-client, webhook, and logging how-tos.',
      },
    ],
  },
  {
    name: 'Operations',
    description: 'Containers, packaging caveats, and host operations that sit beside day-to-day streaming.',
    iconName: 'Boxes',
    items: [
      {
        slug: 'docker',
        title: 'Docker and containers',
        badge: 'Ops',
        description: 'Upstream Sunshine images, GPU passthrough caveats, and why SolarFlare prefers a native install.',
      },
    ],
  },
  {
    name: 'Configuration',
    description: 'Host settings, Audio FX DSP filters, Opus tuning, and client profiles.',
    iconName: 'Sliders',
    items: [
      {
        slug: 'configuration',
        title: 'Configuration Reference',
        badge: 'Core',
        description: 'Fork-specific network, scheduling, capture, and watchdog tunables plus Audio FX, Opus DSP, webhooks, API tokens, and per-client profiles.',
      },
      {
        slug: 'app-examples',
        title: 'App & Game Examples',
        description: 'Launch configs for Steam, Epic, Lutris, elevated commands, and encoder presets.',
      },
    ],
  },
  {
    name: 'Optimization',
    description: 'Latency reduction, real-time scheduling, and diagnostic workflows.',
    iconName: 'Zap',
    items: [
      {
        slug: 'performance-tuning',
        title: 'Performance & Latency Tuning',
        badge: 'Tuning',
        description: 'CPU pinning, GPU governors, PipeWire audio latency hints, and socket buffer tuning.',
      },
      {
        slug: 'troubleshooting',
        title: 'Troubleshooting & Diagnostics',
        description: 'Diagnosing KMS/Wayland capture, audio sink setup, hardware encoding, and logs.',
      },
    ],
  },
  {
    name: 'Developer & API',
    description: 'REST API endpoints, multi-distro builds, and compilation.',
    iconName: 'Code2',
    items: [
      {
        slug: 'api',
        title: 'REST API Reference',
        badge: 'API',
        description: 'Full REST API reference for automation, scoped tokens, updater, and telemetry.',
      },
      {
        slug: 'porting',
        title: 'Multi-Distro Porting',
        description: 'Package translation matrices for Arch, Debian, Ubuntu, Fedora, and openSUSE.',
      },
      {
        slug: 'building',
        title: 'Building from Source',
        description: 'Compiling with CMake, developer build profiles, testing with GoogleTest.',
      },
      {
        slug: 'third-party-packages',
        title: 'Dependencies & Third-Party',
        description: 'Bundled submodules, FFmpeg prebuilt pins, Flatpak versions, and community-package warnings.',
      },
    ],
  },
  {
    name: 'Project & Release',
    description: 'Release history, security advisories, and code standards.',
    iconName: 'Shield',
    items: [
      {
        slug: 'changelog',
        title: 'Changelog & Releases',
        badge: 'v1.3.0',
        description: 'Chronological history of SolarFlare releases, cherry-picks, and enhancements.',
      },
      {
        slug: 'security',
        title: 'Security Policy',
        description: 'Vulnerability disclosure policies, supported release branches, and advisory channels.',
      },
      {
        slug: 'contributing',
        title: 'Contributing Guide',
        description: 'Code style guidelines, Doxygen documentation requirements, and testing rules.',
      },
      {
        slug: 'release-process',
        title: 'Maintainer Release Guide',
        description: 'Standard operating procedure for creating tags, verifying binaries, and publishing.',
      },
      {
        slug: 'legal',
        title: 'Legal & Licensing',
        description: 'GPL-3.0 operator summary, trademarks, codec patents, redistribution checklist, and privacy.',
      },
      {
        slug: 'ecosystem',
        title: 'Ecosystem & Upstream',
        description: 'Awesome-Sunshine catalog, upstream changelog feed, and SolarFlare vs upstream history.',
      },
      {
        slug: 'maintainers',
        title: 'Maintainer Handbook',
        description: 'Triage duties, dual versioning, artifact contracts, CI scope, and new-maintainer handoff.',
      },
    ],
  },
]

export const DOC_ARTICLES: Record<string, DocArticle> = {
  'getting-started': {
    slug: 'getting-started',
    title: 'Installation & Quickstart',
    category: 'Getting Started',
    badge: 'v1.3.0',
    description: 'Install SolarFlare on your Linux host, configure permissions, and pair Moonlight streaming clients.',
    readTime: '6 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'overview',
        title: 'Overview',
        content:
          'SolarFlare is a low-latency self-hosted game-streaming server for Moonlight clients. It runs on Linux hosts with native KMS/Wayland capture, hardware-accelerated NVENC, VA-API, and Vulkan encoding, and fine-grained host tunables.',
        image: {
          src: '/sf-web-ui-featured.png',
          alt: 'SolarFlare Web UI featured applications view',
          caption: 'The SolarFlare Web UI landing view after sign-in.',
        },
      },
      {
        id: 'automated-install',
        title: 'Automated Linux Installation',
        content:
          'The maintained installation path on Linux is the installer script. It detects the distribution, installs packages, compiles SolarFlare, and sets up the systemd user service. Use the distro tabs for extra notes.',
        code: {
          language: 'bash',
          code: `git clone --recursive https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/linux-install.sh
systemctl --user enable --now app-dev.lizardbyte.app.Sunshine.service`,
        },
        tabs: [
          {
            id: 'arch',
            label: 'Arch / CachyOS',
            content:
              'Uses pacman. GCC 14+ is typical. CachyOS can enable native CPU flags through the installer path. After install, confirm setcap on the sunshine binary.',
            code: {
              language: 'bash',
              code: 'pacman -Q gcc cmake ninja pipewire libdrm',
            },
          },
          {
            id: 'debian',
            label: 'Debian / Ubuntu',
            content:
              'Uses apt. Ubuntu 22.04 may need GCC 13 from the toolchain PPA. Installer pulls libpipewire and libva development packages.',
            callout: {
              type: 'note',
              text: 'If cmake is older than 3.20, install a newer CMake before running the installer.',
            },
          },
          {
            id: 'fedora',
            label: 'Fedora / Bazzite',
            content:
              'Uses dnf. Bazzite layers packages with rpm-ostree and may require a reboot, then re-run ./scripts/linux-install.sh --skip-deps.',
          },
          {
            id: 'suse',
            label: 'openSUSE',
            content: 'Uses zypper. Pull ffmpeg-devel and pipewire-devel. Tumbleweed tracks current GCC.',
          },
          {
            id: 'nixos',
            label: 'NixOS',
            content:
              'Do not apt/dnf install. Enter the repo Nix shell and apply the declarative host block in the porting guide before starting the user service.',
            callout: {
              type: 'important',
              text: 'See the Porting article for the NixOS module snippet. Installs land in ~/.local.',
            },
          },
        ],
        callout: {
          type: 'tip',
          text: 'New hosts should always build with ./scripts/linux-install.sh. GitHub binaries are for updating an already working install.',
        },
      },
      {
        id: 'linux-permissions',
        title: 'KMS & Real-Time Permissions',
        content:
          'For low-latency KMS capture and real-time capture thread scheduling (SCHED_RR), grant the necessary capabilities to the binary if installed manually:',
        code: {
          language: 'bash',
          code: `sudo setcap 'cap_sys_admin,cap_sys_nice+p' /usr/local/bin/sunshine`,
        },
      },
      {
        id: 'first-run',
        title: 'First Launch & Web UI Setup',
        content:
          'Start SolarFlare as a systemd user service or run it directly from your terminal. Open the Web UI configuration portal at https://localhost:47990 in your browser.',
        code: {
          language: 'bash',
          code: `# Start and enable as a systemd user service
systemctl --user enable --now app-dev.lizardbyte.app.Sunshine.service

# View real-time logs
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -f`,
        },
        callout: {
          type: 'important',
          text: 'On first launch, the Web UI prompts you to create an administrator username and password. Keep these credentials safe!',
        },
        image: {
          src: '/sf-web-ui-home.png',
          alt: 'SolarFlare Web UI home page',
          caption: 'Web UI home after the first-run credential setup.',
        },
      },
      {
        id: 'pairing',
        title: 'Pairing Moonlight Clients',
        content:
          '1. Open the Moonlight client on your client device (PC, phone, tablet, Apple TV, Steam Deck).\n2. Select your host computer from the list or enter your host LAN IP address.\n3. Moonlight will display a 4-digit PIN.\n4. Open the SolarFlare Web UI at https://localhost:47990/pin, enter the PIN, and click Pair.',
        image: {
          src: '/sf-web-ui-pin.png',
          alt: 'SolarFlare Web UI PIN pairing form',
          caption: 'Enter the 4-digit PIN shown by Moonlight to pair a client.',
        },
      },
      {
        id: 'firewall',
        title: 'Firewall & Network Ports',
        content:
          'Open the GameStream-compatible ports on the host firewall. The Web UI listens on TCP 47990. Video is typically UDP 47998.',
        table: {
          headers: ['Port / Range', 'Protocol', 'Purpose'],
          rows: [
            ['47984-47990', 'TCP', 'Control, RTSP, HTTPS Web UI (47990)'],
            ['48010', 'TCP', 'Additional control'],
            ['47998-48000', 'UDP', 'Video and audio stream'],
          ],
        },
        codeTabs: [
          {
            label: 'ufw',
            language: 'bash',
            code: `sudo ufw allow 47984:47990/tcp
sudo ufw allow 48010/tcp
sudo ufw allow 47998:48000/udp`,
          },
          {
            label: 'firewalld',
            language: 'bash',
            code: `sudo firewall-cmd --permanent --add-port=47984-47990/tcp
sudo firewall-cmd --permanent --add-port=48010/tcp
sudo firewall-cmd --permanent --add-port=47998-48000/udp
sudo firewall-cmd --reload`,
          },
          {
            label: 'nftables',
            language: 'bash',
            code: `sudo nft add rule inet filter input tcp dport 47984-47990 accept
sudo nft add rule inet filter input tcp dport 48010 accept
sudo nft add rule inet filter input udp dport 47998-48000 accept`,
          },
        ],
      },

      {
        id: 'installer-flags',
        title: 'Installer flags & what it does',
        content:
          'No flag = full install (deps, submodules, cmake configure, build, install, post-install services). `--clean` removes `cmake-build-cachyos` and reconfigures. `--skip-deps` (alias `--no-pacman`) skips the package manager and rebuilds only. `--print-distro-id` prints the canonical `/etc/os-release` ID and exits. `-h` prints help. The installer retries submodule clones, picks `ccache` and `mold > lld > ld` when available, configures Release with `BUILD_DOCS=OFF BUILD_TESTS=OFF SUNSHINE_ENABLE_TRAY=OFF SUNSHINE_ENABLE_CUDA=OFF`, builds with capped parallelism, then `sudo cmake --install` plus `systemctl --user daemon-reload`. `npm install --no-audit --no-fund` runs when `node_modules/` is missing.',
      },
      {
        id: 'distro-deps',
        title: 'Per-distribution dependencies',
        content: 'The installer detects the distro and installs the exact package set. Key notes per family:',
        tabs: [
          {
            id: 'arch-deps',
            label: 'Arch / CachyOS',
            content: 'GCC 14+ typical. Confirm capabilities after install.',
            code: {
              language: 'bash',
              code: 'sudo pacman -S --needed --noconfirm base-devel cmake ninja git openssl curl libpulse libdrm libva libx11 libxfixes libxrandr libxcb libxkbcommon libevdev opus libpipewire libportal wayland wayland-protocols systemd-libs libcap libnatpmp vulkan-headers shaderc glslang boost miniupnpc nlohmann-json libpng libxext libxtst nodejs npm',
            },
          },
          {
            id: 'debian-deps',
            label: 'Debian / Ubuntu',
            content: 'Ubuntu 22.04 needs GCC 13 from the toolchain PPA. CMake must be 3.20+.',
            code: {
              language: 'bash',
              code: 'sudo apt-get update && sudo apt-get install -y --no-install-recommends build-essential cmake ninja-build git pkg-config libssl-dev libcurl4-openssl-dev libpulse-dev libdrm-dev libva-dev libx11-dev libxfixes-dev libxrandr-dev libxcb1-dev libxkbcommon-dev libevdev-dev libopus-dev ffmpeg libpipewire-0.3-dev libportal-dev libwayland-dev wayland-protocols libudev-dev libcap-dev libnatpmp-dev vulkan-tools glslang-tools spirv-tools libboost-all-dev libminiupnpc-dev nlohmann-json3-dev libpng-dev libxext-dev libxtst-dev nodejs npm',
            },
          },
          {
            id: 'fedora-deps',
            label: 'Fedora / Nobara',
            content: 'RPM Fusion may be required for ffmpeg-devel. Vulkan encode needs vulkan-devel + shaderc.',
            code: {
              language: 'bash',
              code: 'sudo dnf install -y gcc-c++ cmake ninja-build git pkgconfig openssl-devel libcurl-devel pulseaudio-libs-devel libdrm-devel libva-devel libX11-devel libXfixes-devel libXrandr-devel libxcb-devel libxkbcommon-devel libevdev-devel opus-devel pipewire-devel libportal-devel wayland-devel wayland-protocols-devel systemd-devel libcap-devel vulkan-devel glslang-devel miniupnpc-devel nodejs npm',
            },
          },
          {
            id: 'bazzite-note',
            label: 'Bazzite',
            content: 'rpm-ostree layering is a hard stop: install, reboot, then re-run the installer with --skip-deps.',
            code: {
              language: 'bash',
              code: `sudo rpm-ostree install --apply-live --allow-inactive gcc-c++ cmake ninja-build git nodejs npm
sudo systemctl reboot
./scripts/linux-install.sh --skip-deps`,
            },
          },
        ],
      },
      {
        id: 'verify',
        title: 'Verify the install',
        content: 'Run these five checks. Every one must pass before pairing:',
        code: {
          language: 'bash',
          code: `systemctl --user --no-pager status app-dev.lizardbyte.app.Sunshine.service
getcap /usr/local/bin/sunshine
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -n 50 --no-pager
curl --insecure --output /dev/null --write-out \'%{http_code}\\n\' https://localhost:47990/
sunshine --version 2>&1 | grep -m1 \'Fork: SolarFlare\'`,
        },
        table: {
          headers: ['Check', 'Expected'],
          rows: [
            ['Service status', 'active (running)'],
            ['getcap', 'cap_sys_admin,cap_sys_nice=p'],
            ['curl before login', 'HTTP 401 (serving, auth required)'],
            ['--version', 'Line containing Fork: SolarFlare'],
          ],
        },
      },
      {
        id: 'update-uninstall',
        title: 'Update & uninstall',
        content:
          'Preferred update: Web UI **Update now** on the outdated banner (open the chevron for the command log). It downloads `solarflare-linux-x86_64.tar.gz`, verifies `SHA256SUMS`, installs binary + assets, and restarts; an active stream blocks apply until idle. Manual binary-only fallback stops the service, curls `sunshine-x86_64` over `/usr/local/bin/sunshine`, chmods 0755, re-applies `setcap`, and restarts. Raw GitHub executables cannot retain Linux capabilities — always re-run setcap after a manual download. Uninstall: disable the user service, remove the binary (`/usr/local/bin/sunshine` or `~/.local/bin/sunshine` on NixOS) and the update helper + polkit file (back up `~/.config/sunshine/` first), then run the redesign uninstaller and Hermes-KMS uninstaller if those were installed. Note `--clean` only nukes `cmake-build-cachyos`, not installed files.',
      },
      {
        id: 'pairing-deep',
        title: 'Pairing details & auto-pairing',
        content:
          'If the host never appears, add it by LAN IP: mDNS needs one broadcast domain with no AP isolation. Moonlight shows a 4-digit PIN; enter it with a device name at https://<host>:47990/pin (same as POST /api/pin with pin 0000-9999 plus name). Keep host and client clocks within ~30 s or pairing fails. trusted_subnet_auto_pairing with trusted_subnets (for example 192.168.1.0/24) skips the PIN for matching CIDRs — only on fully-controlled networks, since broad ranges silently pair strangers and bad CIDRs can lock out your LAN. Unpair from the clients list or via POST /api/clients/unpair and /api/clients/unpair-all; pairing certificates live under ~/.config/sunshine/.',
      },
    ],
  },

  'configuration': {
    slug: 'configuration',
    title: 'Configuration Reference',
    category: 'Configuration',
    badge: 'Core',
    description: 'Detailed specification of all SolarFlare host settings, Audio FX DSP filters, Opus tuning, and client profiles.',
    readTime: '10 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'overview',
        title: 'Configuration File Location',
        content:
          'SolarFlare settings live in \`~/.config/sunshine/sunshine.conf\`. You can edit this file directly or configure settings through the Web UI at https://localhost:47990/config.',
        image: {
          src: '/sf-web-ui-configuration.png',
          alt: 'SolarFlare Web UI configuration page',
          caption: 'The Web UI configuration editor mirrors sunshine.conf keys.',
        },
      },
      {
        id: 'host-tunables',
        title: 'Fork Host Tunables',
        content:
          'Switch tabs to browse network, scheduling, capture, and access keys. Full fork prose lives in the sections below; inherited upstream keys are summarized in the Developer articles.',
        tabs: [
          {
            id: 'network',
            label: 'Network',
            params: [
              {
                name: 'busy_poll_us',
                type: 'int',
                defaultVal: '50',
                range: '0 - 10000',
                description: 'SO_BUSY_POLL in microseconds on the ENet UDP socket. 0 disables.',
                example: 'busy_poll_us = 50',
              },
              {
                name: 'rate_cap_pct',
                type: 'int',
                defaultVal: '80',
                range: '50 - 95',
                description: 'Percent of detected link speed used as the send pacer.',
                example: 'rate_cap_pct = 80',
              },
              {
                name: 'enet_4mib_buffer',
                type: 'bool',
                defaultVal: 'true',
                description: 'Grow ENet UDP send and receive buffers to 4 MiB.',
                example: 'enet_4mib_buffer = true',
              },
              {
                name: 'dscp_qos',
                type: 'bool',
                defaultVal: 'true',
                description: 'Tag streaming UDP with DSCP CS3 for router QoS.',
                example: 'dscp_qos = true',
              },
            ],
          },
          {
            id: 'scheduling',
            label: 'CPU / GPU',
            params: [
              {
                name: 'cpu_pinning',
                type: 'bool',
                defaultVal: 'true',
                description: 'SCHED_RR capture thread on a non-IRQ physical core.',
                example: 'cpu_pinning = true',
              },
              {
                name: 'gpu_governor',
                type: 'bool',
                defaultVal: 'true',
                description: 'Raise AMD DRM cards to performance during capture; restore auto on stop.',
                example: 'gpu_governor = true',
              },
              {
                name: 'latency_mode',
                type: 'string',
                defaultVal: 'safe',
                range: 'safe | aggressive',
                description: 'safe keeps quality; aggressive tightens audio queue and software scaler.',
                example: 'latency_mode = aggressive',
              },
            ],
          },
          {
            id: 'capture',
            label: 'Capture',
            params: [
              {
                name: 'pipewire_latency_ms',
                type: 'int',
                defaultVal: '8',
                range: '1 - 40',
                description: 'PW_KEY_NODE_LATENCY hint for PipeWire capture.',
                example: 'pipewire_latency_ms = 8',
              },
              {
                name: 'headless_virtual_display',
                type: 'bool',
                defaultVal: 'false',
                description: 'Create a virtual xrandr output when no physical display is found.',
                example: 'headless_virtual_display = true',
              },
              {
                name: 'skip_wayland_correlation',
                type: 'bool',
                defaultVal: 'false',
                description: 'Skip Wayland-to-KMS correlation if the compositor omits output metadata.',
                example: 'skip_wayland_correlation = false',
              },
              {
                name: 'idle_timeout_min',
                type: 'int',
                defaultVal: '0',
                range: '0 - 600',
                description: 'Stop the stream after N minutes without client input. 0 disables.',
                example: 'idle_timeout_min = 15',
              },
            ],
          },
          {
            id: 'access',
            label: 'Access',
            params: [
              {
                name: 'nvenc_tuning_preset',
                type: 'int',
                defaultVal: '-1',
                range: '-1 to 2',
                description: 'NVENC profile: -1 manual, 0 latency, 1 balanced, 2 quality.',
                example: 'nvenc_tuning_preset = 0',
              },
              {
                name: 'trusted_subnets',
                type: 'string',
                defaultVal: '""',
                description: 'Comma-separated CIDR list used with trusted_subnet_auto_pairing.',
                example: 'trusted_subnets = 192.168.1.0/24',
              },
              {
                name: 'webhook_url_0',
                type: 'string',
                defaultVal: '""',
                description: 'HTTPS endpoint notified on stream start and stop.',
                example: 'webhook_url_0 = https://hooks.example.com/sf',
              },
            ],
          },
        ],
        callout: {
          type: 'important',
          text: 'Inherited upstream keys are summarized in Building / Porting. Fork keys are fully documented in the sections on this page.',
        },
      },
      {
        id: 'audio-fx',
        title: 'Audio FX Pre-Encoder Processing',
        content:
          'SolarFlare includes a lightweight audio signal processor running between PipeWire capture and Opus encoding:',
        params: [
          {
            name: 'sf_audio_agc',
            type: 'bool',
            defaultVal: 'false',
            description: 'Enable Automatic Gain Control to smooth stream audio loudness levels.',
          },
          {
            name: 'sf_audio_agc_target_db',
            type: 'float',
            defaultVal: '-20.0',
            range: '-40.0 to -6.0 dBFS',
            description: 'Target RMS loudness for automatic gain control.',
          },
          {
            name: 'sf_audio_vad',
            type: 'bool',
            defaultVal: 'false',
            description: 'Enable Voice Activity Detection for voice-aware ducking.',
          },
          {
            name: 'sf_audio_ducking',
            type: 'bool',
            defaultVal: 'false',
            description: 'Duck game audio volume when voice speech is detected.',
          },
          {
            name: 'sf_audio_ducker_attenuation_db',
            type: 'float',
            defaultVal: '-12.0',
            range: '-40.0 to 0.0 dB',
            description: 'Game audio attenuation applied when speech is active.',
          },
          {
            name: 'sf_audio_noise_gate',
            type: 'bool',
            defaultVal: 'false',
            description: 'Apply noise gate to eliminate background microphone hum.',
          },
          {
            name: 'sf_audio_noise_gate_db',
            type: 'float',
            defaultVal: '-55.0',
            range: '-90.0 to -10.0 dBFS',
            description: 'Threshold below which audio signal is silenced.',
          },
        ],
      },
      {
        id: 'opus-tuning',
        title: 'Opus Encoder Tuning',
        content:
          'Fine-tune Opus speech vs music mode, VBR behavior, and forward error correction (FEC):',
        params: [
          {
            name: 'sf_opus_application',
            type: 'int',
            defaultVal: '0',
            range: '0 (Restricted LowDelay), 1 (VoIP), 2 (Audio)',
            description: 'Opus application tuning mode.',
          },
          {
            name: 'sf_opus_vbr',
            type: 'int',
            defaultVal: '0',
            range: '0 (CBR), 1 (Constrained VBR), 2 (Full VBR)',
            description: 'Bitrate mode for the audio stream.',
          },
          {
            name: 'sf_opus_complexity',
            type: 'int',
            defaultVal: '10',
            range: '0 - 10',
            description: 'Encoder complexity algorithm trade-off (CPU vs compression).',
          },
          {
            name: 'sf_opus_fec',
            type: 'bool',
            defaultVal: 'true',
            description: 'In-band forward error correction to recover lost audio packets.',
          },
          {
            name: 'sf_opus_expected_loss_pct',
            type: 'int',
            defaultVal: '0',
            range: '0 - 100',
            description: 'Pre-allocate FEC packet redundancy based on expected network loss.',
          },
        ],
      },
      {
        id: 'video-nvenc',
        title: 'NVENC Tuning Presets',
        content:
          'One-click preset tuning for NVIDIA NVENC encoders without low-level manual flag editing:',
        params: [
          {
            name: 'nvenc_tuning_preset',
            type: 'int',
            defaultVal: '-1',
            range: '-1 (Manual), 0 (Latency), 1 (Balanced), 2 (Quality)',
            description: 'Single-knob NVENC profile. 0 enforces zero B-frames and low-delay rate control; 2 enables spatial adaptive quantization.',
          },
        ],
      },
      {
        id: 'webhooks-profiles',
        title: 'Webhooks & Client Profiles',
        content:
          'Automate stream lifecycle events and customize bitrates per client device name:',
        code: {
          language: 'bash',
          code: `# Webhook notifications on stream start and stop
webhook_url_0 = https://home-assistant.local/api/webhook/solarflare-stream
webhook_secret = super-secret-signing-key

# Per-client profile overrides
client_profile_Phone_max_bitrate = 15000
client_profile_Phone_latency_mode = aggressive

client_profile_LivingRoomTV_max_bitrate = 80000
client_profile_LivingRoomTV_hevc_mode = 2`,
        },
      },

      {
        id: 'trust-tokens',
        title: 'Trust, tokens & webhooks',
        params: [
          {
            name: 'trusted_subnet_auto_pairing',
            type: 'bool',
            defaultVal: 'false',
            description: 'Master switch for PIN-less pairing. Clients in trusted_subnets pair without a PIN when enabled. Disabled by default.',
            example: 'trusted_subnet_auto_pairing = false',
          },
          {
            name: 'api_tokens',
            type: 'string',
            defaultVal: '[]',
            description: 'Scoped automation tokens, file-only with no Web UI field. Format name, hash, salt, scopes. Mint via POST /api/tokens; the plaintext is shown once and must be saved to sunshine.conf.',
            example: 'api_tokens = home-assistant\thash\tsalt\tstream:control,logs:get',
          },
          {
            name: 'webhook_secret',
            type: 'string',
            defaultVal: '""',
            description: 'HMAC-SHA256 secret signing every webhook body as X-Solarflare-Signature: sha256=<hex>. Session history and GET /api/sessions work even with no URLs set.',
            example: 'webhook_secret = super-secret-signing-key',
          },
          {
            name: 'client_profile_<name>_<field>',
            type: 'group',
            defaultVal: 'global',
            description: 'Per-device overrides keyed by Moonlight client name (uniqueid), file-only. Fields: max_bitrate (kbps ceiling), hevc_mode (0-3), av1_mode (0-3), latency_mode (safe|aggressive). Applied at launch, restored at session end.',
            example: 'client_profile_Phone_max_bitrate = 15000',
          },
        ],
      },
      {
        id: 'input-seat',
        title: 'Linux input seat isolation',
        content:
          'New in v1.3.0. `input_seat` assigns virtual mouse, keyboard, touch, pen, and gamepad devices to a non-default systemd-logind seat (for example `seat1`). Precedence is `input_seat` > `XDG_SEAT` > empty or `seat0` (no isolation). The runtime injects a transient udev rule at `/run/udev/rules.d/99-solarflare-seat.rules` and synthesizes a change uevent per device; a shipped fallback rule can be installed manually where `/run/udev` is read-only. Same-seat hardening uses exclusive EVIOCGRAB on virtual event nodes. The target seat must exist (`loginctl seat-add`) with a display or input device attached. Set it in the Web UI Inputs tab (Linux only) or directly in config.',
        params: [
          {
            name: 'input_seat',
            type: 'string',
            defaultVal: '""',
            description: 'Target logind seat for virtual input devices. Empty follows XDG_SEAT; seat0 disables isolation.',
            example: 'input_seat = seat1',
          },
        ],
      },
      {
        id: 'headless-compositor',
        title: 'Headless compositor streaming',
        content:
          '`headless_virtual_display` is the simple xrandr VIRTUAL1 fallback for monitor-less X11 (independent knob). `headless_mode` instead routes launches into a private nested compositor. When headless_mode is on, `compositor_backend` selects labwc, krfb, or gamescope (auto picks krfb-virtualmonitor under KWin, else labwc). Width, height, and refresh of 0 follow the client request.',
        params: [
          {
            name: 'headless_mode',
            type: 'bool',
            defaultVal: 'false',
            description: 'Route game launches into a private nested compositor instead of hijacking the desktop.',
            example: 'headless_mode = true',
          },
          {
            name: 'compositor_backend',
            type: 'string',
            defaultVal: 'auto',
            range: 'auto | labwc | krfb | gamescope',
            description: 'Headless display backend selection.',
            example: 'compositor_backend = labwc',
          },
          {
            name: 'linux_use_cage_compositor',
            type: 'bool',
            defaultVal: 'false',
            description: 'Use the labwc nested compositor for headless streaming. Requires headless_mode.',
            example: 'linux_use_cage_compositor = true',
          },
          {
            name: 'headless_width',
            type: 'int',
            defaultVal: '0',
            range: '0 - 7680',
            description: 'Override headless virtual display width. 0 follows the client resolution.',
            example: 'headless_width = 1920',
          },
          {
            name: 'headless_height',
            type: 'int',
            defaultVal: '0',
            range: '0 - 4320',
            description: 'Override headless virtual display height. 0 follows the client resolution.',
            example: 'headless_height = 1080',
          },
          {
            name: 'headless_refresh',
            type: 'int',
            defaultVal: '0',
            range: '0 - 240',
            description: 'Override headless virtual display refresh rate. 0 follows the client framerate.',
            example: 'headless_refresh = 120',
          },
        ],
      },
      {
        id: 'audio-dsp-full',
        title: 'Audio DSP full reference',
        content: 'The seven most-used Audio FX knobs are above. The remaining eleven fine-tune AGC dynamics, VAD sensitivity, and ducker timing:',
        params: [
          {
            name: 'sf_audio_agc_max_gain_db',
            type: 'float',
            defaultVal: '12.0',
            range: '0.0 to 30.0 dB',
            description: 'Maximum AGC boost applied to quiet audio.',
          },
          {
            name: 'sf_audio_agc_min_gain_db',
            type: 'float',
            defaultVal: '-12.0',
            range: '-30.0 to 0.0 dB',
            description: 'Maximum AGC cut applied to loud audio.',
          },
          {
            name: 'sf_audio_agc_attack_ms',
            type: 'int',
            defaultVal: '10',
            range: '1 - 500 ms',
            description: 'Gain-up ramp speed once audio drops below target.',
          },
          {
            name: 'sf_audio_agc_hold_ms',
            type: 'int',
            defaultVal: '200',
            range: '0 - 5000 ms',
            description: 'Hold time before the gain releases after loud passages.',
          },
          {
            name: 'sf_audio_agc_release_ms',
            type: 'int',
            defaultVal: '100',
            range: '1 - 5000 ms',
            description: 'Gain-down ramp speed once audio exceeds target.',
          },
          {
            name: 'sf_audio_vad_threshold_db',
            type: 'float',
            defaultVal: '-45.0',
            range: '-80.0 to -10.0 dBFS',
            description: 'Speech RMS threshold for voice activity detection.',
          },
          {
            name: 'sf_audio_vad_hysteresis_db',
            type: 'float',
            defaultVal: '6.0',
            range: '0.0 to 30.0 dB',
            description: 'Anti-flutter band around the VAD threshold.',
          },
          {
            name: 'sf_audio_vad_min_speech_ms',
            type: 'int',
            defaultVal: '100',
            range: '10 - 2000 ms',
            description: 'Speech debounce: minimum voiced time before ducking engages.',
          },
          {
            name: 'sf_audio_vad_min_silence_ms',
            type: 'int',
            defaultVal: '200',
            range: '10 - 5000 ms',
            description: 'Silence debounce before ducking releases.',
          },
          {
            name: 'sf_audio_ducker_attack_ms',
            type: 'int',
            defaultVal: '50',
            range: '1 - 2000 ms',
            description: 'Duck-down speed when speech starts.',
          },
          {
            name: 'sf_audio_ducker_release_ms',
            type: 'int',
            defaultVal: '500',
            range: '1 - 5000 ms',
            description: 'Duck recovery speed when speech ends.',
          },
          {
            name: 'sf_opus_bandwidth_extension',
            type: 'bool',
            defaultVal: 'true',
            description: 'Allow super-wideband and fullband Opus above 16 kHz. False restricts to wideband-only.',
          },
        ],
      },
      {
        id: 'inherited-groups',
        title: 'Inherited upstream option groups',
        content:
          'Every upstream Sunshine key still works. The most-used groups: Video (`capture`, `encoder`, `adapter_name`, `output_name`, `max_bitrate`, `qp`, `hevc_mode`, `av1_mode`, `sw_preset`, `sw_tune`, plus per-vendor `nvenc_*`, `vaapi_*`, `qsv_*`, `amd_*`, `vt_*`, `vk_*` groups; adaptive bitrate `adaptive_bitrate_enabled`, `adaptive_bitrate_min`, `adaptive_bitrate_max`). Audio (`audio_sink`, `virtual_sink`, `stream_audio`). Network (`upnp`, `address_family`, `bind_address`, `port` default 47989, `external_ip`, `lan_encryption_mode`, `wan_encryption_mode`, `ping_timeout`, `packetsize`, `fec_percentage`). Input (`controller`, `gamepad`, `keyboard`, `mouse`, `keybindings`, DS4/DS5 mapping keys). Security (`origin_web_ui_allowed` pc|lan|wan, `csrf_allowed_origins`, `pkey`, `cert`). Files and logging (`file_apps`, `file_state`, `credentials_file`, `locale`, `sunshine_name`, `min_log_level`, `log_path`, `global_prep_cmd`). The full machine-readable reference is `docs/configuration.md` in the repo; the Web UI exposes the common subset with inline help.',
      },
    ],
  },

  'performance-tuning': {
    slug: 'performance-tuning',
    title: 'Performance & Latency Tuning',
    category: 'Optimization',
    badge: 'Low Latency',
    description: 'System-level optimizations for Linux kernels, GPU governors, CPU pinning, and network transport.',
    readTime: '7 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'linux-tuning',
        title: 'Ready-made profiles',
        content:
          'Copy one profile into ~/.config/sunshine/sunshine.conf, then tune Moonlight bitrate to the link. Measure with the client overlay before changing more keys.',
        tabs: [
          {
            id: 'competitive',
            label: 'Competitive',
            content: 'Wired NVIDIA, 1080p120 or 1440p120.',
            code: {
              language: 'ini',
              code: `busy_poll_us = 50
rate_cap_pct = 90
enet_4mib_buffer = true
dscp_qos = true
cpu_pinning = true
gpu_governor = true
latency_mode = aggressive
pipewire_latency_ms = 4
nvenc_tuning_preset = 0`,
            },
          },
          {
            id: 'quality',
            label: '4K quality',
            content: 'Wired HEVC, single-player.',
            code: {
              language: 'ini',
              code: `busy_poll_us = 50
rate_cap_pct = 80
enet_4mib_buffer = true
latency_mode = safe
pipewire_latency_ms = 8
nvenc_tuning_preset = 2`,
            },
          },
          {
            id: 'wifi',
            label: 'Wi-Fi',
            content: 'Cap Moonlight bitrate below the iperf3 result. DSCP helps only if the AP honors WMM.',
            code: {
              language: 'ini',
              code: `busy_poll_us = 100
rate_cap_pct = 70
enet_4mib_buffer = true
dscp_qos = true
latency_mode = safe
pipewire_latency_ms = 8`,
            },
          },
          {
            id: 'shared',
            label: 'Shared LAN',
            content: 'Leave headroom for other household traffic.',
            code: {
              language: 'ini',
              code: `busy_poll_us = 0
rate_cap_pct = 60
enet_4mib_buffer = true
pipewire_latency_ms = 12
latency_mode = safe`,
            },
          },
          {
            id: 'headless',
            label: 'Headless',
            content: 'Requires KMS capabilities. Set width/height/refresh to match the client.',
            code: {
              language: 'ini',
              code: `headless_virtual_display = true
headless_width = 1920
headless_height = 1080
headless_refresh = 120
capture = kms`,
            },
          },
        ],
      },
      {
        id: 'cpu-governor',
        title: 'CPU Frequency Governor & Pinning',
        content:
          'For 120 FPS + 4K streaming, verify your CPU governor is set to \`performance\` and enable SolarFlare CPU pinning:\n\n\`cpu_pinning = true\` elevates the capture worker to \`SCHED_RR\` real-time policy and pins the thread to a non-IRQ core, bypassing general OS context-switch overhead.',
        code: {
          language: 'bash',
          code: `# Check current scaling governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Set all cores to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`,
        },
      },
      {
        id: 'gpu-power',
        title: 'GPU Clocks & Power Governors',
        content:
          '- **AMD GPUs:** Setting \`gpu_governor = true\` in sunshine.conf forces \`power_dpm_force_performance_level\` to \`performance\` during active streaming.\n- **NVIDIA GPUs:** Prevent GPU down-clocking during video capture using clock locking (\`nvidia-smi -lgc <boost_clock>\`).',
        code: {
          language: 'bash',
          code: `# Query maximum graphics boost clock
nvidia-smi --query-gpu=clocks.max.graphics --format=csv,noheader

# Lock GPU clock to boost frequency (e.g. 2100 MHz)
sudo nvidia-smi -lgc 2100,2100`,
        },
      },
      {
        id: 'network-tuning',
        title: 'Network Buffers & Low-Latency Polling',
        content:
          'Wi-Fi and high-bitrate LAN links benefit from expanded socket buffers and busy-polling:',
        params: [
          {
            name: 'enet_4mib_buffer = true',
            type: 'Network',
            defaultVal: 'true',
            description: 'Expands socket buffers so 50+ Mbps bursts do not get dropped in kernel send queues.',
          },
          {
            name: 'busy_poll_us = 50',
            type: 'Network',
            defaultVal: '50',
            description: 'Enables kernel SO_BUSY_POLL for instant UDP wakeup without burning full CPU cores.',
          },
          {
            name: 'dscp_qos = true',
            type: 'Network',
            defaultVal: 'true',
            description: 'Marks streaming packets with DSCP CS3 so QoS-enabled routers prioritize stream traffic.',
          },
        ],
      },
      {
        id: 'kernel-sysctl',
        title: 'Recommended Kernel sysctls',
        content: 'Add the following tweaks to \`/etc/sysctl.d/99-streaming.conf\`:',
        code: {
          language: 'ini',
          code: `# Increase socket max buffer sizes for 4K streaming
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.core.rmem_default = 4194304
net.core.wmem_default = 4194304

# Enable BBR congestion control
net.core.default_qdisc = fq
net.ipv4.tcp_congestion_control = bbr`,
        },
      },
      {
        id: 'latency-deep-dive',
        title: 'Latency deep-dive: measure first',
        content:
          'Read the Moonlight overlay (decode + network + render) next to host `GET /api/stream/latency` (`capture_ms`, `encode_ms`, `network_total_ms`, `network_queue_dwell_ms`, `rtt_ms`) and `GET /api/stream/telemetry` (CPU/GPU/RAM pressure). If capture_ms is high, suspect the compositor or CPU pinning; if encode_ms is high, drop a preset level or resolution; if network/rtt dominates, run iperf3 UDP at the Moonlight bitrate and keep loss under 5% with jitter under 1 ms before touching knobs.',
        code: {
          language: 'bash',
          code: `iperf3 -s
iperf3 -c <host> -u -b 80M -t 10
ping -c 100 <host> | tail -3
curl -sk -H "Authorization: Bearer $TOK" https://localhost:47990/api/stream/latency | jq .capture_ms,.encode_ms,.rtt_ms`,
        },
      },
      {
        id: 'link-tuning',
        title: 'Link, buffers & adaptive bitrate',
        content:
          'Link speed is auto-detected from `/sys/class/net/<iface>/speed`; a 2.5 GbE host feeding a 1 GbE client must cap with `rate_cap_pct` (70 or lower on Wi-Fi) or buffers overrun. `enet_4mib_buffer` expands socket buffers for 50+ Mbps bursts. For lossy links enable `adaptive_bitrate_enabled` with sane min/max bounds, raise `fec_percentage`, and set Opus `sf_opus_fec` plus `sf_opus_expected_loss_pct`. DSCP CS3 helps only if the router and AP honor QoS/WMM. Note the BBR sysctl below shapes TCP control traffic, not the UDP stream itself.',
        params: [
          {
            name: 'adaptive_bitrate_enabled',
            type: 'bool',
            defaultVal: 'false',
            description: 'Let client network feedback move the bitrate between min and max at runtime.',
            example: 'adaptive_bitrate_enabled = true',
          },
          {
            name: 'fec_percentage',
            type: 'int',
            defaultVal: '20',
            range: '1 - 255',
            description: 'Forward error correction redundancy for the video stream. Raise on lossy Wi-Fi.',
            example: 'fec_percentage = 30',
          },
          {
            name: 'packetsize',
            type: 'int',
            defaultVal: '0',
            range: '0, 200 - 65535',
            description: 'RTP packet size. 0 leaves MTU discovery automatic; lower only on fragmented paths.',
            example: 'packetsize = 1392',
          },
        ],
      },
      {
        id: 'vendor-profiles',
        title: 'Vendor & link profiles',
        content: 'Start from the closest profile, then A/B one knob at a time and re-measure:',
        tabs: [
          {
            id: 'intel',
            label: 'Intel VAAPI/QSV',
            code: {
              language: 'ini',
              code: `encoder = vaapi
capture = kms
latency_mode = safe
pipewire_latency_ms = 8`,
            },
          },
          {
            id: 'amd',
            label: 'AMD VAAPI 4K',
            code: {
              language: 'ini',
              code: `encoder = vaapi
capture = kms
gpu_governor = true
rate_cap_pct = 80
latency_mode = safe`,
            },
          },
          {
            id: 'lossy',
            label: 'Lossy Wi-Fi',
            code: {
              language: 'ini',
              code: `adaptive_bitrate_enabled = true
adaptive_bitrate_min = 5000
adaptive_bitrate_max = 30000
fec_percentage = 30
rate_cap_pct = 70`,
            },
          },
          {
            id: 'deck',
            label: 'Deck / phone 720p',
            code: {
              language: 'ini',
              code: `client_profile_Deck_max_bitrate = 12000
client_profile_Deck_latency_mode = aggressive
latency_mode = safe`,
            },
          },
        ],
      },
      {
        id: 'hdr-tuning',
        title: 'HDR streaming chain',
        content:
          'HDR needs the whole chain: KMS capture, an HDR compositor (Plasma 6, Gamescope), a 10-bit encoder (`hevc_mode` or `av1_mode` 3+), HDR enabled in host OS and Moonlight client, and an EDID emulator or HDR display on the host. Washed-out output means something fell back to SDR — check the colorspace lines in the log and the `IsHdrSupported` negotiation.',
      },
      {
        id: 'symptom-table',
        title: 'Symptom to knob table',
        table: {
          headers: ['Symptom', 'Check', 'Knob'],
          rows: [
            ['KMS black + Probably not permitted', 'getcap, /dev/dri perms', 'setcap; video group'],
            ['vainfo fails', 'libva driver present', 'Install driver or encoder = software'],
            ['No monitor', 'xrandr --listmonitors', 'VIRTUAL1 or headless_mode'],
            ['No sink', 'pactl/wpctl sinks', 'audio_sink = name'],
            ['Loss over 5%', 'iperf3 UDP', 'rate_cap_pct down; adaptive on'],
            ['401 from API', 'Credentials', 'sunshine --creds reset'],
            ['403 from API', 'Scope / origin / CSRF', 'Token scope; origin_web_ui_allowed; X-CSRF-Token'],
            ['Update stuck waiting_idle', 'GET /api/sessions', 'End stream or cancel'],
          ],
        },
      },
    ],
  },

  'api': {
    slug: 'api',
    title: 'REST API Reference',
    category: 'Developer & API',
    badge: 'REST API',
    description: 'Comprehensive REST API documentation for host administration, scoped API tokens, telemetry, and self-updater.',
    readTime: '9 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'auth',
        title: 'Authentication & Scoped Tokens',
        content:
          'All API endpoints require authentication using Basic Auth (admin credentials) or Scoped Bearer Tokens via the \`Authorization: Bearer <token>\` header.\n\nState-changing requests (POST, DELETE) from browser clients validate CSRF tokens via \`X-CSRF-Token\`. Non-browser API clients (curl, scripts, Home Assistant) are exempt from CSRF checks.',
      },
      {
        id: 'tokens-api',
        title: 'Scoped API Tokens Endpoints',
        content: 'Mint, list, and revoke automation tokens. The plaintext token is returned only on create.',
        tabs: [
          {
            id: 'list',
            label: 'GET /api/tokens',
            endpoints: [
              {
                method: 'GET',
                path: '/api/tokens',
                auth: 'Admin or tokens:manage',
                scopes: ['tokens:manage'],
                description: 'List active automation tokens with assigned scopes (hashes omitted).',
                responseBody: `{\n  "status": true,\n  "status_code": 200,\n  "tokens": [\n    {\n      "name": "home-assistant",\n      "scopes": ["stream:control", "logs:get"]\n    }\n  ]\n}`,
              },
            ],
          },
          {
            id: 'create',
            label: 'POST /api/tokens',
            endpoints: [
              {
                method: 'POST',
                path: '/api/tokens',
                auth: 'Admin or tokens:manage',
                scopes: ['tokens:manage'],
                description: 'Mint a scoped API token. Store the plaintext immediately.',
                requestBody: `{\n  "name": "ci-monitor",\n  "scopes": ["logs:get", "stream:stats"]\n}`,
                responseBody: `{\n  "status": true,\n  "status_code": 200,\n  "name": "ci-monitor",\n  "plaintext": "sf_tok_abc123...",\n  "scopes": ["logs:get", "stream:stats"]\n}`,
              },
            ],
          },
          {
            id: 'revoke',
            label: 'DELETE /api/tokens/{name}',
            endpoints: [
              {
                method: 'DELETE',
                path: '/api/tokens/{name}',
                auth: 'Admin or tokens:manage',
                scopes: ['tokens:manage'],
                description: 'Revoke and delete a named API token.',
                responseBody: `{\n  "status": true,\n  "status_code": 200\n}`,
              },
            ],
          },
        ],
      },
      {
        id: 'stream-telemetry',
        title: 'Stream Telemetry & Adaptive Bitrate',
        content: 'Host-side latency, bitrate bounds, client network feedback, and error counters.',
        tabs: [
          {
            id: 'latency',
            label: 'Latency',
            endpoints: [
              {
                method: 'GET',
                path: '/api/stream/latency',
                auth: 'logs:get',
                description: 'Host-side latency breakdown in milliseconds.',
                responseBody: `{\n  "status": true,\n  "capture_ms": { "min": 0.8, "max": 2.1, "avg": 1.1, "samples": 300 },\n  "encode_ms": { "min": 1.2, "max": 3.4, "avg": 1.8, "samples": 300 }\n}`,
              },
            ],
          },
          {
            id: 'bitrate',
            label: 'Bitrate',
            endpoints: [
              {
                method: 'GET',
                path: '/api/stream/bitrate',
                auth: 'config:get',
                description: 'Adaptive bitrate parameters and bounds.',
                responseBody: `{\n  "status": true,\n  "adaptive_bitrate_enabled": true,\n  "adaptive_bitrate_min": 5000,\n  "adaptive_bitrate_max": 60000\n}`,
              },
            ],
          },
          {
            id: 'netstats',
            label: 'Network stats',
            endpoints: [
              {
                method: 'POST',
                path: '/api/stream/network-stats',
                auth: 'logs:get',
                description: 'Ingest client packet loss and RTT into the bitrate pacer.',
                requestBody: `{\n  "packet_loss_pct": 0.5,\n  "rtt_ms": 18.2\n}`,
              },
            ],
          },
          {
            id: 'errors',
            label: 'Errors',
            endpoints: [
              {
                method: 'GET',
                path: '/api/errors',
                auth: 'logs:get',
                description: 'Categorized error counts across encoder, capture, network, and session.',
                responseBody: `{\n  "status": true,\n  "encoder": 0,\n  "capture": 0,\n  "network": 0,\n  "session": 0,\n  "total": 0\n}`,
              },
            ],
          },
        ],
      },
      {
        id: 'updater-api',
        title: 'Host Self-Updater Endpoints',
        content: 'Linux in-app updater. Active streams block apply until idle unless forced.',
        tabs: [
          {
            id: 'status',
            label: 'Status',
            endpoints: [
              {
                method: 'GET',
                path: '/api/update',
                auth: 'config:get',
                description: 'Query updater state, progress, and release notes.',
              },
            ],
          },
          {
            id: 'start',
            label: 'Start',
            endpoints: [
              {
                method: 'POST',
                path: '/api/update/start',
                auth: 'admin',
                description: 'Download and checksum the latest solarflare-linux-x86_64.tar.gz.',
              },
            ],
          },
          {
            id: 'apply',
            label: 'Apply',
            endpoints: [
              {
                method: 'POST',
                path: '/api/update/apply',
                auth: 'admin',
                description: 'Install the staged archive now or when streams go idle.',
                requestBody: `{\n  "when_idle": true\n}`,
              },
            ],
          },
          {
            id: 'cancel',
            label: 'Cancel',
            endpoints: [
              {
                method: 'POST',
                path: '/api/update/cancel',
                auth: 'config:set',
                description: 'Cancel a pending when-idle apply.',
              },
            ],
          },
        ],
      },
      {
        id: 'game-scanner-api',
        title: 'Game Scanner & Health Check',
        endpoints: [
          {
            method: 'GET',
            path: '/api/games/scan',
            auth: 'apps:get scope',
            description: 'Scan host for Steam, Lutris, and Heroic installed games.',
            responseBody: `[\n  {\n    "name": "Hades",\n    "path": "/home/user/.steam/steam/steamapps/common/Hades/Hades",\n    "launcher": "steam"\n  }\n]`,
          },
          {
            method: 'GET',
            path: '/api/health',
            auth: 'Unauthenticated',
            description: 'Health check endpoint for container orchestrators and load balancers.',
            responseBody: `{\n  "status": "ok",\n  "status_code": 200,\n  "version": "2026.909.1",\n  "uptime": 3600\n}`,
          },
        ],
      },

      {
        id: 'transport-auth',
        title: 'Transport, auth & envelopes',
        content:
          'Base URL is `https://<host>:47990/api/...` — the Web UI port is the GameStream `port` (default 47989) plus one, and `port` is configurable. TLS uses the configured `cert`/`pkey` pair (self-signed by default; pass `-k`/`--insecure` to curl or install a trusted cert). Every JSON response carries `Strict-Transport-Security`, `X-Frame-Options: DENY`, and `frame-ancestors none`. Success envelope is `{status: true, status_code: 200, ...}`; failures are `{status: false, status_code, error}` with 400 validation, 401 unauthenticated (+ `WWW-Authenticate: Basic realm="Sunshine Gamestream Host"`), 403 wrong scope / origin / CSRF, 404 unknown path, 413 bodies over 1 MiB, 429 rate-limited. Auth tries `Authorization: Bearer <64-hex-token>` first (SHA-256 of `plaintext:salt` vs `api_tokens` entries), then HTTP Basic (always full admin, bypasses scopes). A `*` token is admin. Full scope vocabulary: `config:get`, `config:set`, `apps:get`, `apps:launch`, `apps:close`, `clients:list`, `clients:pair`, `clients:unpair`, `logs:get`, `display:reset`, `tokens:manage`, `*`. Login failures are bucketed per IP (10 fails / 30 s → 429); success resets. Mutating browser calls need `X-CSRF-Token` (or `?csrf_token=`); mint one via `GET /api/csrf-token` (1 h TTL, per client id). Same-origin and origin-allowlisted (`csrf_allowed_origins` plus auto localhost) calls skip the token; curl/scripts with no Origin/Referer are exempt.',
        code: {
          language: 'bash',
          code: 'curl -ku admin:pass https://localhost:47990/api/config | jq .version\nTOK=sf_hex_here\ncurl -sk -H "Authorization: Bearer $TOK" https://localhost:47990/api/sessions?limit=5 | jq .\nCSRF=$(curl -sk -u admin:pass https://localhost:47990/api/csrf-token | jq -r .csrf_token)\ncurl -sk -u admin:pass -H "X-CSRF-Token: $CSRF" -H \'Content-Type: application/json\' -d \'{}\' https://localhost:47990/api/config -w \'%{http_code}\\n\'',
        },
      },
      {
        id: 'apps-covers',
        title: 'Apps, covers, browser & game scanner',
        endpoints: [
          {
            method: 'GET',
            path: '/api/apps',
            auth: 'apps:get',
            description: 'Return the apps.json catalog as {apps[]}. An empty file yields {apps:[]}.',
          },
          {
            method: 'POST',
            path: '/api/apps',
            auth: 'config:set + CSRF, Content-Type: application/json',
            description: 'Create (index -1) or update (index N) an app: {name, output, cmd, index, exclude-global-prep-cmd, elevated, auto-detach, wait-all, exit-timeout, prep-cmd[{do, undo, elevated}], detached[], image-path}.',
          },
          {
            method: 'DELETE',
            path: '/api/apps/{index}',
            auth: 'config:set + CSRF',
            description: 'Delete the app at numeric index. Path regex only matches digits.',
          },
          {
            method: 'POST',
            path: '/api/apps/close',
            auth: 'apps:close + CSRF',
            description: 'Terminate the running streamed process.',
          },
          {
            method: 'GET',
            path: '/api/covers/{index}',
            auth: 'apps:get',
            description: 'Return image/png artwork for the app index. 404 when none is set.',
          },
          {
            method: 'POST',
            path: '/api/covers/upload',
            auth: 'config:set',
            description: 'Upload artwork as {key, url|data}. Key must not contain /../ or NUL; url host must be images.igdb.com, otherwise send base64 data. Saved to appdata/covers/<key>.png.',
          },
          {
            method: 'GET',
            path: '/api/browse',
            auth: 'config:get',
            description: 'Browse host paths: ?path=&type=directory|executable|file|any returns {path, parent, entries[{name, path, type}]}. Empty path lists / (Linux) or drives (Windows). Combine with least-privilege tokens and origin_web_ui_allowed to limit exposure.',
          },
        ],
      },
      {
        id: 'clients-pairing',
        title: 'Clients & pairing',
        endpoints: [
          {
            method: 'GET',
            path: '/api/clients/list',
            auth: 'clients:list',
            description: 'List paired clients as {status, named_certs[]}.',
          },
          {
            method: 'POST',
            path: '/api/clients/unpair',
            auth: 'clients:unpair + CSRF + JSON',
            description: 'Unpair one client: {uuid}.',
          },
          {
            method: 'POST',
            path: '/api/clients/unpair-all',
            auth: 'clients:unpair + CSRF',
            description: 'Erase all pairings and terminate running sessions.',
          },
          {
            method: 'POST',
            path: '/api/clients/update',
            auth: 'admin + CSRF + JSON',
            description: 'Enable or disable a cert: {uuid, enabled}. Disabling kills that client sessions.',
          },
          {
            method: 'POST',
            path: '/api/pin',
            auth: 'clients:pair + CSRF + JSON',
            description: 'Pair with the Moonlight PIN: {pin: 0000-9999, name}. Returns {status}.',
          },
          {
            method: 'POST',
            path: '/api/password',
            auth: 'admin + CSRF + JSON (unauth on first run)',
            description: 'Set credentials: {currentUsername, currentPassword, newUsername, newPassword, confirmNewPassword}. First-run with no username set is unauthenticated and redirects to /welcome otherwise.',
          },
        ],
      },
      {
        id: 'config-system',
        title: 'Config, logs & system',
        endpoints: [
          {
            method: 'GET',
            path: '/api/config',
            auth: 'config:get',
            description: 'Dump effective config: {status, platform, version, ...sunshine.conf vars plus solarflare audio_fx/headless/adaptive/trusted defaults}.',
          },
          {
            method: 'POST',
            path: '/api/config',
            auth: 'config:set + CSRF, JSON object only',
            description: 'Write {key: value} pairs. Arrays, scalars, or empty objects are rejected with 400 (they would wipe the file); write failures return {status: false, error}.',
          },
          {
            method: 'GET',
            path: '/api/configLocale',
            auth: 'Unauthenticated',
            description: 'Return {status, locale} for the Web UI boot path.',
          },
          {
            method: 'GET',
            path: '/api/logs',
            auth: 'logs:get',
            description: 'Return text/plain host logs (not JSON).',
          },
          {
            method: 'GET',
            path: '/api/csrf-token',
            auth: 'Any authenticated caller',
            description: 'Mint a CSRF token: {csrf_token} (32 random bytes, 1 h TTL).',
          },
          {
            method: 'GET',
            path: '/api/stream/telemetry',
            auth: 'logs:get',
            description: 'Host resource monitor: {status, status_code, telemetry{host_cpu_pct[], host_gpu_pct[], host_ram_used_mb[], window_s}}. Non-Linux returns window_s only.',
          },
          {
            method: 'GET',
            path: '/api/sessions',
            auth: 'logs:get',
            description: 'Session history with ?limit=100&app=&client=. Records match webhook payloads minus the event wrapper.',
          },
          {
            method: 'POST',
            path: '/api/reset-display-device-persistence',
            auth: 'display:reset + CSRF',
            description: 'Reset Windows display-device persistence. Windows-only.',
          },
          {
            method: 'POST',
            path: '/api/restart',
            auth: 'admin + CSRF',
            description: 'Restart the host process. May not return a body.',
          },
          {
            method: 'GET',
            path: '/api/vigembus/status',
            auth: 'config:get',
            description: 'ViGEmBus driver state: {installed, version, version_compatible, packaged_version}. Non-Windows returns error with empty values.',
          },
          {
            method: 'POST',
            path: '/api/vigembus/install',
            auth: 'admin + CSRF',
            description: 'Run vigembus_installer.exe /quiet. Non-Windows returns {status: false, error}.',
          },
        ],
      },
      {
        id: 'webhook-payloads',
        title: 'Webhook payloads',
        content:
          'On stream start/stop SolarFlare POSTs application/json to every `webhook_url_N` (http(s) only, 5 s connect/timeout, 2 retries with backoff, no retry on 4xx). With `webhook_secret` set, each request carries `X-Solarflare-Signature: sha256=HMAC(secret, body)`. Body: {event: stream.start|stream.end, t_start, t_end, app_name, client_name, client_address, codec, width, height, fps, avg_bitrate_kbps, avg_rtt_ms, avg_encode_ms, dropped_frames, error}. Identical to GET /api/sessions items except for the event wrapper — verify the HMAC before acting on callbacks.',
      },
    ],
  },

  'porting': {
    slug: 'porting',
    title: 'Multi-Distro Porting Guide',
    category: 'Developer & API',
    badge: 'Linux',
    description: 'Package translation matrices, dependencies, and build requirements for all major Linux distributions.',
    readTime: '6 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'distro-matrix',
        title: 'Distribution Dependency Matrix',
        content:
          'SolarFlare compiles natively across Arch, Debian, Ubuntu, Fedora, and openSUSE. Package names for dependencies are translated below:',
        table: {
          headers: ['Component', 'Arch / CachyOS', 'Debian / Ubuntu', 'Fedora / Nobara', 'openSUSE'],
          rows: [
            ['Compiler', 'base-devel (GCC 14+)', 'build-essential (GCC 13+)', 'gcc-c++', 'gcc-c++'],
            ['Build System', 'cmake ninja', 'cmake ninja-build', 'cmake ninja-build', 'cmake ninja'],
            ['Audio', 'libpipewire libpulse', 'libpipewire-0.3-dev libpulse-dev', 'pipewire-devel pulseaudio-libs-devel', 'pipewire-devel libpulse-devel'],
            ['Video / DRM', 'libdrm libva', 'libdrm-dev libva-dev', 'libdrm-devel libva-devel', 'libdrm-devel libva-devel'],
            ['Wayland Protocols', 'wayland-protocols libportal', 'wayland-protocols libportal-dev', 'wayland-protocols-devel libportal-devel', 'wayland-protocols-devel libportal-devel'],
            ['Opus DSP', 'opus', 'libopus-dev', 'opus-devel', 'opus-devel'],
            ['FFmpeg Codecs', 'ffmpeg', 'ffmpeg', 'ffmpeg-devel', 'ffmpeg-devel'],
            ['Vulkan SDK', 'vulkan-headers', 'vulkan-headers / vulkan-sdk', 'vulkan-devel', 'vulkan-devel'],
          ],
        },
      },
      {
        id: 'distro-quirks',
        title: 'Distribution-Specific Details',
        content: 'Use the tabs for distro quirks. The matrix above lists package names.',
        tabs: [
          {
            id: 'arch',
            label: 'Arch / CachyOS',
            content:
              'Arch provides current GCC 14+ and kernel 6.x. scripts/cachyos-build.sh forwards to linux-install.sh. Confirm cap_sys_admin after install.',
          },
          {
            id: 'debian',
            label: 'Ubuntu / Debian',
            content:
              'Ubuntu 22.04 may need gcc-13 g++-13. Prefer CMake from the distro if it is at least 3.20.',
          },
          {
            id: 'fedora',
            label: 'Fedora / Nobara',
            content: 'Install vulkan-devel and shaderc for Vulkan encode. RPM Fusion may be required for ffmpeg-devel.',
          },
          {
            id: 'suse',
            label: 'openSUSE',
            content: 'Use ffmpeg-devel and libopenssl-3-devel. Tumbleweed tracks current toolchains.',
          },
          {
            id: 'bazzite',
            label: 'Bazzite',
            content:
              'rpm-ostree layering requires a reboot after the first installer pass. Re-run with --skip-deps to finish the build.',
          },
          {
            id: 'nixos',
            label: 'NixOS',
            content:
              'Use the repository Nix shell and the declarative host settings in the Porting article. Do not mix apt/dnf packages.',
          },
        ],
      },
      {
        id: 'nixos-decl',
        title: 'NixOS declarative block',
        content:
          'NixOS never uses apt/dnf. Enter the repo Nix shell (it installs into ~/.local with units under ~/.config/systemd/user), export ~/.local/bin onto PATH, and apply the host-side NixOS settings (uinput device, video group membership, firewall ports) with nixos-rebuild switch. The imperative redesign boot units are skipped on NixOS — express CPU governor, ethtool, and NVIDIA tuning declaratively instead.',
        code: {
          language: 'bash',
          code: `nix-build packaging/linux/nixos/shell.nix --out-link ~/.local/share/solarflare/build-environment
SOLARFLARE_NIX_SHELL=1 nix-shell packaging/linux/nixos/shell.nix --run "./scripts/linux-install.sh $@"
export PATH="$HOME/.local/bin:$PATH"
sudo nixos-rebuild switch`,
        },
      },
      {
        id: 'bazzite-reboot',
        title: 'Bazzite two-pass install',
        content:
          'rpm-ostree layering cannot finish live: the first installer pass layers the toolchain and then stops with REBOOT REQUIRED. Reboot, then re-run with --skip-deps to compile against the layered packages.',
        code: {
          language: 'bash',
          code: `./scripts/linux-install.sh
sudo systemctl reboot
./scripts/linux-install.sh --skip-deps`,
        },
      },
      {
        id: 'cuda-packaging',
        title: 'CUDA, packaging & verification',
        content:
          'Vulkan encode needs vulkan-devel plus shaderc. CUDA builds add -DSUNSHINE_ENABLE_CUDA=ON with CMAKE_CUDA_COMPILER pointing at nvcc; keep CUDA_FAIL_ON_MISSING=ON so a silent CPU-only binary never ships by accident. Package with cpack (DEB/RPM), the AppImage AppDir flow, or flatpak build-bundle; Arch uses SUNSHINE_CONFIGURE_PKGBUILD. Verify every port the same way: getcap shows both capabilities, curl -k reports 401, and sunshine --version prints the Fork: SolarFlare line.',
      },
    ],
  },

  'app-examples': {
    slug: 'app-examples',
    title: 'App & Game Examples',
    category: 'Configuration',
    badge: 'Apps',
    description: 'Launch commands, working directory setups, detached commands, and per-app encoder presets.',
    readTime: '5 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'overview',
        title: 'Application Management in apps.json',
        content:
          'Applications and game shortcuts are stored in \`~/.config/sunshine/apps.json\`. You can manage apps through the Web UI Application tab or edit the JSON file directly.',
        image: {
          src: '/sf-web-ui-applications.png',
          alt: 'SolarFlare Web UI applications editor',
          caption: 'Manage launch commands and per-app overrides in the Applications tab.',
        },
      },
      {
        id: 'steam-bigpicture',
        title: 'Launcher examples',
        content: 'apps.json lives in ~/.config/sunshine/apps.json. Use the tabs for common launchers.',
        tabs: [
          {
            id: 'steam',
            label: 'Steam',
            code: {
              language: 'json',
              code: `{\n  "name": "Steam Big Picture",\n  "cmd": "",\n  "detached": ["setsid steam steam://open/bigpicture"],\n  "prep-cmd": [{ "do": "", "undo": "setsid steam steam://close/bigpicture" }],\n  "image-path": "steam.png"\n}`,
            },
          },
          {
            id: 'appid',
            label: 'Steam AppID',
            code: {
              language: 'json',
              code: `{\n  "name": "Competitive",\n  "cmd": "steam steam://rungameid/730",\n  "encoder-preset": 0\n}`,
            },
          },
          {
            id: 'gamescope',
            label: 'Gamescope',
            code: {
              language: 'json',
              code: `{\n  "name": "Gamescope session",\n  "cmd": "gamescope -W 2560 -H 1440 -r 120 -- steam steam://rungameid/1091500"\n}`,
            },
          },
          {
            id: 'lutris',
            label: 'Lutris',
            code: {
              language: 'json',
              code: `{\n  "name": "Lutris game",\n  "cmd": "lutris lutris:rungameid/1"\n}`,
            },
          },
          {
            id: 'quality',
            label: 'Quality preset',
            content: 'encoder-preset: -1 host default, 0 latency, 1 balanced, 2 quality.',
            code: {
              language: 'json',
              code: `{\n  "name": "Story title",\n  "cmd": "steam steam://rungameid/1174180",\n  "encoder-preset": 2\n}`,
            },
          },
        ],
      },
      {
        id: 'encoder-preset',
        title: 'Per-Application Encoder Tuning Preset',
        content:
          'SolarFlare allows setting an NVENC encoder preset on a per-game basis via \`encoder-preset\`:\n\n- \`-1\`: Inherit host default configuration (\`nvenc_tuning_preset\`)\n- \`0\`: Latency (fastest encoding, zero B-frames)\n- \`1\`: Balanced (balanced performance/quality)\n- \`2\`: Quality (2-pass high visual fidelity)',
        code: {
          language: 'json',
          code: `{\n  "name": "Competitive Fast FPS",\n  "cmd": "gamelauncher",\n  "encoder-preset": 0,\n  "image-path": "shooter.png"\n}`,
        },
      },
      {
        id: 'gamescope',
        title: 'Sandboxed Gamescope Session',
        content:
          'Run a game inside Valve Gamescope micro-compositor for sandboxed resolution control and integer scaling:',
        code: {
          language: 'json',
          code: `{\n  "name": "Cyberpunk 2077 (Gamescope)",\n  "cmd": "gamescope -W 2560 -H 1440 -r 120 -- steam steam://rungameid/1091500",\n  "image-path": "cyberpunk.png"\n}`,
        },
      },
      {
        id: 'field-reference',
        title: 'apps.json field reference',
        content:
          'Every entry supports: name, cmd, working-dir (required by some games), output (null path appends vs discards), env (top-level map with $(VAR) expansion plus injected SUNSHINE_APP_* and SUNSHINE_CLIENT_*), auto-detach (default true, exit-0 heuristic), wait-all (default true, process-group wait), exit-timeout (default 5 s), exclude-global-prep-cmd alongside global prep_cmds, elevated on the app and on each prep-cmd, prep-cmd [{do, undo}] where empty do is skipped and undo runs on terminate, detached [cmds] that are never tracked, image-path (must be valid PNG, relative or absolute, default box.png; id is CRC32 of name+image), and encoder-preset (-1 inherit, 0 latency, 1 balanced, 2 quality; out-of-range values are silently ignored and the host default is kept, restored on terminate).',
      },
      {
        id: 'epic-flatpak-heroic',
        title: 'Epic, Heroic and Flatpak launches',
        content:
          'The built-in game scanner (Web UI Applications tab, GET /api/games/scan) finds Steam, Lutris, and Heroic titles: Lutris entries store a directly-pasteable lutris lutris:<slug> path. Heroic scan results need manual cmd translation. Flatpak games launch through flatpak run (note the Flatpak Steam data path ~/.var/app/... when setting working-dir). Manage the catalog through the Web UI Applications tab, by editing ~/.config/sunshine/apps.json directly, or via GET/POST /api/apps.',
        code: {
          language: 'json',
          code: `{
  "name": "Flatpak Steam game",
  "cmd": "flatpak run com.valvesoftware.Steam steam://rungameid/1091500",
  "working-dir": "/home/user/.var/app/com.valvesoftware.Steam",
  "image-path": "cyberpunk.png"
}`,
        },
      },
      {
        id: 'prep-cmd-elevated',
        title: 'Prep commands and elevated launches',
        content:
          'Use prep-cmd do/undo pairs for resolution or refresh-rate switches around a game. Mark elevated true on the app or individual prep steps when anti-cheat or drivers demand it. Prefer detached for fire-and-forget launchers such as Steam Big Picture via setsid so the tracked process tree stays clean.',
        code: {
          language: 'json',
          code: `{
  "name": "High-refresh session",
  "cmd": "steam steam://rungameid/730",
  "prep-cmd": [{ "do": "xrandr --output DP-1 --mode 2560x1440 --rate 120", "undo": "xrandr --output DP-1 --mode 3840x2160 --rate 60" }],
  "encoder-preset": 0
}`,
        },
      },
    ],
  },

  'troubleshooting': {
    slug: 'troubleshooting',
    title: 'Troubleshooting & Diagnostics',
    category: 'Optimization',
    badge: 'Diagnostics',
    description: 'Diagnosing display capture, PipeWire audio routing, controller mappings, and GPU encoding issues.',
    readTime: '7 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'diagnostics-flow',
        title: 'Quick Diagnostic Checklist',
        content:
          'When encountering streaming issues, follow this verification flow:\n\n1. **Check Logs:** Open https://localhost:47990/logs or run \`journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -n 100\`.\n2. **Check Port Bindings:** Verify UDP 47998-48010 and TCP 47984/47990 are listening (\`ss -tulwn | grep -E "4798|4799|4800|4801"\`).\n3. **Inspect Subsystem Errors:** Query \`/api/errors\` to check if encoder or capture counters are incrementing.',
        image: {
          src: '/sf-web-ui-troubleshooting.png',
          alt: 'SolarFlare Web UI troubleshooting page',
          caption: 'The Troubleshooting tab surfaces logs and recent errors in the Web UI.',
        },
      },
      {
        id: 'display-capture',
        title: 'Capture, audio, and network diagnostics',
        content: 'Pick the symptom tab that matches the Moonlight overlay or host logs.',
        tabs: [
          {
            id: 'video',
            label: 'Black screen',
            content:
              'Confirm capture backend (kms, portal, x11). KMS needs cap_sys_admin. NVIDIA needs nvidia_drm.modeset=1. KWin overlays can break KMS on Plasma 6.5+ (KWIN_USE_OVERLAYS=0).',
            code: {
              language: 'bash',
              code: 'getcap "$(command -v sunshine)"\njournalctl --user -u app-dev.lizardbyte.app.Sunshine.service -n 80 --no-pager',
            },
          },
          {
            id: 'audio',
            label: 'No audio',
            content:
              'Confirm the default sink plays locally. Raise pipewire_latency_ms to 8-12 ms if you hear crackle. Check pavucontrol while a stream is active.',
          },
          {
            id: 'network',
            label: 'Stutter',
            content:
              'Run iperf3 UDP at the Moonlight bitrate. Keep packet loss under 5% and jitter under 1 ms. Lower rate_cap_pct on Wi-Fi. Enable enet_4mib_buffer for 4K.',
          },
          {
            id: 'input',
            label: 'Input',
            content:
              'Add the user to the input group and re-login. Steam should use Generic Gamepad, not Xbox/PS overlays. Disconnect unused physical pads so games do not grab them first.',
          },
          {
            id: 'pairing',
            label: 'Pairing',
            content:
              'Sync host time. Enter the PIN from the Web UI. Overly broad trusted_subnets can pair unexpected clients; a bad CIDR can block LAN clients.',
          },
        ],
      },
      {
        id: 'audio-issues',
        title: 'PipeWire & Audio Sink Diagnostics',
        content:
          'SolarFlare connects to PipeWire directly.\n\n- If no audio is received on client: Open \`pavucontrol\` or \`qpwgraph\` while streaming. Verify that the SolarFlare capture stream is linked to your default audio sink monitor.\n- If audio crackles: Increase \`pipewire_latency_ms\` from \`1\` or \`4\` up to \`8\` or \`12\` ms.',
      },

      {
        id: 'black-screen-matrix',
        title: 'Black screen by capture backend',
        content: 'SolarFlare has six capture paths and each fails differently. Identify yours first, then apply the matching fix:',
        code: {
          language: 'bash',
          code: `getcap "$(command -v sunshine)"
ls -l /dev/dri/card* /dev/dri/render*
echo "$XDG_SESSION_TYPE $WAYLAND_DISPLAY $DISPLAY"
cat /sys/module/nvidia_drm/parameters/modeset
grep -E "^capture =|^adapter_name|^output_name" ~/.config/sunshine/sunshine.conf
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -n 80 --no-pager | grep -iE "kms|capture|portal|wayland|x11|hermes|Probably not permitted"`,
        },
        table: {
          headers: ['Backend', 'Failure signature', 'Fix'],
          rows: [
            ['KMS', 'Probably not permitted on /dev/dri', 'setcap cap_sys_admin,cap_sys_nice+p; user in video group; re-login'],
            ['KMS NVIDIA', 'No modes / modeset N', 'nvidia_drm.modeset=1 kernel param, reboot'],
            ['Wayland', 'Missing output metadata', 'skip_wayland_correlation = true only as last resort (breaks absolute mouse)'],
            ['Portal', 'portal session closed', 'Check xdg-desktop-portal running; re-accept the share dialog'],
            ['X11', 'No displays / wrong screen', 'DISPLAY=:0 set; xrandr --listmonitors; VIRTUAL1 or headless_mode'],
            ['Hermes-KMS', 'Module absent', 'Install headers + DKMS module; look for HERMES-1 source'],
            ['Plasma 6.5+', 'Flicker with overlays', 'KWIN_USE_OVERLAYS=0'],
          ],
        },
      },
      {
        id: 'encoder-triage',
        title: 'Encoder failures by vendor',
        content: 'When logs say no working encoder was found, check the vendor stack directly. HDR fallbacks (hevc/av1 modes 3+) silently downgrade when the chain is not 10-bit end to end.',
        code: {
          language: 'bash',
          code: `vainfo; vainfo --display drm --device /dev/dri/renderD128
nvidia-smi --query-gpu=name,driver_version --format=csv
lspci | grep -E 'VGA|3D'; ls -l /dev/dri/render*
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -n 200 --no-pager | grep -iE "encoder|nvenc|vaapi|qsv|vulkan|sw_preset|not supported"`,
        },
      },
      {
        id: 'pairing-discovery',
        title: 'Pairing & discovery failures',
        content: 'PIN failures are usually time skew, firewall asymmetry, or origin policy — not the PIN itself. Host and client clocks must agree within ~30 s. mDNS needs one broadcast domain with no AP isolation; otherwise add the host by LAN IP.',
        code: {
          language: 'bash',
          code: `timedatectl status
ss -tlnp | grep -E '47984|47990|48010'; ss -ulnp | grep -E '47998|48000'
curl -sk https://127.0.0.1:47990/api/health | jq .
grep -E "trusted_subnets|trusted_subnet_auto_pairing|origin_web_ui_allowed|const_pin|upnp|^port =" ~/.config/sunshine/sunshine.conf
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -n 100 --no-pager | grep -iE "pair|pin|cert|crypto|origin"`,
        },
      },
      {
        id: 'webui-auth',
        title: 'Web UI 401 / 403 / CSRF errors',
        content: '401 means no or bad credentials; 403 means scope, origin policy, or CSRF. Browser cross-origin POSTs need X-CSRF-Token from GET /api/csrf-token (1 h TTL); curl without Origin/Referer is exempt. Ten login failures per IP per 30 s triggers 429. Forgotten passwords reset with sunshine --creds followed by a service restart.',
        code: {
          language: 'bash',
          code: `curl -vk https://localhost:47990/api/config -w "%{http_code}"
CSRF=$(curl -sk -u "$USER:$PASS" https://localhost:47990/api/csrf-token | jq -r .csrf_token)
grep -E "csrf_allowed_origins|origin_web_ui_allowed|api_tokens" ~/.config/sunshine/sunshine.conf
sunshine --creds "$USER" "$PASS"; systemctl --user restart app-dev.lizardbyte.app.Sunshine.service`,
        },
      },
      {
        id: 'log-surfaces',
        title: 'Logs, error counters & telemetry',
        content: 'Error counters (encoder, capture, network, session, process, config, crypto, unknown, total) are monotonic since start — diff the total over 60 s and want zero deltas. min_log_level accepts verbose, debug, info, warning, error, fatal. Never run two instances: stop the user service before a foreground repro.',
        code: {
          language: 'bash',
          code: `curl -sk -H "Authorization: Bearer $TOK" https://localhost:47990/api/errors | jq .
curl -sk -H "Authorization: Bearer $TOK" https://localhost:47990/api/logs | tail -100
SUNSHINE_LOG_JSON=1 systemctl --user restart app-dev.lizardbyte.app.Sunshine.service
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -p err -b -n 100 --no-pager`,
        },
      },
      {
        id: 'update-failures',
        title: 'Self-update failures',
        content: 'Updater phases: idle, checking, downloading, verifying, ready, waiting_idle, applying, restarting, error, unsupported (non-Linux). A non-empty session list blocks apply unless when_idle is false. SHA256SUMS mismatches, missing pkexec/helper, and lost setcap after binary swap are the common causes.',
        code: {
          language: 'bash',
          code: `curl -sk -H "Authorization: Bearer $TOK" https://localhost:47990/api/update | jq '{phase,message,percent,busy,can_apply,latest_tag,outdated}'
curl -sk -H "Authorization: Bearer $TOK" https://localhost:47990/api/sessions | jq .
journalctl --user -u app-dev.lizardbyte.app.Sunshine.service --no-pager | grep -iE "update|staging|SHA-256|tarball|setcap|pkexec|helper"`,
        },
      },
    ],
  },

  'building': {
    slug: 'building',
    title: 'Building from Source',
    category: 'Developer & API',
    badge: 'Build',
    description: 'Compiling SolarFlare, CMake build flags, developer profiles, and running the GoogleTest suite.',
    readTime: '5 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'prerequisites',
        title: 'Build Prerequisites',
        content:
          'SolarFlare requires a C++20 compliant compiler (GCC 13+ or Clang 17+), CMake 3.25+, Ninja, NodeJS 20+, and development headers for PipeWire, Opus, DRM, VA-API, and OpenSSL.',
      },
      {
        id: 'build-steps',
        title: 'Standard Build Commands',
        content: 'Keep build directories under cmake-build-. Switch tabs for host OS prefixes.',
        codeTabs: [
          {
            label: 'Linux',
            language: 'bash',
            code: `cmake -S . -B cmake-build-release -G Ninja \\
  -DCMAKE_BUILD_TYPE=Release \\
  -DBUILD_TESTS=OFF \\
  -DBUILD_DOCS=OFF
cmake --build cmake-build-release --target sunshine web-ui -j2
sudo setcap 'cap_sys_admin,cap_sys_nice+p' cmake-build-release/sunshine`,
          },
          {
            label: 'Windows (MSYS2)',
            language: 'bash',
            code: `C:\\msys64\\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build cmake-build-release --target sunshine -j2"`,
          },
          {
            label: 'Tests',
            language: 'bash',
            code: `cmake -S . -B cmake-build-tests -G Ninja \\
  -DCMAKE_BUILD_TYPE=Debug \\
  -DBUILD_TESTS=ON \\
  -DBUILD_DOCS=OFF
cmake --build cmake-build-tests --target test_sunshine -j2
./cmake-build-tests/tests/test_sunshine --gtest_brief=1`,
          },
        ],
      },
      {
        id: 'running-tests',
        title: 'Running the Test Suite',
        content:
          'SolarFlare uses GoogleTest (\`gtest\`). The test executable \`test_sunshine\` is generated in \`cmake-build-release/tests/\`:',
        code: {
          language: 'bash',
          code: `# Build test target
cmake --build cmake-build-release --target test_sunshine -j$(nproc)

# Run full test suite
./cmake-build-release/tests/test_sunshine --gtest_brief=1`,
        },
      },
      {
        id: 'toolchain',
        title: 'Toolchain requirements',
        content:
          'C++23 with GCC 13+ (14+ recommended; Fedora 45 uses GCC 15) or Clang 17+, CMake 3.20 minimum (the linux_build.sh Docker/CI builder enforces CMake 4.0+ and bootstraps 4.3 when needed), Ninja, Node.js 20+ with npm, Python 3.14+ with uv for Flatpak generators, and Doxygen 1.10–1.12 plus Graphviz for docs builds. Optional accelerators: ccache and mold/lld linkers (auto-preferred), CUDA 12+ for NVENC paths. Ubuntu 22.04 needs the gcc-13 toolchain PPA.',
        table: {
          headers: ['Component', 'Minimum', 'Notes'],
          rows: [
            ['GCC', '13+ (14+ recommended)', 'C++23 <format> needs it; CI uses GCC 14'],
            ['Clang', '17+', 'Linux and FreeBSD'],
            ['CMake', '3.20 (4.0+ for linux_build.sh)', 'Bootstraps 4.3 when distro is old'],
            ['Ninja', 'any recent', 'Recommended generator'],
            ['Node.js + npm', '20+', 'Web UI build'],
            ['Doxygen + Graphviz', '1.10 - 1.12', 'Required when BUILD_DOCS=ON'],
            ['CUDA', '12.0+', 'NVENC / NvFBC paths only'],
          ],
        },
      },
      {
        id: 'cmake-options',
        title: 'CMake options reference',
        content:
          'All options live in cmake/prep/options.cmake. The maintained installer path already picks sane Release defaults; override individual flags for manual or packaging builds.',
        table: {
          headers: ['Option', 'Default', 'Purpose'],
          rows: [
            ['BUILD_TESTS', 'ON', 'Build test_sunshine and enable CTest'],
            ['BUILD_DOCS', 'ON', 'Build Doxygen docs (OFF for fast dev)'],
            ['BUILD_WERROR', 'OFF', 'Treat warnings as errors (CI sets ON)'],
            ['ENABLE_COVERAGE', 'OFF', 'gcov instrumentation for tests'],
            ['NPM_OFFLINE', 'OFF', 'Offline npm cache (Flatpak builds)'],
            ['SUNSHINE_ENABLE_TRAY', 'ON', 'System tray (installer sets OFF)'],
            ['SUNSHINE_ENABLE_CUDA', 'ON', 'NVENC/CUDA paths (installer sets OFF)'],
            ['SUNSHINE_ENABLE_DRM / VAAPI / VULKAN / WAYLAND / X11 / KWIN / PORTAL', 'ON', 'Capture and encode backends'],
            ['SUNSHINE_BUILD_APPIMAGE / FLATPAK', 'OFF', 'Alternate packaging layouts'],
            ['FFMPEG_PREPARED_BINARIES', '(auto)', 'Override path to extracted FFmpeg static libs'],
          ],
        },
      },
      {
        id: 'profiles',
        title: 'Build profiles & test commands',
        content:
          'Keep every local tree under the cmake-build- prefix. Bare build/ is reserved for the linux_build.sh Docker/CI builder. The test binary is always <build-dir>/tests/test_sunshine. Iterate with --gtest_filter, then run the full suite before review; hardware-dependent tests skip without devices, failures must be explained.',
        code: {
          language: 'bash',
          code: `cmake -S . -B cmake-build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_DOCS=OFF
cmake -S . -B cmake-build-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_DOCS=OFF
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF
cmake -S . -B cmake-build-docs -G Ninja -DBUILD_DOCS=ON -DBUILD_TESTS=OFF
cmake --build cmake-build-tests --target test_sunshine -j2
./cmake-build-tests/tests/test_sunshine --gtest_brief=1
./cmake-build-tests/tests/test_sunshine --gtest_filter='ConfigTest.*' --gtest_brief=1
cmake --build cmake-build-release --target sunshine web-ui -j2
clang-format --dry-run --Werror src/path/to/changed.cpp
git diff --check`,
        },
      },
      {
        id: 'cuda-build',
        title: 'CUDA / NVENC builds',
        content:
          'Default installer builds disable CUDA (VA-API/Vulkan cover most Linux users). For NVENC: install a driver plus a CUDA toolkit matching your GCC, then configure with SUNSHINE_ENABLE_CUDA=ON and point CMAKE_CUDA_COMPILER at nvcc. Known-good reference is CUDA 13.1.1 with driver build 590.48.01; Flatpak pins 13.2.0; release notes record CUDA 13.4 builds. Architecture coverage follows the toolkit version (older toolkits cover sm_50–sm_90, newer ones add sm_100+ families). glibc mismatches against NVCC math headers are fixed by the patches under packaging/linux/patches/.',
      },
      {
        id: 'failed-builds',
        title: 'Failed build recovery',
        table: {
          headers: ['Failure', 'Likely cause', 'Recovery'],
          rows: [
            ['Empty third-party/* at configure', 'Submodules not initialized', 'git submodule update --init --recursive'],
            ['FFmpeg release tag unavailable', 'build-deps not at a tag', 'git -C third-party/build-deps fetch --tags'],
            ['No pinned FFmpeg checksum', 'Unsupported arch/OS', 'Build on x86_64/aarch64 Linux, Windows, macOS, or FreeBSD amd64'],
            ['Doxygen target fails', 'Missing doxygen/graphviz', '-DBUILD_DOCS=OFF for code-only builds'],
            ['Link OOM under LTO', 'Low RAM with -flto', '-DSUNSHINE_CACHYOS_NATIVE=OFF or lower -j'],
            ['ninja: no build.ninja', 'Wrong build dir', 'Reuse one -B cmake-build-* path consistently'],
            ['KMS permission denied after install', 'Missing setcap', "sudo setcap 'cap_sys_admin,cap_sys_nice+p' on the binary"],
          ],
        },
      },
    ],
  },

  'gamestream-migration': {
    slug: 'gamestream-migration',
    title: 'GameStream Migration',
    category: 'Getting Started',
    badge: 'Migration',
    description: 'Seamlessly transition from discontinued NVIDIA GameStream to SolarFlare host for Moonlight.',
    readTime: '4 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'migration-overview',
        title: 'Migrating from NVIDIA GameStream',
        content:
          'In February 2023, NVIDIA discontinued GameStream in GeForce Experience. SolarFlare is a modern, actively maintained open-source replacement designed specifically for Moonlight clients.',
      },
      {
        id: 'key-differences',
        title: 'Key Differences & Advantages',
        table: {
          headers: ['Feature', 'NVIDIA GameStream', 'SolarFlare Host'],
          rows: [
            ['Operating System', 'Windows only (NVIDIA GPU)', 'Linux (Primary), Windows, macOS'],
            ['GPU Vendor Support', 'NVIDIA GeForce only', 'AMD (AMDGPU/VA-API), NVIDIA (NVENC), Intel (QuickSync)'],
            ['Capture Pipeline', 'NVFBC proprietary', 'Linux KMS, Wayland DMA-BUF, PipeWire, X11'],
            ['Audio DSP', 'None (unfiltered audio)', 'Automatic Gain Control, Voice Activity Ducking, Noise Gate'],
            ['Per-Client Profiles', 'None (global only)', 'Per-client bitrate, codec, and latency tuning'],
            ['Self-Hosted API', 'None (cloud dependent)', 'Full REST API, Scoped Tokens, Webhooks'],
          ],
        },
      },
      {
        id: 'gsms-tool',
        title: 'Automated Migration with GSMS',
        content:
          'The upstream GSMS (GameStream Migration Sunshine) utility can automatically import existing box-art, commands, and shortcuts into SolarFlare \`apps.json\`.',
      },
      {
        id: 'step-flow',
        title: 'Migration steps',
        content:
          'Disable GameStream in GeForce Experience first (it binds port 47989 and the two cannot coexist), then uninstall or disable GFE. Back up ~/.config/sunshine/{sunshine.conf, apps.json, credentials}. Install with ./scripts/linux-install.sh, apply setcap for KMS capture, open firewall TCP 47984-47990 plus 48010 and UDP 47998-48000, then re-pair each Moonlight client at https://<host>:47990/pin. Clocks must agree within ~30 s and clients re-pin because the server certificate changed. Moonlight itself needs no reinstall — get clients at moonlight-stream.org.',
      },
      {
        id: 'sunshine-to-solarflare',
        title: 'Sunshine to SolarFlare on the same PC',
        content:
          'In place: the binary name (sunshine), ports, systemd service (app-dev.lizardbyte.app.Sunshine.service), and ~/.config/sunshine/ layout are identical, so install over the existing setup and keep your apps.json, credentials, and pairings. Never run both at once. Windows hosts moving to Linux should export their app list first and rebuild launch commands for Linux paths.',
      },
      {
        id: 'client-reuse',
        title: 'Client reuse notes',
        content:
          'Desktop (Qt, recommended, full codec and HDR set), Android/TV (manual host add when mDNS is blocked), iOS/tvOS (HDR needs both ends capable), embedded/Pi (host must be a separate machine), and browser clients (smaller feature surface) all pair the same way. H.264 works everywhere; HEVC and AV1 need encoder and client support on both ends, with per-client overrides via client_profile_* keys.',
      },
    ],
  },

  'changelog': {
    slug: 'changelog',
    title: 'SolarFlare Changelog',
    category: 'Project & Release',
    badge: 'v1.3.0',
    description: 'Chronological release notes, performance upgrades, and upstream compatibility syncs.',
    readTime: '8 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'releases',
        title: 'Release notes',
        content: 'Select a version tab. Full GitHub compare links live on each release.',
        tabs: [
          {
            id: 'eol',
            label: 'End of life',
            content:
              '**SolarFlare reached end of life in September 2026.**\\n\\n- **No further releases, updates, bug fixes, or security patches will be published.**\\n- The repository is archived and read-only.\\n- This documentation site is kept for reference only.\\n- Users are encouraged to migrate to [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine), the upstream project from which SolarFlare was forked, which remains actively maintained.\\n\\nThank you to everyone who used, tested, and contributed to this project.',
          },
          {
            id: 'v130',
            label: 'v1.3.0',
            content:
              '**Build** `v2026.909.1-solarflare`\n\n- **Input seat isolation:** `input_seat` config key routes virtual devices to a systemd-logind seat via udev `ID_SEAT` + `EVIOCGRAB` hardening (Discussion #20)\n- **libudev integration:** CMake `FindUdev` detection, compile-out safe\n- **Fallback udev rule:** Shipped `99-solarflare-seat.rules` for locked-down hosts\n- **Web UI:** New Input Seat field in the Inputs tab\n- **Tests:** 703 passed, 95.9% coverage on changed lines\n- **CUDA:** Built with CUDA 13.4 (architectures 75–121)',
          },
          {
            id: 'v122',
            label: 'v1.2.2',
            content:
              '**Build** `v2026.824.1-solarflare`\n\n- **Docs portal:** Next.js documentation at https://vindeckyy.github.io/Solar-Flare/docs\n- **REST API:** Scoped tokens (`/api/tokens`), game scanner (`/api/games/scan`), live telemetry\n- **NVENC:** Harmonized `nvenc_tuning_preset` latency / balanced / quality profiles\n- **Audio FX:** Documented AGC, VAD, ducking, and noise-gate tunables\n- **Doxygen:** Restored TOC navigation for fork settings',
          },
          {
            id: 'v121',
            label: 'v1.2.1',
            content:
              '**Build** `v2026.809.1-solarflare`\n\n- **Live telemetry:** `GET /api/stream/telemetry` for host CPU, RAM, and GPU\n- **Session history:** `GET /api/sessions` backed by `session_history.jsonl`\n- **Self-updater:** Staged apply with `/api/update` status\n- **Adaptive bitrate:** Client network feedback via `/api/stream/network-stats`',
          },
          {
            id: 'v120',
            label: 'v1.2.0',
            content:
              '**Build** `v2026.807.1-solarflare`\n\n- **Audio FX:** AGC, VAD, ducking, and noise gate before Opus\n- **Opus:** Application mode, VBR, and FEC knobs\n- **Porting:** Arch, Debian, Fedora, and openSUSE install paths\n- **Scheduling:** SCHED_RR capture worker and non-IRQ core pinning',
          },
        ],
      },
      {
        id: 'v1-2-2',
        title: 'SolarFlare v1.2.2 (Build 2026.824.1)',
        content:
          '### Features and improvements\n\n- **Comprehensive docs portal:** Next.js documentation portal at `https://vindeckyy.github.io/Solar-Flare/docs`.\n- **REST API extensions:** Scoped API tokens (`/api/tokens`), game scanner (`/api/games/scan`), and live telemetry endpoints.\n- **NVENC and video:** Harmonized `nvenc_tuning_preset` one-click latency, balanced, and quality profiles.\n- **Audio FX subsystem:** Documented float tunables for AGC, VAD, ducking, and noise gate.\n- **Doxygen layout:** Restored TOC navigation for SolarFlare fork settings.',
      },
      {
        id: 'v1-2-1',
        title: 'SolarFlare v1.2.1 (Build 2026.809.1)',
        content:
          '### Features and improvements\n\n- **Live telemetry:** Added `GET /api/stream/telemetry` for host CPU, RAM, and GPU time series.\n- **Session history:** Added `GET /api/sessions` backed by `session_history.jsonl`.\n- **Self-updater staging:** Staged update workflow with `/api/update` status reporting.\n- **Adaptive bitrate pacing:** Real-time network feedback queue (`/api/stream/network-stats`).',
      },
      {
        id: 'v1-2-0',
        title: 'SolarFlare v1.2.0 (Build 2026.807.1)',
        content:
          '### Features and improvements\n\n- **Audio FX signal chain:** AGC, VAD speech detection, game audio ducking, and noise gate DSP.\n- **Opus low-delay controls:** Application mode, variable bitrate, and forward error correction knobs.\n- **Multi-distro porting:** Build support for openSUSE, Fedora, and Debian.\n- **CPU real-time scheduling:** SCHED_RR capture worker priority and non-IRQ core pinning.',
      },
      {
        id: 'versioning',
        title: 'How versions work',
        content:
          'Two identifiers ride every release. The SemVer display title (for example SolarFlare v1.3.0) is what release pages, badges, and headings show. The chronological build version (for example v2026.909.1-solarflare) is the git tag, the CMake PROJECT_VERSION, the sunshine --version output, and the GET /api/health version that Moonlight and update tooling compare. Upstream Sunshine cherry-picks are logged per release so inherited fixes stay traceable.',
      },
    ],
  },

  'security': {
    slug: 'security',
    title: 'Security Policy & Advisories',
    category: 'Project & Release',
    badge: 'Security',
    description: 'Vulnerability disclosure procedures, security mechanisms, and supported version matrices.',
    readTime: '4 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'supported-versions',
        title: 'Supported Release Versions',
        table: {
          headers: ['Version', 'Supported', 'Patch Cadence'],
          rows: [
            ['Latest 1.3.x release', 'Yes', 'Immediate security patches & hotfixes'],
            ['master branch', 'Yes', 'Continuous rolling security updates'],
            ['Older 1.x releases', 'Best Effort', 'Supported until the next minor release'],
            ['Pre-1.0 tags', 'No', 'Unsupported legacy releases'],
          ],
        },
      },
      {
        id: 'reporting',
        title: 'Reporting a Vulnerability',
        content:
          'If you discover a security vulnerability in SolarFlare, please **do not open a public issue**.\n\nSubmit a confidential report via [GitHub Private Security Advisory](https://github.com/vindeckyy/Solar-Flare/security/advisories/new).\n\nReports are triaged promptly by the maintainer.',
      },
      {
        id: 'threat-model',
        title: 'Security Architecture & Defenses',
        content:
          '- **Scoped API Tokens:** Fine-grained permission model prevents external automation from accessing arbitrary administrative operations.\n- **CSRF Token Validation:** State-changing browser requests require valid \`X-CSRF-Token\` headers.\n- **Signed Webhooks:** Outgoing webhook payloads carry HMAC-SHA256 signatures (\`X-Solarflare-Signature\`).\n- **Encrypted Local Stream:** RTSP and video/audio channels are encrypted with TLS and AES-128-GCM.',
      },
      {
        id: 'hardening',
        title: 'Hardening checklist',
        table: {
          headers: ['Priority', 'Action'],
          rows: [
            ['High', 'Strong Web UI password; never keep defaults'],
            ['High', 'Keep origin_web_ui_allowed at lan or pc'],
            ['High', 'Never expose port 47990 to the Internet without a hardened reverse proxy'],
            ['Medium', 'Scoped Bearer tokens instead of admin Basic in automation'],
            ['Medium', 'webhook_secret set; receivers verify HMAC signatures'],
            ['Medium', 'trusted_subnet_auto_pairing disabled unless strictly needed'],
            ['Medium', 'CA-signed TLS cert when browsers reach the UI remotely'],
            ['Low', 'Prune csrf_allowed_origins and unused paired clients'],
          ],
        },
      },
      {
        id: 'mechanisms',
        title: 'Mechanisms in depth',
        content:
          'The origin gate (pc loopback-only, lan private/local default, wan anywhere) runs before authentication: outsiders get bare HTTP 403. Both listeners (GameStream port default 47989, Web UI port +1) share the configured cert/pkey; Moonlight pins the cert at pairing, so a key change forces re-pairing and a pre-pairing MITM is the residual risk. Passwords are salted SHA-256; API tokens are 64-char hex stored as SHA-256(token:salt). Browsers need X-CSRF-Token on state-changing calls (same-origin and allowlisted origins skip it; curl without Origin/Referer is exempt). Trusted-subnet auto-pairing silently pairs matching CIDRs — keep ranges tight. Webhooks accept https only, retry twice, and sign with X-Solarflare-Signature when a secret is set. The GameStream HTTPS handshake pool is capped at 64 concurrent to blunt slow-handshake DoS. GET /api/browse exposes host paths to the process user, so combine auth, origin policy, and OS permissions.',
      },
      {
        id: 'updates-disclosure',
        title: 'Fixes, versions & disclosure',
        content:
          'No LTS branches and no backports: fixes land on master and ride the next tag. Confirm patch state with both the installed package version and GET /api/health version. Pre-tag fixes: pull master, rebuild or re-run linux-install.sh, re-check /api/health. Monitor both SolarFlare and upstream Sunshine advisories for inherited components (OpenSSL, FFmpeg, libcurl). Reports: affected version/commit, component, reproducer, impact, optional fix — expect acknowledgement in about 7 days, a fix on master, a changelog entry, and a published advisory when impact warrants it.',
      },
    ],
  },

  'contributing': {
    slug: 'contributing',
    title: 'Contributing Guide',
    category: 'Project & Release',
    badge: 'Guidelines',
    description: 'Code standards, Doxygen requirements, test coverage guidelines, and pull request rules.',
    readTime: '5 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'code-standards',
        title: 'C++ Code Standards & Formatting',
        content:
          '- **Formatting:** All C/C++ code must conform to the project \`.clang-format\` definition.\n- **Build Directory Naming:** Prefix all build directories with \`cmake-build-\` (e.g. \`cmake-build-release\`).\n- **Localization:** Update only \`en.json\` (\`src_assets/common/assets/web/public/assets/locale/en.json\`). Do not edit other language variants.',
      },
      {
        id: 'doxygen-rules',
        title: 'Doxygen Documentation Requirements',
        content:
          'All classes, structs, functions, and member variables must have Doxygen comments or the build will fail (\`BUILD_WERROR=ON\`):\n\n- Primary function/struct blocks:\n\`\`\`cpp\n/**\n * @brief Brief summary of the function.\n * @param param_name Parameter description.\n * @return Return value description.\n */\n\`\`\`\n- Inline member variables must use \`///< ...\` format (never \`/**< ... */\`):',
        code: {
          language: 'cpp',
          code: `int latency_mode = 0;  ///< Latency mode: 0 for safe, 1 for aggressive`,
        },
      },
      {
        id: 'tests-rule',
        title: 'Test Coverage & Verification',
        content:
          'Always add unit tests in \`tests/unit/\` for new or modified functionality using GoogleTest (\`gtest\`). Target 100% test coverage on changed code.',
        code: {
          language: 'bash',
          code: `cmake --build cmake-build-release --target test_sunshine -j$(nproc)\n./cmake-build-release/tests/test_sunshine --gtest_brief=1`,
        },
      },
      {
        id: 'upstream-rule',
        title: 'Fork Boundary & Upstream Policy',
        callout: {
          type: 'important',
          text: 'Do not open issues or pull requests in the upstream LizardByte GitHub organization for SolarFlare fork work. Submit all contributions to vindeckyy/Solar-Flare.',
        },
      },
      {
        id: 'workflow',
        title: 'Branch & pull request workflow',
        content:
          'Topic branch from master, one logical change per commit with conventional subjects (feat/fix/docs/test). Rebase when stale. Never commit build trees, credentials, local state, or unrelated submodule bumps. Preserve protocol identifiers, config paths, and service compatibility unless the change ships an explicit migration plan. Copy the 11-item PR checklist from CONTRIBUTING.md into the PR description: problem statement, GoogleTest coverage at 100% of changed code, full test_sunshine pass, Doxygen + BUILD_DOCS=ON green, user docs updated, clang-format clean, diff-check clean, npm run build green for frontend changes, en.json-only locale, no generated output, upstream links where relevant, fork-target PR.',
        code: {
          language: 'bash',
          code: `git submodule update --init --recursive
cmake -S . -B cmake-build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_DOCS=OFF
cmake --build cmake-build-dev --target test_sunshine -j2
./cmake-build-dev/tests/test_sunshine --gtest_filter='ConfigTest.*' --gtest_brief=1
./cmake-build-dev/tests/test_sunshine --gtest_brief=1
clang-format --dry-run --Werror src/path/to/changed.cpp
git diff --check
npm run build`,
        },
      },
      {
        id: 'webui-ci',
        title: 'Web UI, CI & reporting',
        content:
          'The Web UI is a multi-entry Vite app, not a routed SPA: Navbar owns navigation, init.js bootstraps theme/locale per entry, sunshine.css holds the design system, Vue SFCs own interactive config, EJS files own static shells. Keep endpoints and form serialization independent of visuals, animate only state changes, and honor prefers-reduced-motion. Build via the CMake web-ui target (npm run dev for iteration, npm run build before submit) and refresh README screenshots when layout changes. CI runs the Web bundle job plus Linux compile + test_sunshine under Xvfb with coverage upload, using release_version 0.0.0-ci; release binaries never come from CI. Report bugs with repro steps and redacted logs, distro and desktop session, GPU/backend/encoder, Moonlight client version, and source/binary/CI build provenance.',
      },
    ],
  },

  'release-process': {
    slug: 'release-process',
    title: 'Maintainer Release Guide',
    category: 'Project & Release',
    badge: 'Maintainer',
    description: 'Step-by-step SOP for version tagging, artifact compilation, checksumming, and GitHub releases.',
    readTime: '8 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'versioning-rules',
        title: 'Dual Version Identifiers',
        content:
          '- **Release Title:** SemVer (e.g. \`SolarFlare v1.3.0\`).\n- **Compatibility Build Version:** \`v<YYYY>.<MDD>.<rev>-solarflare\` (e.g. \`v2026.909.1-solarflare\`).',
      },
      {
        id: 'artifacts',
        title: 'Release artifacts',
        table: {
          headers: ['Asset', 'Contents', 'Consumer'],
          rows: [
            ['sunshine-x86_64', 'Stripped ELF executable (compatibility name)', 'Manual binary swap'],
            ['solarflare-linux-x86_64.tar.gz', 'Executable, runtime layout, Web UI assets, icon, license', 'Web UI in-app updater'],
            ['SHA256SUMS', 'Checksums for both files above', 'Updater integrity verification'],
          ],
        },
        callout: {
          type: 'caution',
          text: 'GitHub Actions must not build release binaries. Artifacts are produced locally from a clean, tagged tree. Raw GitHub executables cannot retain Linux capabilities — instruct users to run setcap after download for KMS capture.',
        },
      },
      {
        id: 'prerequisites',
        title: 'Prerequisites and prepare',
        content:
          'Requires a clean tree, master synced with origin/master, git identity, gh authenticated to vindeckyy/Solar-Flare, a Linux x86-64 toolchain, and Web UI + tests verified locally. Before tagging: sync branch, choose next display + build versions, verify quality gates, confirm git status is empty, and draft release-notes.md with exact asset filenames plus the update-only CAUTION callout.',
        code: {
          language: 'bash',
          code: 'git checkout master\ngit pull origin master\ngit status --short\ncmake --build cmake-build-release-prep --target test_sunshine -j2\n./cmake-build-release-prep/tests/test_sunshine --gtest_brief=1',
        },
      },
      {
        id: 'tagging-steps',
        title: 'Release Workflow Commands',
        content: 'Use --dry-run first. Push only after local verification. The script updates CMakeLists.txt, pyproject.toml, uv.lock, README metadata, prepends the website changelog entry, commits, tags, and optionally pushes.',
        tabs: [
          {
            id: 'dry',
            label: 'Dry run',
            code: {
              language: 'bash',
              code: './scripts/release.sh 2026.909.1 1.3.0 --dry-run',
            },
          },
          {
            id: 'tag',
            label: 'Tag locally',
            code: {
              language: 'bash',
              code: './scripts/release.sh 2026.909.1 1.3.0 --no-push',
            },
          },
          {
            id: 'publish',
            label: 'Publish',
            code: {
              language: 'bash',
              code: `gh release create v2026.909.1-solarflare \\
  sunshine-x86_64 \\
  solarflare-linux-x86_64.tar.gz \\
  SHA256SUMS \\
  --repo vindeckyy/Solar-Flare \\
  --verify-tag \\
  --latest \\
  --title 'SolarFlare v1.3.0' \\
  --notes-file release-notes.md`,
            },
          },
        ],
      },
      {
        id: 'verify',
        title: 'Build, verify, and publish',
        content:
          'Build from the final tagged commit with embedded identity (BRANCH, BUILD_VERSION, COMMIT), strip the executable into release-artifacts/, assemble the tarball mirroring the previous layout, generate SHA256SUMS, verify with sha256sum -c, push commit + tag only after verification, then gh release create with --verify-tag --latest --title --notes-file. Post-publish: confirm non-draft with all three assets, download into a clean directory, re-verify checksums, smoke-test --version, and optionally trigger in-app update on staging.',
        code: {
          language: 'bash',
          code: 'export BRANCH=master BUILD_VERSION=2026.909.1 COMMIT=$(git rev-parse HEAD)\ncmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF\ncmake --build cmake-build-release --target sunshine web-ui -j2\nmkdir -p release-artifacts\nstrip -o release-artifacts/sunshine-x86_64 cmake-build-release/sunshine\ncd release-artifacts\nsha256sum sunshine-x86_64 solarflare-linux-x86_64.tar.gz > SHA256SUMS\nsha256sum -c SHA256SUMS',
        },
      },
      {
        id: 'rollback',
        title: 'Rollback, hotfix, and notes template',
        content:
          'Prefer forward fixes: land the fix on master with tests, cut a new build version (never reuse a published one), publish new assets. Do not force-push tags users may have downloaded. Release notes must include the update-only CAUTION, an Assets table, setcap recovery snippet, Changes list, and a full compare link.',
      },
    ],
  },

  docker: {
    slug: 'docker',
    title: 'Docker and containers',
    category: 'Operations',
    badge: 'Ops',
    description:
      'SolarFlare does not publish a container image. This page covers upstream Sunshine images and experimental compose.',
    readTime: '6 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'policy',
        title: 'Supported path',
        content:
          'Build SolarFlare on the host with ./scripts/linux-install.sh. Upstream lizardbyte/sunshine images do not include fork tunables, the SolarFlare Web UI, or API token features.',
        callout: {
          type: 'caution',
          text: 'KMS capture, SCHED_RR pinning, and GPU governors often fail inside namespaces. Expect portal or X11 capture in containers.',
        },
      },
      {
        id: 'when-containers',
        title: 'When containers make sense',
        table: {
          headers: ['Scenario', 'Recommendation'],
          rows: [
            ['Quick upstream Sunshine smoke test', 'Upstream image OK with caveats below'],
            ['Production SolarFlare host on Linux', 'Source install — not Docker'],
            ['Homelab media server with GPU passthrough', 'Custom Dockerfile possible; expect manual tuning'],
            ['Kubernetes / orchestrated gaming', 'See Games on Whales; not SolarFlare-maintained'],
            ['CI compile-only builds', 'Use scripts/linux_build.sh Docker builder, not runtime image'],
          ],
        },
      },
      {
        id: 'image-tags',
        title: 'Image tags',
        content:
          'Container tags combine a version channel and OS suffix. Bare tags such as `latest`, `master`, or `vX.X.X` are not complete image tags. Always use `<SUNSHINE_VERSION>-<SUNSHINE_OS>`, for example `latest-ubuntu-24.04`. Browse tags on [Docker Hub](https://hub.docker.com/r/lizardbyte/sunshine/tags) and [GHCR](https://github.com/LizardByte/Sunshine/pkgs/container/sunshine/versions).',
      },
      {
        id: 'run',
        title: 'Runtime examples',
        content: 'Tags combine version and OS suffix, for example latest-ubuntu-24.04. Internal Web UI port stays 47990.',
        tabs: [
          {
            id: 'compose',
            label: 'Compose',
            code: {
              language: 'yaml',
              code: `services:
  sunshine:
    image: lizardbyte/sunshine:latest-ubuntu-24.04
    ipc: host
    devices:
      - /dev/dri/
    ports:
      - "47984-47990:47984-47990/tcp"
      - "47998-48000:47998-48000/udp"
    volumes:
      - ./sunshine-config:/config`,
            },
          },
          {
            id: 'docker',
            label: 'docker run',
            code: {
              language: 'bash',
              code: `docker run -d --name sunshine --ipc=host \\
  --device /dev/dri/ \\
  -v /home/user/sunshine-config:/config \\
  -p 47984-47990:47984-47990/tcp \\
  -p 47998-48000:47998-48000/udp \\
  lizardbyte/sunshine:latest-ubuntu-24.04`,
            },
          },
          {
            id: 'podman',
            label: 'Podman',
            content: 'Rootless Podman may not expose uinput or SYS_ADMIN. Software encode is the realistic fallback.',
            code: {
              language: 'bash',
              code: `podman run -d --name sunshine --userns=keep-id \\
  --device /dev/dri/ \\
  -v /home/user/sunshine-config:/config \\
  -p 47984-47990:47984-47990/tcp \\
  lizardbyte/sunshine:latest-ubuntu-24.04`,
            },
          },
        ],
      },
      {
        id: 'volumes-ports',
        title: 'Ports, volumes, and PUID/PGID',
        content:
          'Persist pairing certificates, `sunshine.conf`, and `apps.json` with `-v /path/on/host/sunshine-config:/config` (native installs use `~/.config/sunshine/`). Required ports: TCP 47984–47990 plus 48010, UDP 47998–48000. Set `PUID`/`PGID` to match the host config owner; `chown -R` the volume if you change IDs after first run. `amd64` and `arm64` manifests exist per OS suffix, but SolarFlare release binaries are Linux x86-64 only.',
      },
      {
        id: 'failures',
        title: 'Common failure modes',
        table: {
          headers: ['Symptom', 'Likely cause', 'Mitigation'],
          rows: [
            ['Black screen in Moonlight', 'No GPU in container', 'Pass /dev/dri, validate host driver'],
            ['Web UI unreachable', 'Wrong port map', 'Map host port to container 47990'],
            ['Pairing lost on recreate', 'Ephemeral /config', 'Persist volume mount'],
            ['Encoder Function not implemented', 'VA-API/NVENC not visible', 'Driver-matched image; check vainfo/nvidia-smi'],
            ['Input not working', 'Missing uinput', '--device /dev/uinput, correct groups'],
            ['High latency vs native', 'No fork tunables in upstream image', 'Build SolarFlare from source on host'],
          ],
        },
      },
    ],
  },

  guides: {
    slug: 'guides',
    title: 'Operator Guides',
    category: 'Getting Started',
    badge: 'How-to',
    description: 'Curated LAN baseline, low-latency, headless, multi-GPU, HDR, per-client, webhook, and logging how-tos.',
    readTime: '9 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'lan-baseline',
        title: 'LAN streaming baseline',
        content:
          'Goal: stable 1080p120 or 4K60 on a wired LAN with default fork settings. Install with `./scripts/linux-install.sh` and enable the user service. Wire the host with Ethernet. Leave fork defaults enabled (`cpu_pinning`, `enet_4mib_buffer`, `busy_poll_us`, `dscp_qos`). In Moonlight set bitrate to ~80% of measured iPerf throughput. Run `iperf3` per the Troubleshooting network test.',
        callout: {
          type: 'tip',
          text: 'If the host NIC is 2.5 GbE but the client is 1 GbE, set `rate_cap_pct = 80` or lower to avoid buffer overruns.',
        },
      },
      {
        id: 'competitive',
        title: 'Competitive low-latency profile',
        content: 'Add to `~/.config/sunshine/sunshine.conf`, plus per-game NVENC override in `apps.json`. Disable V-Sync in-game; cap Moonlight bitrate only if packet loss appears.',
        code: {
          language: 'ini',
          code: 'latency_mode = aggressive\nbusy_poll_us = 75\npipewire_latency_ms = 4\ncpu_pinning = true\nnvenc_tuning_preset = 0',
        },
        callout: {
          type: 'warning',
          text: '`latency_mode = aggressive` trades some visual quality on software scaling paths. Test before using in single-player titles.',
        },
      },
      {
        id: 'headless-ssh',
        title: 'Headless and SSH access',
        content:
          'Path 1 — virtual display (fork): `headless_virtual_display = true` with `headless_width/headless_height/headless_refresh`. Path 2 — dummy plug for NVIDIA stable modes with a normal graphical session. Path 3 — SSH into an existing X11 session: `ssh user@host \'export DISPLAY=:0; sunshine\'`.',
        callout: {
          type: 'caution',
          text: 'Do not run two `sunshine` instances. Stop the systemd user service before foreground debugging.',
        },
      },
      {
        id: 'multi-gpu',
        title: 'Multi-GPU workstations',
        table: {
          headers: ['Step', 'Action'],
          rows: [
            ['1', "Identify GPUs: lspci | grep -E 'VGA|3D' and nvidia-smi"],
            ['2', 'Plug the monitor (or dummy) into the GPU you want to capture'],
            ['3', 'Launch games on that GPU (DRI_PRIME=1, prime-run, or BIOS mux)'],
            ['4', 'In Web UI, set display adapter / output if multiple heads are visible'],
            ['5', 'Enable gpu_governor = true for AMD; install redesign nvidia-clock-lock for NVIDIA'],
          ],
        },
      },
      {
        id: 'hdr',
        title: 'HDR on Linux (experimental)',
        content:
          'Requires KMS capture, an HDR compositor (KDE Plasma 6, Gamescope), HEVC Main 10 or AV1 10-bit encoder (VAAPI on AMD/Intel), HDR enabled in host OS and Moonlight client, and an EDID emulator or HDR-capable display on host.',
      },
      {
        id: 'per-client',
        title: 'Per-client household profiles',
        content: 'Use `client_profile_<identifier>` in `sunshine.conf`. The identifier matches the paired client name from the Web UI PIN / clients list.',
        code: {
          language: 'ini',
          code: 'client_profile_living-room-tv = {"max_bitrate": 150000, "hevc_mode": 1}\nclient_profile_phone = {"max_bitrate": 20000, "width": 1280, "height": 720}',
        },
      },
      {
        id: 'webhooks-logging',
        title: 'Webhooks and structured logging',
        content:
          'Notify Home Assistant or Discord on stream start/stop with `webhook_secret` + `webhook_url_0`, verifying `X-Solarflare-Signature`. For observability: `SUNSHINE_LOG_JSON=1 systemctl --user restart app-dev.lizardbyte.app.Sunshine.service`, then `journalctl --user -u app-dev.lizardbyte.app.Sunshine.service -f`.',
        code: {
          language: 'ini',
          code: 'webhook_secret = your-hmac-secret\nwebhook_url_0 = https://example.com/hooks/solarflare',
        },
      },
      {
        id: 'platform-support',
        title: 'Platform support',
        content:
          'Linux x86-64 is primary via `./scripts/linux-install.sh`. Other-arch Linux may build but is not release-tested. Windows / macOS / FreeBSD paths install upstream Sunshine unless you build this repository yourself on that platform.',
        table: {
          headers: ['Platform', 'SolarFlare support'],
          rows: [
            ['Linux x86-64', 'Primary; ./scripts/linux-install.sh'],
            ['Linux (other arch)', 'Source may build; not release-tested'],
            ['Windows / macOS', 'Inherited code; use upstream Sunshine releases'],
            ['FreeBSD', 'Inherited upstream packages only'],
          ],
        },
      },
      {
        id: 'builder-vs-runtime',
        title: 'Builder vs runtime images',
        content:
          'scripts/linux_build.sh is a compile-only Docker builder, not a streaming runtime. It validates packaging metadata (appstreamcli, desktop-file-validate) and can emit AppImage artifacts. Runtime containers use the upstream lizardbyte/sunshine images with --ipc=host, /dev/dri (and /dev/uinput for input), the full TCP/UDP port set, and a persisted /config volume. Set PUID/PGID to the config owner and chown -R the volume if IDs change later. Multi-arch manifests cover amd64 and arm64, but SolarFlare release binaries are x86-64 only — arm64 hosts must build from source. Point orchestrator health checks at GET /api/health.',
      },
      {
        id: 'moonlight-clients',
        title: 'Moonlight client picks',
        table: {
          headers: ['Client', 'Platforms', 'Notes'],
          rows: [
            ['Moonlight Desktop (Qt)', 'Windows, macOS, Linux', 'Recommended; full codec and HDR set'],
            ['Moonlight Android', 'Android, Android TV', 'Manual host add when mDNS is blocked'],
            ['Moonlight iOS / tvOS', 'iPhone, iPad, Apple TV', 'HDR needs host + client capable'],
            ['Moonlight Embedded', 'Raspberry Pi, embedded Linux', 'Host must be a separate machine'],
            ['Moonlight Web', 'Chrome, Edge', 'Browser client; smaller feature surface'],
          ],
        },
      },
    ],
  },

  'third-party-packages': {
    slug: 'third-party-packages',
    title: 'Dependencies & Third-Party Packages',
    category: 'Developer & API',
    badge: 'Deps',
    description: 'Bundled submodules, FFmpeg prebuilt pins, Flatpak versions, and community-package warnings.',
    readTime: '6 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'submodules',
        title: 'Bundled git submodules',
        content:
          'Initialize before any CMake configure with `git submodule update --init --recursive`. Key paths: `third-party/moonlight-common-c`, `Simple-Web-Server`, `libdisplaydevice`, `tray`, `glad`, `nv-codec-headers`, `nanors`, `wlr-protocols`, `wayland-protocols`, `doxyconfig`, `build-deps`, `lizardbyte-common`, `hermes-kms`, plus optional `googletest`, `inputtino`, `nvapi`, `plasma-wayland-protocols`, `TPCircularBuffer`, `ViGEmClient`. If a directory under `third-party/` is empty, re-run the init for that path; shallow CI clones may need `git fetch --tags` inside `build-deps`.',
        code: {
          language: 'bash',
          code: 'git submodule update --init --recursive\ngit -C third-party/build-deps describe --tags --exact-match',
        },
      },
      {
        id: 'ffmpeg',
        title: 'FFmpeg prebuilt binaries',
        content:
          'SolarFlare links static FFmpeg libraries from LizardByte/build-deps releases. The tag is read from the `third-party/build-deps` submodule via `cmake/dependencies/ffmpeg.cmake`; checksums are authoritative in `ffmpeg.cmake`. Override with `-DFFMPEG_PREPARED_BINARIES=/path/to/extracted/ffmpeg`. Flatpak may pin a different release in `packaging/linux/flatpak/modules/ffmpeg.json` and builds with `-DFFMPEG_PREPARED_BINARIES=/app/ffmpeg`.',
      },
      {
        id: 'node-flatpak',
        title: 'Node.js and Flatpak pins',
        content:
          'Run `npm install --no-audit --no-fund` before CMake (locks in `package-lock.json`; Vue 3.5, Vite 6, Bootstrap 5). Flatpak uses the Node 20 SDK extension and offline sources via `flatpak-node-generator`. Build-time pins live in `packaging/linux/flatpak/modules/`: CUDA, FFmpeg prebuilt, Boost 1.89.0, nlohmann_json 3.11.3, miniupnpc 2.3.3, KDE Platform 6.10, Freedesktop SDK 25.08.',
      },
      {
        id: 'community',
        title: 'Community packages (upstream only)',
        content:
          'Chocolatey, Scoop, nixpkgs, Solus, and Flathub links install third-party builds of upstream LizardByte Sunshine — not SolarFlare. They do not include the fork Web UI, performance controls, or maintainer support. For SolarFlare use `scripts/linux-install.sh` or GitHub release assets. See `packaging/linux/flatpak/README.md` for a local Flatpak build; published Flathub builds are not SolarFlare.',
        callout: {
          type: 'warning',
          text: 'Community packages install upstream Sunshine, not SolarFlare. Verify the package source before reporting fork issues.',
        },
      },
    ],
  },

  legal: {
    slug: 'legal',
    title: 'Legal & Licensing',
    category: 'Project & Release',
    badge: 'GPL-3.0',
    description: 'GPL-3.0 operator summary, trademarks, codec patents, redistribution checklist, and privacy.',
    readTime: '5 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'license',
        title: 'License',
        content:
          'SolarFlare is distributed under GNU General Public License v3.0 (GPL-3.0-only). The tree contains work derived from LizardByte/Sunshine and other third-party components, each retaining its own copyright notices where applicable. This page is informational only, not legal advice.',
        callout: {
          type: 'caution',
          text: 'Consult a qualified attorney for questions about using, modifying, or distributing SolarFlare in your jurisdiction.',
        },
      },
      {
        id: 'relationship',
        title: 'SolarFlare and Sunshine relationship',
        table: {
          headers: ['Aspect', 'Detail'],
          rows: [
            ['Copyright', 'SolarFlare fork holders per LICENSE and commit history'],
            ['Upstream', 'Substantial code lineage from Sunshine (GPL-3.0)'],
            ['Trademarks', '"SolarFlare" is the fork product name; "Sunshine" and "Moonlight" are third-party marks'],
            ['Compatibility', 'Binary sunshine, ~/.config/sunshine, service IDs retained for compatibility — not a trademark grant'],
          ],
        },
      },
      {
        id: 'gpl',
        title: 'GPL-3.0 summary for operators',
        content:
          'Permitted: private runs without distribution, personal modifications, commercial use of the software itself. Required when you convey the program: provide corresponding source or a written offer, include license and copyright notices, document changes in modified files, and license derivatives under GPL-3.0. Read the full LICENSE text.',
      },
      {
        id: 'redistribution',
        title: 'Redistribution checklist',
        content:
          '1. Source offer matching the binary (or exact tag link). 2. Include LICENSE and preserve notices. 3. Aggregate third-party licenses from third-party/ into NOTICE where required. 4. Do not imply LizardByte, NVIDIA, or Moonlight endorsement. 5. Label forks clearly. 6. Assess H.264/HEVC distribution rules in target regions. Verify SHA256SUMS on repackaged release artifacts.',
      },
      {
        id: 'privacy',
        title: 'Privacy and data handling',
        content:
          'Self-hosted: no mandatory cloud telemetry in the streaming path. Pairing certs and config stay on the host (`~/.config/sunshine/`). Optional `webhook_url_*` sends events you configure to third-party URLs. API tokens are stored hashed; protect `sunshine.conf` permissions. Operators own GDPR, logging, and LAN exposure of the HTTPS Web UI (default 47990). Report vulnerabilities privately via GitHub Security Advisories.',
      },
    ],
  },

  ecosystem: {
    slug: 'ecosystem',
    title: 'Ecosystem & Upstream',
    category: 'Project & Release',
    badge: 'Upstream',
    description: 'Awesome-Sunshine catalog, upstream changelog feed, and SolarFlare vs upstream history.',
    readTime: '4 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'overview',
        title: 'SolarFlare vs upstream history',
        table: {
          headers: ['Document', 'Contents'],
          rows: [
            ['Upstream feed (below)', 'LizardByte/Sunshine releases: capture, encode, protocol, cross-platform hosts'],
            ['Changelog (/docs/changelog)', 'Fork-only features, Linux tuning, Web UI, API scopes'],
            ['Maintainer handbook (/docs/maintainers)', 'How SolarFlare versions and tags are cut'],
          ],
        },
        callout: {
          type: 'note',
          text: 'When triaging a bug, check whether it reproduces on upstream Sunshine. Fork-specific issues (Web UI, solarflare_t keys, pacing) belong to SolarFlare.',
        },
      },
      {
        id: 'awesome',
        title: 'Upstream Sunshine ecosystem',
        content:
          'SolarFlare stays compatible with Moonlight and much of the Sunshine ecosystem. The independent LizardByte/awesome-sunshine catalog is not maintained or endorsed by SolarFlare and may assume upstream packages, Docker images, or UI behavior. Install SolarFlare via the Quickstart; use fork tunables from Configuration; get clients at moonlight-stream.org. Test community tools (companion apps, scripts, themes) targeting the Moonlight protocol before production use.',
      },
      {
        id: 'feeds',
        title: 'Live feeds',
        content:
          'Upstream changelog: https://raw.githubusercontent.com/LizardByte/Sunshine/changelog/CHANGELOG.md. Ecosystem catalog: https://raw.githubusercontent.com/LizardByte/awesome-sunshine/master/README.md. Both render live on the Doxygen site; the website docs tab curates the SolarFlare-relevant subset above.',
      },

      {
        id: 'links-triage',
        title: 'Links and bug triage',
        content:
          'Clients: [Moonlight](https://moonlight-stream.org/). Ecosystem catalog: [awesome-sunshine](https://github.com/LizardByte/awesome-sunshine). Upstream releases: [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine). Orchestrated stacks: [Games on Whales](https://games-on-whales.github.io). Triage rule: reproduces on stock Sunshine without fork keys, Web UI, or pacing involved — report to LizardByte and link it. Fork-specific (tunables, Web UI, API scopes, pacing) — report to vindeckyy/Solar-Flare. Never open fork PRs or issues in the LizardByte organization.',
      },
    ],
  },

  maintainers: {
    slug: 'maintainers',
    title: 'Maintainer Handbook',
    category: 'Project & Release',
    badge: 'Maintainer',
    description: 'Triage duties, dual versioning, artifact contracts, CI scope, and new-maintainer handoff.',
    readTime: '6 min read',
    lastUpdated: 'September 2026',
    sections: [
      {
        id: 'duties',
        title: 'Maintainer responsibilities',
        content:
          '1. Triage issues and PRs against fork scope. 2. Enforce test coverage, Doxygen, and clang-format on merged C++ changes. 3. Cut releases locally per the Release Guide — GitHub Actions does not produce release binaries. 4. Keep dual versioning synchronized across CMake, Python metadata, README, and Git tags. 5. Never publish releases, issues, or PRs under the LizardByte organization for SolarFlare work.',
      },
      {
        id: 'versioning',
        title: 'Dual versioning',
        content:
          'Display version (SemVer, e.g. 1.3.0): GitHub release title, README badge, user-facing copy. Build version (chronological YYYY.MDD.REVISION, e.g. 2026.909.1): CMakeLists.txt, pyproject.toml, uv.lock, embedded sunshine --version, compatibility tag v<build>-solarflare. The executable reports the build version; release titles show the display version.',
        table: {
          headers: ['Identifier', 'Example', 'Where it appears'],
          rows: [
            ['Display version (SemVer)', '1.3.0', 'GitHub release title, README badge, changelog headings'],
            ['Build version (chronological)', '2026.909.1', 'CMake PROJECT_VERSION, Python package, --version, git tag'],
          ],
        },
      },
      {
        id: 'artifacts',
        title: 'Release artifacts',
        table: {
          headers: ['Asset', 'Purpose'],
          rows: [
            ['sunshine-x86_64', 'Stripped executable (compatibility filename) for in-place binary updates'],
            ['solarflare-linux-x86_64.tar.gz', 'Executable + runtime/Web UI assets + icon + license (Web UI updater path)'],
            ['SHA256SUMS', 'SHA-256 checksums for both payloads'],
          ],
        },
        callout: {
          type: 'caution',
          text: 'Release binaries are for updating an existing SolarFlare install only. New users must build from source with scripts/linux-install.sh.',
        },
      },
      {
        id: 'ci',
        title: 'Continuous integration',
        content:
          'Fork CI: Web bundle (npm ci + npm run build) and Linux build and tests (linux_build.sh, Xvfb, test_sunshine, gcovr upload). Not run by default: macOS, Windows, FreeBSD, Arch, Homebrew, Flatpak, Copr, upstream release publishing. Those workflows remain inherited but depend on LizardByte-only secrets. CI builds use release_version 0.0.0-ci for health checks, not release fidelity.',
      },
      {
        id: 'handoff',
        title: 'Handoff checklist',
        content:
          'Read CONTRIBUTING and AGENTS. Build from source with scripts/linux-install.sh. Run test_sunshine locally with BUILD_TESTS=ON. Walk through the Release Guide with --dry-run and --no-push. Confirm gh auth for vindeckyy/Solar-Flare. Review the Security Policy disclosure path.',
      },

      {
        id: 'version-sync',
        title: 'Version sync checklist',
        content:
          'Every release touches the same set: CMakeLists.txt project version, pyproject.toml and uv.lock package versions, README badge plus release and build-tag rows, the website changelog tabs in website/lib/docs-data.ts, then commit and tag v<build>-solarflare with message SolarFlare v<display>. The binary reports the build version, never the display title. Script it with ./scripts/release.sh <build> <display> --dry-run first, then --no-push, verify artifacts, and push only when green.',
      },
    ],
  },
}
