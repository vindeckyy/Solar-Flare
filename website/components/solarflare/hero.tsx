import Image from 'next/image'
import { ArrowRight, GitFork, Terminal } from 'lucide-react'
import { Button } from '@/components/ui/button'

const REPO = 'https://github.com/vindeckyy/Solar-Flare'

const SPECS = [
  { k: 'Client protocol', v: 'Moonlight / GameStream' },
  { k: 'Control plane', v: 'https://localhost:47990' },
  { k: 'Host focus', v: 'Linux x86-64, AMD-first' },
  { k: 'Release', v: 'View latest release', href: `${REPO}/releases/latest` },
]

export function Hero() {
  return (
    <section id="top" className="relative overflow-hidden">
      {/* corona backdrop */}
      <div className="pointer-events-none absolute inset-0" aria-hidden="true">
        <Image
          src="/solar-corona.png"
          alt=""
          fill
          priority
          className="object-cover object-right opacity-40 [mask-image:radial-gradient(120%_120%_at_80%_20%,black,transparent_70%)]"
        />
        <div className="absolute inset-0 grid-lines opacity-40" />
        <div className="absolute inset-0 bg-gradient-to-r from-background via-background/85 to-transparent" />
        <div className="absolute inset-x-0 bottom-0 h-32 bg-gradient-to-t from-background to-transparent" />
      </div>

      <div className="relative mx-auto max-w-6xl px-4 pb-20 pt-20 md:px-6 md:pb-28 md:pt-28">
        <div className="inline-flex items-center gap-2 rounded-full border border-border bg-card/60 px-3 py-1 font-mono text-xs text-muted-foreground backdrop-blur">
          <span className="h-1.5 w-1.5 rounded-full bg-primary" />
          A fork of LizardByte / Sunshine
        </div>

        <h1 className="mt-6 max-w-3xl text-balance text-4xl font-semibold leading-[1.05] tracking-tight text-foreground md:text-6xl">
          A game-streaming host with an{' '}
          <span className="text-primary">instrument panel</span>.
        </h1>

        <p className="mt-6 max-w-2xl text-pretty text-lg leading-relaxed text-muted-foreground">
          SolarFlare is a self-hosted streaming server for Moonlight clients. Low
          latency Linux capture and transport, plus an observatory-style Web UI
          for pairing, apps, host tuning, and pipeline diagnostics.
        </p>

        <div className="mt-8 flex flex-wrap items-center gap-3">
          <Button
            size="lg"
            nativeButton={false}
            className="h-11 gap-2 px-5 text-sm font-medium"
            render={<a href="#install" />}
          >
            <Terminal className="h-4 w-4" aria-hidden="true" />
            Install SolarFlare
          </Button>
          <Button
            size="lg"
            variant="outline"
            nativeButton={false}
            className="h-11 gap-2 border-border bg-card/40 px-5 text-sm font-medium backdrop-blur"
            render={<a href={REPO} target="_blank" rel="noreferrer" />}
          >
            <GitFork className="h-4 w-4" aria-hidden="true" />
            View source
            <ArrowRight className="h-4 w-4" aria-hidden="true" />
          </Button>
        </div>

        <p className="mt-6 font-mono text-sm italic text-primary/90">
          Self-hosted streaming on your LAN. No cloud in the path.
        </p>

        {/* spec strip */}
        <dl className="mt-14 grid max-w-3xl grid-cols-2 gap-px overflow-hidden rounded-xl border border-border bg-border md:grid-cols-4">
          {SPECS.map((s) => (
            <div key={s.k} className="bg-card/80 p-4 backdrop-blur">
              <dt className="font-mono text-[11px] uppercase tracking-wider text-muted-foreground">
                {s.k}
              </dt>
              <dd className="mt-1 text-sm font-medium text-foreground">
                {'href' in s ? (
                  <a
                    href={s.href}
                    target="_blank"
                    rel="noreferrer"
                    className="hover:text-primary"
                  >
                    {s.v}
                  </a>
                ) : (
                  s.v
                )}
              </dd>
            </div>
          ))}
        </dl>

        {/* live UI preview */}
        <div className="mt-14 overflow-hidden rounded-2xl border border-border bg-card shadow-2xl shadow-black/50">
          <div className="flex items-center gap-2 border-b border-border bg-background/60 px-4 py-3">
            <span className="h-3 w-3 rounded-full bg-destructive/70" />
            <span className="h-3 w-3 rounded-full bg-primary/70" />
            <span className="h-3 w-3 rounded-full bg-muted-foreground/40" />
            <span className="ml-3 truncate font-mono text-xs text-muted-foreground">
              https://localhost:47990 - Host Status &amp; Telemetry
            </span>
          </div>
          <div className="relative aspect-[1440/891] w-full bg-background">
            <Image
              src={`${process.env.NEXT_PUBLIC_BASE_PATH || ''}/sf-web-ui-home.png`}
              alt="SolarFlare observatory Web UI home screen with sidebar navigation and host status controls."
              fill
              priority
              sizes="(max-width: 768px) 100vw, 1152px"
              className="object-cover object-top"
            />
          </div>
        </div>
      </div>
    </section>
  )
}
