import { Aperture, Cpu, Waypoints, SlidersHorizontal, ChevronRight } from 'lucide-react'

const STAGES = [
  {
    n: '01',
    icon: Aperture,
    title: 'Capture',
    body: 'X11, KMS, PipeWire/portal, headless compositor, and optional Hermes-KMS paths are selected for the build and host environment.',
  },
  {
    n: '02',
    icon: Cpu,
    title: 'Encode',
    body: 'NVENC presets and per-application overrides tune latency, lookahead, adaptive quantization, and frame structure — without changing the protocol.',
  },
  {
    n: '03',
    icon: Waypoints,
    title: 'Transport',
    body: 'Link-speed detection, pacing, socket buffers, busy polling, QoS marking, and adaptive bitrate respond to local-network conditions.',
  },
  {
    n: '04',
    icon: SlidersHorizontal,
    title: 'Control',
    body: 'The HTTPS UI, API scopes, pairing rules, and diagnostics expose host state — without placing cloud services in the streaming path.',
  },
]

export function Architecture() {
  return (
    <section
      id="architecture"
      className="relative border-t border-border bg-card/30 py-20 md:py-28"
    >
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="max-w-2xl">
          <p className="font-mono text-xs uppercase tracking-wider text-primary">
            Performance architecture
          </p>
          <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
            An event-driven path from display to client
          </h2>
          <p className="mt-4 text-pretty leading-relaxed text-muted-foreground">
            The fork-specific work is concentrated in four areas. Capture is
            driven by frame arrival, per-session pacing bounds batches and
            rejects stale frames, and the encoder applies live NVENC bitrate
            changes when supported.
          </p>
        </div>

        {/* pipeline */}
        <div className="mt-12 grid items-stretch gap-4 lg:grid-cols-[1fr_auto_1fr_auto_1fr_auto_1fr]">
          {STAGES.map((s, i) => (
            <div key={s.title} className="contents">
              <div className="relative rounded-xl border border-border bg-card p-6">
                <div className="flex items-center justify-between">
                  <div className="flex h-10 w-10 items-center justify-center rounded-lg border border-border bg-background text-primary">
                    <s.icon className="h-5 w-5" aria-hidden="true" />
                  </div>
                  <span className="font-mono text-xs text-muted-foreground">
                    {s.n}
                  </span>
                </div>
                <h3 className="mt-4 text-base font-semibold text-foreground">
                  {s.title}
                </h3>
                <p className="mt-2 text-sm leading-relaxed text-muted-foreground">
                  {s.body}
                </p>
              </div>
              {i < STAGES.length - 1 && (
                <div className="flex items-center justify-center py-1 lg:py-0">
                  <ChevronRight
                    className="h-5 w-5 rotate-90 text-primary/60 lg:rotate-0"
                    aria-hidden="true"
                  />
                </div>
              )}
            </div>
          ))}
        </div>

        <div className="mt-6 rounded-xl border border-border bg-card/60 px-5 py-4 font-mono text-xs">
          <p className="text-foreground">latency_mode</p>
          <div className="mt-3 grid gap-3 sm:grid-cols-2">
            <div className="rounded-lg border border-border/70 bg-background/40 p-3">
              <p className="text-primary">safe</p>
              <p className="mt-1 leading-relaxed text-muted-foreground">
                Bounded, quality-preserving defaults.
              </p>
            </div>
            <div className="rounded-lg border border-border/70 bg-background/40 p-3">
              <p className="text-primary">aggressive</p>
              <p className="mt-1 leading-relaxed text-muted-foreground">
                Tighter audio and scaler tradeoffs; two-pass NVENC is off.
              </p>
            </div>
          </div>
        </div>
      </div>
    </section>
  )
}
