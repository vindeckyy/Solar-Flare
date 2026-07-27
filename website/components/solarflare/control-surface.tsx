'use client'

import { useState } from 'react'
import Image from 'next/image'
import {
  House,
  KeyRound,
  LayoutGrid,
  Star,
  SlidersHorizontal,
  Activity,
} from 'lucide-react'
import { cn } from '@/lib/utils'

const TABS = [
  {
    id: 'home',
    short: 'Home',
    label: 'Monitor the host',
    desc: 'The home page shows host status, version, and platform, with direct links for pairing a device or managing applications.',
    image: '/sf-web-ui-home.png',
    alt: 'SolarFlare home screen showing host status and platform, with Pair a device and Open applications actions.',
  },
  {
    id: 'pin',
    short: 'Pair',
    label: 'Pair a client',
    desc: 'Enter the PIN shown by Moonlight and confirm which device you are pairing. The page warns that paired clients can control the host.',
    image: '/sf-web-ui-pin.png',
    alt: 'SolarFlare PIN pairing screen with PIN and device name fields, a Send button, and a warning about granting client control of the host.',
  },
  {
    id: 'apps',
    short: 'Applications',
    label: 'Manage applications',
    desc: 'Launch definitions with artwork, sort and search, add-new tools, and per-application editing for commands, working directories, and encoder overrides.',
    image: '/sf-web-ui-applications.png',
    alt: 'SolarFlare applications page listing Desktop, Steam Big Picture, Gamescope Session, and RetroArch as cards with edit and delete controls.',
  },
  {
    id: 'featured',
    short: 'Featured',
    label: 'Discover clients',
    desc: 'A local catalog of Moonlight clients and tools (no third-party runtime fetch), with platform badges and direct download and source links.',
    image: '/sf-web-ui-featured.png',
    alt: 'SolarFlare featured apps page showing Moonlight PC, Moonlight Mobile, and Moonlight Embedded clients with platform badges and get and source buttons.',
  },
  {
    id: 'configuration',
    short: 'Configuration',
    label: 'Tune the host',
    desc: 'Search or browse settings for general host behavior, headless streaming, input, audio/video, networking, and encoders.',
    image: '/sf-web-ui-configuration.png',
    alt: 'SolarFlare configuration page with a search bar and a category sidebar (General, Headless Stream, Input, Audio/Video, Network, encoders) beside general host settings.',
  },
  {
    id: 'troubleshooting',
    short: 'Troubleshooting',
    label: 'Inspect the pipeline',
    desc: 'Logs, diagnostics, and recovery actions together: force-close a stuck app, restart the host, or unpair devices without leaving the console.',
    image: '/sf-web-ui-troubleshooting.png',
    alt: 'SolarFlare troubleshooting page with Force Close, Restart SolarFlare, and Unpair Devices recovery actions.',
  },
] as const

const TAB_ICONS = {
  home: House,
  pin: KeyRound,
  apps: LayoutGrid,
  featured: Star,
  configuration: SlidersHorizontal,
  troubleshooting: Activity,
} as const

export function ControlSurface() {
  const [active, setActive] = useState<(typeof TABS)[number]['id']>('home')
  const activeTab = TABS.find((t) => t.id === active)!

  return (
    <section id="interface" className="relative border-t border-border py-20 md:py-28">
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="max-w-2xl">
          <p className="font-mono text-xs uppercase tracking-wider text-primary">
            Web interface
          </p>
          <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
            Manage the host from one page
          </h2>
          <p className="mt-4 text-pretty leading-relaxed text-muted-foreground">
            The Web UI runs on the host at{' '}
            <code className="rounded bg-muted px-1.5 py-0.5 font-mono text-sm text-foreground">
              https://localhost:47990
            </code>
            . It supports keyboard navigation and adapts to desktop and mobile
            screens. Switch tabs to see each page.
          </p>
        </div>

        {/* tabs */}
        <div className="mt-10 flex flex-wrap gap-2">
          {TABS.map((t) => {
            const Icon = TAB_ICONS[t.id]
            return (
              <button
                key={t.id}
                onClick={() => setActive(t.id)}
                className={cn(
                  'flex items-center gap-2 rounded-full border px-4 py-2 text-sm font-medium transition-colors',
                  active === t.id
                    ? 'border-primary/50 bg-primary/15 text-foreground'
                    : 'border-border text-muted-foreground hover:border-border hover:bg-accent/40 hover:text-foreground',
                )}
                aria-pressed={active === t.id}
              >
                <Icon
                  className={cn('h-4 w-4', active === t.id && 'text-primary')}
                  aria-hidden="true"
                />
                {t.short}
              </button>
            )
          })}
        </div>

        {/* mock window */}
        <div className="mt-6 overflow-hidden rounded-2xl border border-border bg-card shadow-2xl shadow-black/40">
          {/* window bar */}
          <div className="flex items-center gap-2 border-b border-border bg-background/60 px-4 py-3">
            <span className="h-3 w-3 rounded-full bg-destructive/70" />
            <span className="h-3 w-3 rounded-full bg-primary/70" />
            <span className="h-3 w-3 rounded-full bg-muted-foreground/40" />
            <span className="ml-3 truncate font-mono text-xs text-muted-foreground">
              https://localhost:47990 - {activeTab.short}
            </span>
          </div>

          {/* screenshot */}
          <div className="relative aspect-[1440/891] w-full bg-background">
            <Image
              key={activeTab.id}
              src={`${process.env.NEXT_PUBLIC_BASE_PATH || ''}${activeTab.image || '/placeholder.svg'}`}
              alt={activeTab.alt}
              fill
              sizes="(max-width: 768px) 100vw, 1152px"
              className="object-cover object-top"
              priority
            />
          </div>
        </div>

        {/* caption */}
        <p className="mt-4 max-w-2xl text-pretty text-sm leading-relaxed text-muted-foreground">
          <span className="font-medium text-foreground">{activeTab.label}.</span>{' '}
          {activeTab.desc}
        </p>
      </div>
    </section>
  )
}
