import { Smartphone, Monitor, Server, Tv, Gamepad2 } from 'lucide-react'

const FACTS = [
  { k: 'Primary use', v: 'High-quality game & desktop streaming across a trusted local network' },
  { k: 'Host focus', v: 'Linux x86-64, with native tuning for modern AMD and Intel CPUs' },
  { k: 'Client protocol', v: 'Moonlight / NVIDIA GameStream-compatible transport' },
  { k: 'Control plane', v: 'Responsive HTTPS interface at localhost:47990' },
  { k: 'Upstream', v: 'A fork of LizardByte / Sunshine' },
  { k: 'License', v: 'GPL-3.0-only' },
]

const USES = [
  { icon: Gamepad2, label: 'In-home game streaming' },
  { icon: Monitor, label: 'Remote desktop' },
  { icon: Server, label: 'Headless game server' },
  { icon: Tv, label: 'Couch / TV play' },
  { icon: Smartphone, label: 'Handheld & phone' },
]

const STATS = [
  { n: '0', label: 'cloud services in the stream path' },
  { n: '34', label: 'fork-specific tunables (10 host + 24 audio)' },
  { n: '5', label: 'Linux distro families auto-detected' },
]

export function Overview() {
  return (
    <section id="overview" className="relative border-t border-border py-20 md:py-28">
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="grid gap-12 lg:grid-cols-[1.1fr_0.9fr] lg:gap-16">
          {/* narrative */}
          <div>
            <p className="font-mono text-xs uppercase tracking-wider text-primary">
              What it does
            </p>
            <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
              Your own game-streaming host — no subscription, no cloud
            </h2>
            <div className="mt-5 space-y-4 text-pretty leading-relaxed text-muted-foreground">
              <p>
                SolarFlare turns a Linux machine into a private streaming host.
                Install it on the PC that has your GPU, pair a{' '}
                <a
                  href="https://moonlight-stream.org/"
                  target="_blank"
                  rel="noreferrer"
                  className="text-foreground underline decoration-primary/40 underline-offset-4 hover:decoration-primary"
                >
                  Moonlight
                </a>{' '}
                client — phone, tablet, laptop, TV, or handheld — and play your
                full library remotely over your own network. The video never
                leaves your LAN and no third-party server sits in the path.
              </p>
              <p>
                It is a precision fork of LizardByte&apos;s Sunshine, focused on
                Linux and AMD. On top of the GameStream-compatible transport it
                layers link-aware network pacing, real-time capture scheduling,
                per-application NVENC encoder profiles, a pre-encoder audio DSP
                chain, and an original observatory-style Web UI that runs the
                entire host from one instrument panel.
              </p>
            </div>

            {/* use cases */}
            <div className="mt-8 flex flex-wrap gap-2">
              {USES.map((u) => (
                <span
                  key={u.label}
                  className="inline-flex items-center gap-2 rounded-full border border-border bg-card px-3 py-1.5 text-sm text-muted-foreground"
                >
                  <u.icon className="h-4 w-4 text-primary" aria-hidden="true" />
                  {u.label}
                </span>
              ))}
            </div>

            {/* stats */}
            <div className="mt-10 grid grid-cols-1 gap-5 min-[420px]:grid-cols-3 min-[420px]:gap-4">
              {STATS.map((s) => (
                <div key={s.label}>
                  <div className="font-mono text-3xl font-semibold text-primary md:text-4xl">
                    {s.n}
                  </div>
                  <p className="mt-1 text-xs leading-relaxed text-muted-foreground">
                    {s.label}
                  </p>
                </div>
              ))}
            </div>
          </div>

          {/* facts card */}
          <div className="lg:pt-14">
            <dl className="overflow-hidden rounded-2xl border border-border bg-card">
              {FACTS.map((f, i) => (
                <div
                  key={f.k}
                  className={
                    'flex flex-col gap-1 px-5 py-4' +
                    (i !== 0 ? ' border-t border-border' : '')
                  }
                >
                  <dt className="font-mono text-[11px] uppercase tracking-wider text-muted-foreground">
                    {f.k}
                  </dt>
                  <dd className="text-sm leading-relaxed text-foreground">
                    {f.v}
                  </dd>
                </div>
              ))}
            </dl>
          </div>
        </div>
      </div>
    </section>
  )
}
