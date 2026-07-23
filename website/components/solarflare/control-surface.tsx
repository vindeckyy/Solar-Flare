'use client'

import Image from 'next/image'
import { useState } from 'react'
import {
  KeyRound,
  LayoutGrid,
  Radar,
  SlidersHorizontal,
  Activity,
  Gauge,
  Wifi,
  Cpu,
  ShieldCheck,
} from 'lucide-react'
import { cn } from '@/lib/utils'

const TABS = [
  {
    id: 'dashboard',
    label: 'Monitor the host',
    icon: Gauge,
    short: 'Dashboard',
    desc: 'Connection state, release status, and direct actions on a magnetic-field host dashboard.',
        image: 'web-ui-home.png',
  },
  {
    id: 'pair',
    label: 'Pair a client',
    icon: KeyRound,
    short: 'Pairing',
    desc: 'Focused PIN entry with clear host state and trusted-subnet rules.',
        image: 'web-ui-pin.png',
  },
  {
    id: 'apps',
    label: 'Manage applications',
    icon: LayoutGrid,
    short: 'Apps',
    desc: 'Launch definitions, artwork, per-app encoder presets, and import tools.',
        image: 'web-ui-applications.png',
  },
  {
    id: 'clients',
    label: 'Discover clients',
    icon: Radar,
    short: 'Clients',
    desc: 'A curated local catalog with no third-party runtime fetch.',
        image: 'web-ui-featured.png',
  },
  {
    id: 'tune',
    label: 'Tune the host',
    icon: SlidersHorizontal,
    short: 'Config',
    desc: 'Dense configuration surfaces with a consistent hierarchy.',
        image: 'web-ui-configuration.png',
  },
  {
    id: 'diag',
    label: 'Inspect the pipeline',
    icon: Activity,
    short: 'Diagnostics',
    desc: 'Logs, diagnostics, and recovery actions in one place.',
        image: 'web-ui-troubleshooting.png',
  },
]

const METRICS = [
  { icon: Wifi, label: 'Link', value: '2.5 GbE', sub: 'rate_cap 80%' },
  { icon: Gauge, label: 'Encode', value: 'NVENC · latency', sub: 'two-pass off' },
  { icon: Cpu, label: 'Capture', value: 'KMS · pinned', sub: 'SCHED_RR' },
  { icon: ShieldCheck, label: 'Access', value: 'Scoped tokens', sub: 'subnet paired' },
]

export function ControlSurface() {
  const [active, setActive] = useState('dashboard')
  const activeTab = TABS.find((t) => t.id === active)!

  return (
    <section id="interface" className="relative border-t border-border py-20 md:py-28">
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="max-w-2xl">
          <p className="font-mono text-xs uppercase tracking-wider text-primary">
            Control surface
          </p>
          <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
            A host instrument panel, not a settings page
          </h2>
          <p className="mt-4 text-pretty leading-relaxed text-muted-foreground">
            Motion is reserved for interaction and state changes — there are no
            ambient looping effects. Select a surface to see what it does.
          </p>
        </div>

        {/* mock window */}
        <div className="mt-12 overflow-hidden rounded-2xl border border-border bg-card shadow-2xl shadow-black/40">
          {/* window bar */}
          <div className="flex items-center gap-2 border-b border-border bg-background/60 px-4 py-3">
            <span className="h-3 w-3 rounded-full bg-destructive/70" />
            <span className="h-3 w-3 rounded-full bg-primary/70" />
            <span className="h-3 w-3 rounded-full bg-muted-foreground/40" />
            <span className="ml-3 font-mono text-xs text-muted-foreground">
              https://localhost:47990
            </span>
          </div>

          <div className="grid md:grid-cols-[220px_1fr]">
            {/* nav rail */}
            <nav className="flex gap-1 overflow-x-auto border-b border-border p-3 md:flex-col md:overflow-visible md:border-b-0 md:border-r">
              {TABS.map((t) => (
                <button
                  key={t.id}
                  onClick={() => setActive(t.id)}
                  className={cn(
                    'flex shrink-0 items-center gap-2.5 rounded-lg px-3 py-2 text-left text-sm transition-colors',
                    active === t.id
                      ? 'bg-primary/15 text-foreground'
                      : 'text-muted-foreground hover:bg-accent/40 hover:text-foreground',
                  )}
                  aria-pressed={active === t.id}
                >
                  <t.icon
                    className={cn(
                      'h-4 w-4 shrink-0',
                      active === t.id ? 'text-primary' : '',
                    )}
                    aria-hidden="true"
                  />
                  <span className="whitespace-nowrap md:whitespace-normal">
                    {t.short}
                  </span>
                </button>
              ))}
            </nav>

            {/* panel */}
            <div className="min-h-[340px] p-6">
              <div className="flex items-center gap-2">
                <activeTab.icon className="h-5 w-5 text-primary" aria-hidden="true" />
                <h3 className="text-lg font-semibold text-foreground">
                  {activeTab.label}
                </h3>
              </div>
              <div className="relative mt-5 aspect-[16/10] overflow-hidden rounded-xl border border-border bg-background">
                    <Image
                      src={`${process.env.NEXT_PUBLIC_BASE_PATH || ''}/ui/${activeTab.image}`}
                      alt={`SolarFlare ${activeTab.label} web interface`}
                      fill
                      sizes="(max-width: 768px) 100vw, 70vw"
                      className="object-cover object-top"
                    />
                  </div>
                  <p className="mt-4 max-w-md text-sm leading-relaxed text-muted-foreground">
                {activeTab.desc}
              </p>

              <div className="mt-6 grid gap-3 sm:grid-cols-2">
                {METRICS.map((m) => (
                  <div
                    key={m.label}
                    className="rounded-xl border border-border bg-background/60 p-4"
                  >
                    <div className="flex items-center gap-2 text-muted-foreground">
                      <m.icon className="h-4 w-4 text-primary" aria-hidden="true" />
                      <span className="font-mono text-[11px] uppercase tracking-wider">
                        {m.label}
                      </span>
                    </div>
                    <div className="mt-2 text-sm font-semibold text-foreground">
                      {m.value}
                    </div>
                    <div className="font-mono text-xs text-muted-foreground">
                      {m.sub}
                    </div>
                  </div>
                ))}
              </div>

              <div className="mt-4 flex flex-wrap items-center justify-between gap-2 rounded-xl border border-border bg-background/60 px-4 py-3">
                <div className="flex items-center gap-2">
                  <span className="relative flex h-2 w-2">
                    <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-primary opacity-60" />
                    <span className="relative inline-flex h-2 w-2 rounded-full bg-primary" />
                  </span>
                  <span className="text-sm text-foreground">Host online</span>
                </div>
                <span className="max-w-full truncate font-mono text-xs text-muted-foreground sm:max-w-none">
                  v2026.718.5-solarflare
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  )
}
