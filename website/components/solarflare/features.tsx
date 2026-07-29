import {
  Gauge,
  Network,
  Cpu,
  MonitorPlay,
  AudioLines,
  ShieldCheck,
} from 'lucide-react'

const FEATURES = [
  {
    icon: MonitorPlay,
    title: 'Host control',
    body: 'A responsive Web UI with command search, host status, and troubleshooting tools.',
  },
  {
    icon: Network,
    title: 'Network path',
    body: 'Link-aware pacing, optional busy polling, expanded 4 MiB ENet buffers, DSCP QoS tagging, and adaptive bitrate controls.',
  },
  {
    icon: Cpu,
    title: 'Scheduling',
    body: 'Capture-thread affinity, controlled real-time scheduling, native CPU microarch tuning, and optional boot-time performance services.',
  },
  {
    icon: Gauge,
    title: 'Video',
    body: 'NVENC tuning profiles, per-application encoder overrides, headless display paths, and hardware-aware capture selection.',
  },
  {
    icon: AudioLines,
    title: 'Audio',
    body: 'Low-latency PipeWire hints plus optional AGC, voice activity detection, ducking, noise gating, and full Opus encoder control.',
  },
  {
    icon: ShieldCheck,
    title: 'Operations',
    body: 'Scoped API tokens, trusted-subnet pairing, a local Moonlight client catalog, and structured tagged logs.',
  },
]

export function Features() {
  return (
    <section id="features" className="relative border-t border-border py-20 md:py-28">
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="max-w-2xl">
          <p className="font-mono text-xs uppercase tracking-wider text-primary">
            Fork additions
          </p>
          <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
            Each subsystem has its own controls
          </h2>
          <p className="mt-4 text-pretty leading-relaxed text-muted-foreground">
            Defaults stay compatible with upstream. Disable any tuning path when
            you want a cleaner comparison on a given host.
          </p>
        </div>

        <div className="mt-12 grid gap-px overflow-hidden rounded-2xl border border-border bg-border sm:grid-cols-2 lg:grid-cols-3">
          {FEATURES.map((f) => (
            <div
              key={f.title}
              className="group bg-card p-6 transition-colors hover:bg-accent/40"
            >
              <div className="flex h-10 w-10 items-center justify-center rounded-lg border border-border bg-background text-primary">
                <f.icon className="h-5 w-5" aria-hidden="true" />
              </div>
              <h3 className="mt-4 text-base font-semibold text-foreground">
                {f.title}
              </h3>
              <p className="mt-2 text-sm leading-relaxed text-muted-foreground">
                {f.body}
              </p>
            </div>
          ))}
        </div>
      </div>
    </section>
  )
}
