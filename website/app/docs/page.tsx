import Link from 'next/link'
import {
  Rocket,
  Sliders,
  Zap,
  Code2,
  ArrowRight,
  Terminal,
  BookOpen,
  Boxes,
  Shield,
} from 'lucide-react'
import { DOC_CATEGORIES } from '@/lib/docs-data'

export const metadata = {
  title: 'Documentation | SolarFlare Game-Streaming Host',
  description:
    'Comprehensive guides, configuration tunables, Linux optimization, REST API references, and architecture documentation for SolarFlare.',
}

const HIGHLIGHTS = [
  {
    href: '/docs/getting-started',
    icon: Terminal,
    title: 'Install on Linux',
    body: 'Step-by-step setup using the automated installer, systemd user units, KMS capture, and Moonlight pairing.',
    cta: 'Read guide',
  },
  {
    href: '/docs/configuration',
    icon: Sliders,
    title: 'Host tunables and Audio FX',
    body: 'Network pacer, CPU pinning, AGC, VAD ducking, Opus modes, webhooks, and per-client profiles.',
    cta: 'Explore tunables',
  },
  {
    href: '/docs/performance-tuning',
    icon: Zap,
    title: 'Low-latency tuning',
    body: 'SCHED_RR capture, GPU governors, PipeWire quantum, DSCP marking, and ready-made profiles.',
    cta: 'Optimize latency',
  },
  {
    href: '/docs/api',
    icon: Code2,
    title: 'REST API',
    body: 'Scoped tokens, stream telemetry, updater, game scanner, and curl examples for every route.',
    cta: 'Open reference',
  },
  {
    href: '/docs/troubleshooting',
    icon: BookOpen,
    title: 'Diagnostics',
    body: 'Black screen, no audio, pairing loops, encoder failures, and a structured log-first workflow.',
    cta: 'Fix issues',
  },
  {
    href: '/docs/docker',
    icon: Boxes,
    title: 'Containers',
    body: 'Why SolarFlare does not ship an image, upstream Sunshine caveats, and experimental compose.',
    cta: 'Container notes',
  },
]

