import Image from 'next/image'
import { ArrowRight, GitFork, Terminal } from 'lucide-react'
import { Button } from '@/components/ui/button'

const REPO = 'https://github.com/vindeckyy/Solar-Flare'

const SPECS = [
  { k: 'Client protocol', v: 'Moonlight / GameStream' },
  { k: 'Control plane', v: 'https://localhost:47990' },
  { k: 'Host focus', v: 'Linux x86-64 · AMD-first' },
  { k: 'Current release', v: 'v2026.718.5' },
]

export function Hero() {
  return (
    <section id="top" className="relative overflow-hidden">
      {/* corona backdrop */}
      <div className="pointer-events-none absolute inset-0" aria-hidden="true">
        <Image
          src={`${process.env.NEXT_PUBLIC_BASE_PATH || ''}/solar-corona.png`}
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
          A precision fork of LizardByte / Sunshine
        </div>

        <h1 className="mt-6 max-w-3xl text-balance text-4xl font-semibold leading-[1.05] tracking-tight text-foreground md:text-6xl">
          The game-streaming host, rebuilt as an{' '}
          <span className="text-primary">instrument panel</span>.
        </h1>

        <p className="mt-6 max-w-2xl text-pretty text-lg leading-relaxed text-muted-foreground">
          SolarFlare is a self-hosted streaming server for Moonlight clients. It
          pairs a low-latency Linux data path with an observatory-style Web UI
          for pairing devices, managing apps, tuning the host, and diagnosing
          the full pipeline.
        </p>

        <div className="mt-8 flex flex-wrap items-center gap-3">
          <Button
            size="lg"
            nativeButton={false}
            className="h-11 w-full gap-2 px-5 text-sm font-medium sm:w-auto"
            render={<a href="#install" />}
          >
            <Terminal className="h-4 w-4" aria-hidden="true" />
            Install SolarFlare
          </Button>
          <Button
            size="lg"
            variant="outline"
            nativeButton={false}
            className="h-11 w-full gap-2 border-border bg-card/40 px-5 text-sm font-medium backdrop-blur sm:w-auto"
            render={<a href={REPO} target="_blank" rel="noreferrer" />}
          >
            <GitFork className="h-4 w-4" aria-hidden="true" />
            View source
            <ArrowRight className="h-4 w-4" aria-hidden="true" />
          </Button>
        </div>

        <p className="mt-6 font-mono text-sm italic text-primary/90">
          Own the host. Instrument the path. Stream without the cloud.
        </p>

        {/* spec strip */}
        <dl className="mt-14 grid max-w-3xl grid-cols-2 gap-px overflow-hidden rounded-xl border border-border bg-border md:grid-cols-4">
          {SPECS.map((s) => (
            <div key={s.k} className="bg-card/80 p-4 backdrop-blur">
              <dt className="font-mono text-[11px] uppercase tracking-wider text-muted-foreground">
                {s.k}
              </dt>
              <dd className="mt-1 break-words text-sm font-medium text-foreground">
                {s.v}
              </dd>
            </div>
          ))}
        </dl>
      </div>
    </section>
  )
}
