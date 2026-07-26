import { CodeBlock } from './code-block'

const TUNABLES = [
  { key: 'busy_poll_us', def: '50', desc: 'SO_BUSY_POLL on the ENet UDP socket, in microseconds.' },
  { key: 'rate_cap_pct', def: '80', desc: 'Percent of negotiated link speed used as the pacer.' },
  { key: 'enet_4mib_buffer', def: 'true', desc: 'Grow ENet UDP send/recv buffers to 4 MiB.' },
  { key: 'pipewire_latency_ms', def: '8', desc: 'PW_KEY_NODE_LATENCY hint to the PipeWire compositor.' },
  { key: 'cpu_pinning', def: 'true', desc: 'Pin the capture thread to a non-IRQ, non-SMT core.' },
  { key: 'dscp_qos', def: 'true', desc: 'Tag ENet packets with DSCP CS3 for router QoS.' },
  { key: 'gpu_governor', def: 'true', desc: 'Force the GPU performance profile during a stream.' },
  { key: 'headless_virtual_display', def: 'false', desc: 'Create a virtual xrandr output when no display exists.' },
  { key: 'skip_wayland_correlation', def: 'false', desc: 'Skip KMS↔Wayland correlation; leave false unless your compositor omits output metadata.' },
  { key: 'latency_mode', def: 'safe', desc: 'safe or aggressive latency-first media behavior.' },
]

export function Configuration() {
  return (
    <section
      id="configuration"
      className="relative border-t border-border bg-card/30 py-20 md:py-28"
    >
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="max-w-2xl">
          <p className="font-mono text-xs uppercase tracking-wider text-primary">
            Configuration
          </p>
          <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
            Dial the fast path in or out. No rebuild required.
          </h2>
          <p className="mt-4 text-pretty leading-relaxed text-muted-foreground">
            Ten fork-specific tunables live in the same{' '}
            <code className="rounded bg-background px-1.5 py-0.5 font-mono text-sm text-primary">
              ~/.config/sunshine/sunshine.conf
            </code>{' '}
            file. Each is opt-out: revert a knob to its upstream value to undo
            that subsystem&apos;s tuning. Plus 24 audio &amp; Opus controls in the
            Web UI.
          </p>
        </div>

        <div className="mt-12 grid gap-8 lg:grid-cols-2">
          {/* tunable list */}
          <div className="min-w-0 overflow-hidden rounded-2xl border border-border bg-card">
            <div className="border-b border-border px-5 py-3 font-mono text-xs uppercase tracking-wider text-muted-foreground">
              The tunables at a glance
            </div>
            <ul className="divide-y divide-border">
              {TUNABLES.map((t) => (
                <li key={t.key} className="flex flex-col gap-1 px-5 py-3">
                  <div className="flex items-baseline justify-between gap-3">
                    <code className="font-mono text-sm text-primary">
                      {t.key}
                    </code>
                    <span className="shrink-0 rounded border border-border bg-background px-2 py-0.5 font-mono text-xs text-muted-foreground">
                      {t.def}
                    </span>
                  </div>
                  <span className="text-sm leading-relaxed text-muted-foreground">
                    {t.desc}
                  </span>
                </li>
              ))}
            </ul>
          </div>

          {/* code examples */}
          <div className="flex min-w-0 flex-col gap-6">
            <CodeBlock
              title="sunshine.conf"
              lines={[
                'min_log_level = 1',
                'busy_poll_us = 50',
                'rate_cap_pct = 90',
                'enet_4mib_buffer = true',
                'pipewire_latency_ms = 6',
                'cpu_pinning = true',
                'dscp_qos = true',
                'latency_mode = aggressive',
              ]}
            />
            <CodeBlock
              title="apps.json: per-application encoder override"
              lines={[
                '{',
                '  "name": "Competitive profile",',
                '  "cmd": "steam steam://rungameid/730",',
                '  "encoder-preset": 0',
                '}',
                '',
                '// -1 host default · 0 latency · 1 balanced · 2 quality',
              ]}
            />
          </div>
        </div>
      </div>
    </section>
  )
}