export default function DocsPortalPage() {
  return (
    <div className="space-y-12">
      <div className="relative overflow-hidden rounded-2xl border border-border bg-card/40 p-8 sm:p-10">
        <div
          className="pointer-events-none absolute inset-0"
          style={{
            background:
              'radial-gradient(700px circle at 10% 0%, color-mix(in oklch, var(--primary) 16%, transparent), transparent 55%)',
          }}
        />
        <div className="relative space-y-4">
          <div className="flex flex-wrap items-center gap-2">
            <span className="rounded-md bg-primary/10 border border-primary/20 px-2.5 py-1 font-mono text-xs font-semibold text-primary uppercase tracking-wider">
              Official Documentation
            </span>
            <span className="text-xs font-mono text-muted-foreground">v1.2.2 · Linux Host</span>
          </div>

          <h1 className="text-3xl sm:text-5xl font-bold tracking-tight text-foreground text-balance">
            SolarFlare Documentation
          </h1>

          <p className="text-base sm:text-xl text-muted-foreground max-w-3xl leading-relaxed">
            Install, pair, tune, and automate a Linux Moonlight host. Use the category tabs in the header, the sidebar, or search with ⌘K.
          </p>

          <div className="flex flex-wrap items-center gap-3 pt-2">
            <Link
              href="/docs/getting-started"
              className="flex items-center gap-2 rounded-lg bg-primary px-4 py-2 text-sm font-semibold text-primary-foreground hover:bg-primary/90 transition-colors"
            >
              <Rocket className="h-4 w-4" />
              Quickstart
            </Link>
            <Link
              href="/docs/configuration"
              className="flex items-center gap-2 rounded-lg border border-border bg-background/40 px-4 py-2 text-sm font-medium text-foreground hover:border-primary/50 transition-colors"
            >
              <Sliders className="h-4 w-4 text-primary" />
              Configuration
            </Link>
            <Link
              href="/docs/api"
              className="flex items-center gap-2 rounded-lg border border-border bg-background/40 px-4 py-2 text-sm font-medium text-foreground hover:border-primary/50 transition-colors"
            >
              <Code2 className="h-4 w-4 text-primary" />
              REST API
            </Link>
            <Link
              href="/docs/security"
              className="flex items-center gap-2 rounded-lg border border-border bg-background/40 px-4 py-2 text-sm font-medium text-foreground hover:border-primary/50 transition-colors"
            >
              <Shield className="h-4 w-4 text-primary" />
              Security
            </Link>
          </div>
        </div>
      </div>

      <div className="grid gap-5 sm:grid-cols-2 xl:grid-cols-3">
        {HIGHLIGHTS.map((card) => {
          const Icon = card.icon
          return (
            <Link
              key={card.href}
              href={card.href}
              className="group relative flex flex-col justify-between rounded-xl border border-border bg-card/40 p-6 hover:border-primary/50 transition-all hover:shadow-lg hover:shadow-primary/5"
            >
              <div className="space-y-3">
                <div className="w-fit rounded-lg bg-primary/10 p-2.5 text-primary border border-primary/20">
                  <Icon className="h-5 w-5" />
                </div>
                <h2 className="text-lg font-semibold text-foreground group-hover:text-primary transition-colors">
                  {card.title}
                </h2>
                <p className="text-sm text-muted-foreground leading-relaxed">{card.body}</p>
              </div>
              <div className="flex items-center gap-1 text-xs font-mono font-medium text-primary mt-4">
                <span>{card.cta}</span>
                <ArrowRight className="h-3.5 w-3.5 group-hover:translate-x-1 transition-transform" />
              </div>
            </Link>
          )
        })}
      </div>

      <div className="space-y-8 pt-2">
        <div className="border-b border-border pb-4">
          <h2 className="text-2xl font-bold tracking-tight text-foreground">Documentation Directory</h2>
          <p className="text-sm text-muted-foreground mt-1">
            Browse every article, or search with{' '}
            <kbd className="rounded border border-border bg-muted px-1.5 py-0.5 text-xs font-mono">⌘K</kbd>.
          </p>
        </div>

        <div className="grid gap-6 md:grid-cols-2">
          {DOC_CATEGORIES.map((cat) => (
            <div key={cat.name} className="rounded-xl border border-border bg-card/30 p-6 space-y-4">
              <div className="flex items-center justify-between gap-3">
                <h3 className="text-base font-semibold text-foreground">{cat.name}</h3>
                <span className="text-xs font-mono text-muted-foreground">
                  {cat.items.length} {cat.items.length === 1 ? 'article' : 'articles'}
                </span>
              </div>
              <p className="text-xs text-muted-foreground">{cat.description}</p>

              <div className="divide-y divide-border/60">
                {cat.items.map((item) => (
                  <Link
                    key={item.slug}
                    href={`/docs/${item.slug}`}
                    className="group flex items-center justify-between py-2.5 hover:text-primary transition-colors"
                  >
                    <div className="space-y-0.5 pr-2">
                      <div className="flex items-center gap-2">
                        <span className="text-sm font-medium text-foreground group-hover:text-primary transition-colors">
                          {item.title}
                        </span>
                        {item.badge && (
                          <span className="rounded bg-muted px-1.5 py-0.5 text-[10px] font-mono text-muted-foreground">
                            {item.badge}
                          </span>
                        )}
                      </div>
                      <p className="text-xs text-muted-foreground line-clamp-1">{item.description}</p>
                    </div>
                    <ArrowRight className="h-4 w-4 text-muted-foreground opacity-0 group-hover:opacity-100 group-hover:translate-x-0.5 transition-all shrink-0" />
                  </Link>
                ))}
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}
