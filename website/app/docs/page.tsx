import Link from 'next/link'
import {
  Rocket,
  Sliders,
  Zap,
  Code2,
  Shield,
  ArrowRight,
  BookOpen,
  Terminal,
  Cpu,
  Flame,
  Radio,
  FileCode,
} from 'lucide-react'
import { DOC_CATEGORIES, DOC_ARTICLES } from '@/lib/docs-data'

export const metadata = {
  title: 'Documentation | SolarFlare Game-Streaming Host',
  description:
    'Comprehensive guides, configuration tunables, Linux optimization, REST API references, and architecture documentation for SolarFlare.',
}

export default function DocsPortalPage() {
  return (
    <div className="space-y-12">
      {/* Portal Hero */}
      <div className="space-y-4 border-b border-border pb-10">
        <div className="flex items-center gap-2">
          <span className="rounded-md bg-primary/10 border border-primary/20 px-2.5 py-1 font-mono text-xs font-semibold text-primary uppercase tracking-wider">
            Official Documentation
          </span>
          <span className="text-xs font-mono text-muted-foreground">v1.2.2 • Linux Host</span>
        </div>

        <h1 className="text-3xl sm:text-5xl font-bold tracking-tight text-foreground">
          SolarFlare Documentation
        </h1>

        <p className="text-base sm:text-xl text-muted-foreground max-w-3xl leading-relaxed">
          Everything you need to install, configure, optimize, and automate SolarFlare on your Linux game-streaming host.
        </p>

        {/* Quickstart Action Bar */}
        <div className="flex flex-wrap items-center gap-3 pt-2">
          <Link
            href="/docs/getting-started"
            className="flex items-center gap-2 rounded-lg bg-primary px-4 py-2 text-sm font-semibold text-primary-foreground hover:bg-primary/90 transition-colors"
          >
            <Rocket className="h-4 w-4" />
            Quickstart Installation
          </Link>
          <Link
            href="/docs/configuration"
            className="flex items-center gap-2 rounded-lg border border-border bg-card/60 px-4 py-2 text-sm font-medium text-foreground hover:border-primary/50 transition-colors"
          >
            <Sliders className="h-4 w-4 text-primary" />
            Configuration Tunables
          </Link>
          <Link
            href="/docs/api"
            className="flex items-center gap-2 rounded-lg border border-border bg-card/60 px-4 py-2 text-sm font-medium text-foreground hover:border-primary/50 transition-colors"
          >
            <Code2 className="h-4 w-4 text-primary" />
            REST API Reference
          </Link>
        </div>
      </div>

      {/* Featured Topic Cards */}
      <div className="grid gap-6 sm:grid-cols-2 lg:grid-cols-3">
        <Link
          href="/docs/getting-started"
          className="group relative flex flex-col justify-between rounded-xl border border-border bg-card/40 p-6 hover:border-primary/50 transition-all hover:shadow-lg"
        >
          <div className="space-y-3">
            <div className="w-fit rounded-lg bg-primary/10 p-2.5 text-primary border border-primary/20">
              <Terminal className="h-5 w-5" />
            </div>
            <h2 className="text-lg font-semibold text-foreground group-hover:text-primary transition-colors">
              Install on Linux
            </h2>
            <p className="text-sm text-muted-foreground leading-relaxed">
              Step-by-step setup using the automated installer, systemd user units, KMS capture, and Moonlight pairing.
            </p>
          </div>
          <div className="flex items-center gap-1 text-xs font-mono font-medium text-primary mt-4">
            <span>Read guide</span>
            <ArrowRight className="h-3.5 w-3.5 group-hover:translate-x-1 transition-transform" />
          </div>
        </Link>

        <Link
          href="/docs/configuration"
          className="group relative flex flex-col justify-between rounded-xl border border-border bg-card/40 p-6 hover:border-primary/50 transition-all hover:shadow-lg"
        >
          <div className="space-y-3">
            <div className="w-fit rounded-lg bg-primary/10 p-2.5 text-primary border border-primary/20">
              <Sliders className="h-5 w-5" />
            </div>
            <h2 className="text-lg font-semibold text-foreground group-hover:text-primary transition-colors">
              11 Host Tunables & Audio FX
            </h2>
            <p className="text-sm text-muted-foreground leading-relaxed">
              Full reference for network pacer limits, real-time CPU pinning, AGC, VAD ducking, and Opus audio modes.
            </p>
          </div>
          <div className="flex items-center gap-1 text-xs font-mono font-medium text-primary mt-4">
            <span>Explore tunables</span>
            <ArrowRight className="h-3.5 w-3.5 group-hover:translate-x-1 transition-transform" />
          </div>
        </Link>

        <Link
          href="/docs/performance-tuning"
          className="group relative flex flex-col justify-between rounded-xl border border-border bg-card/40 p-6 hover:border-primary/50 transition-all hover:shadow-lg"
        >
          <div className="space-y-3">
            <div className="w-fit rounded-lg bg-primary/10 p-2.5 text-primary border border-primary/20">
              <Zap className="h-5 w-5" />
            </div>
            <h2 className="text-lg font-semibold text-foreground group-hover:text-primary transition-colors">
              Low-Latency Tuning
            </h2>
            <p className="text-sm text-muted-foreground leading-relaxed">
              Eliminate frame drops and audio latency with SCHED_RR priority, GPU power governors, and BBR pacing.
            </p>
          </div>
          <div className="flex items-center gap-1 text-xs font-mono font-medium text-primary mt-4">
            <span>Optimize latency</span>
            <ArrowRight className="h-3.5 w-3.5 group-hover:translate-x-1 transition-transform" />
          </div>
        </Link>
      </div>

      {/* Comprehensive Category Directory */}
      <div className="space-y-8 pt-4">
        <div className="border-b border-border pb-4">
          <h2 className="text-2xl font-bold tracking-tight text-foreground">Documentation Directory</h2>
          <p className="text-sm text-muted-foreground mt-1">
            Browse all articles by category or search with <kbd className="rounded border border-border bg-muted px-1.5 py-0.5 text-xs font-mono">⌘K</kbd>.
          </p>
        </div>

        <div className="grid gap-8 md:grid-cols-2">
          {DOC_CATEGORIES.map((cat) => (
            <div key={cat.name} className="rounded-xl border border-border bg-card/30 p-6 space-y-4">
              <div className="flex items-center justify-between">
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
                          <span className="rounded bg-muted px-1.5 py-0.2 text-[10px] font-mono text-muted-foreground">
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
