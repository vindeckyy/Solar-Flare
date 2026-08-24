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
        badge: 'v1.2.2',
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
    ],
  },
]

export const DOC_ARTICLES: Record<string, DocArticle> = {
  'getting-started': {
    slug: 'getting-started',
    title: 'Installation & Quickstart',
    category: 'Getting Started',
    badge: 'v1.2.2',
    description: 'Install SolarFlare on your Linux host, configure permissions, and pair Moonlight streaming clients.',
    readTime: '6 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'overview',
        title: 'Overview',
        content:
          'SolarFlare is a low-latency self-hosted game-streaming server for Moonlight clients. It runs on Linux hosts with native KMS/Wayland capture, hardware-accelerated NVENC, VA-API, and Vulkan encoding, and fine-grained host tunables.',
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
      },
      {
        id: 'pairing',
        title: 'Pairing Moonlight Clients',
        content:
          '1. Open the Moonlight client on your client device (PC, phone, tablet, Apple TV, Steam Deck).\n2. Select your host computer from the list or enter your host LAN IP address.\n3. Moonlight will display a 4-digit PIN.\n4. Open the SolarFlare Web UI at https://localhost:47990/pin, enter the PIN, and click Pair.',
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
      },
      {
        id: 'host-tunables',
        title: 'Fork Host Tunables',
        content:
          'Switch tabs to browse network, scheduling, capture, and access keys. Full prose lives in docs/CONFIGURATION.md.',
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
          text: 'Inherited upstream keys are in docs/configuration.md. Fork keys are in docs/CONFIGURATION.md.',
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
                responseBody: `{\n  "status": true,\n  "adaptive_bitrate_enabled": true,\n  "adaptive_bitrate_min_kbps": 5000,\n  "adaptive_bitrate_max_kbps": 60000\n}`,
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
            responseBody: `{\n  "status": "ok",\n  "status_code": 200,\n  "version": "2026.824.1",\n  "uptime": 3600\n}`,
          },
        ],
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
              'Use the repository Nix shell and the declarative host settings in docs/PORTING.md. Do not mix apt/dnf packages.',
          },
        ],
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
    ],
  },

  'changelog': {
    slug: 'changelog',
    title: 'SolarFlare Changelog',
    category: 'Project & Release',
    badge: 'v1.2.2',
    description: 'Chronological release notes, performance upgrades, and upstream compatibility syncs.',
    readTime: '8 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'v1-2-2',
        title: 'SolarFlare v1.2.2 (Build 2026.824.1)',
        content:
          '### Features & Improvements\n- **Comprehensive Docs Portal:** Introduced Next.js documentation portal at \`https://vindeckyy.github.io/Solar-Flare/docs\`.\n- **REST API Extensions:** Documented Scoped API Tokens (\`/api/tokens\`), Game Scanner (\`/api/games/scan\`), and live telemetry endpoints.\n- **NVENC & Video Synchronization:** Harmonized \`nvenc_tuning_preset\` one-click latency/balanced/quality profiles across configuration files.\n- **Audio FX Subsystem:** Documented all 10 float sub-tunables for AGC, VAD, Ducking, and Noise Gate.\n- **Doxygen Layout:** Restored full TOC navigation enclosure for SolarFlare fork settings.',
      },
      {
        id: 'v1-2-1',
        title: 'SolarFlare v1.2.1 (Build 2026.809.1)',
        content:
          '### Features & Improvements\n- **Live Telemetry:** Added \`GET /api/stream/telemetry\` for host CPU/RAM/GPU time series.\n- **Session History:** Added \`GET /api/sessions\` queryable endpoint backed by \`session_history.jsonl\`.\n- **Self-Updater Staging:** Implemented staged update workflow with \`/api/update\` status reporting.\n- **Adaptive Bitrate Pacing:** Added real-time network feedback queue (\`/api/stream/network-stats\`).',
      },
      {
        id: 'v1-2-0',
        title: 'SolarFlare v1.2.0 (Build 2026.807.1)',
        content:
          '### Features & Improvements\n- **Audio FX Signal Chain:** Added AGC, VAD speech detection, game audio ducking, and noise gate DSP.\n- **Opus Low-Delay Controls:** Added application mode, variable bitrate (VBR), and forward error correction (FEC) knobs.\n- **Multi-Distro Porting:** Added comprehensive build support for openSUSE, Fedora, and Debian.\n- **CPU Real-Time Scheduling:** Added SCHED_RR capture worker priority and non-IRQ core pinning.',
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
            ['Latest 1.2.x release', 'Yes', 'Immediate security patches & hotfixes'],
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
    ],
  },

  'release-process': {
    slug: 'release-process',
    title: 'Maintainer Release Guide',
    category: 'Project & Release',
    badge: 'Maintainer',
    description: 'Step-by-step SOP for version tagging, artifact compilation, checksumming, and GitHub releases.',
    readTime: '4 min read',
    lastUpdated: 'August 2026',
    sections: [
      {
        id: 'versioning-rules',
        title: 'Dual Version Identifiers',
        content:
          '- **Release Title:** SemVer (e.g. \`SolarFlare v1.2.2\`).\n- **Compatibility Build Version:** \`v<YYYY>.<MDD>.<rev>-solarflare\` (e.g. \`v2026.824.1-solarflare\`).',
      },
      {
        id: 'tagging-steps',
        title: 'Release Workflow Commands',
        content: 'Use --dry-run first. Push only after local verification.',
        tabs: [
          {
            id: 'dry',
            label: 'Dry run',
            code: {
              language: 'bash',
              code: './scripts/release.sh 2026.824.1 1.2.2 --dry-run',
            },
          },
          {
            id: 'tag',
            label: 'Tag locally',
            code: {
              language: 'bash',
              code: './scripts/release.sh 2026.824.1 1.2.2 --no-push',
            },
          },
          {
            id: 'publish',
            label: 'Publish',
            code: {
              language: 'bash',
              code: `gh release create v2026.824.1-solarflare \\
  sunshine-x86_64 \\
  solarflare-linux-x86_64.tar.gz \\
  SHA256SUMS \\
  --repo vindeckyy/Solar-Flare \\
  --verify-tag \\
  --latest \\
  --title 'SolarFlare v1.2.2' \\
  --notes-file release-notes.md`,
            },
          },
        ],
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
    lastUpdated: 'August 2026',
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
    ],
  },
}
