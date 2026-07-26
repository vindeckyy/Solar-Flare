import {
  Mic,
  Volume2,
  AudioWaveform,
  ArrowDownWideNarrow,
  Filter,
  Radio,
  ChevronRight,
} from 'lucide-react'

const CHAIN = [
  { icon: Mic, label: 'PipeWire capture' },
  { icon: Volume2, label: 'AGC' },
  { icon: AudioWaveform, label: 'VAD' },
  { icon: ArrowDownWideNarrow, label: 'Ducking' },
  { icon: Filter, label: 'Noise gate' },
  { icon: Radio, label: 'Opus encoder' },
]

const STAGES = [
  {
    title: 'Automatic gain control',
    key: 'sf_audio_agc',
    body: 'Measures the RMS level of each frame and rides a smooth gain correction toward a target loudness (−20 dBFS by default), clamped between configurable min and max gain with attack, hold, and release timing.',
  },
  {
    title: 'Voice activity detection',
    key: 'sf_audio_vad',
    body: 'Classifies each frame as speech or non-speech using a threshold plus a hysteresis band and minimum speech/silence durations. VAD alone does not change the audio; it produces the voice-active signal that ducking and gating consume.',
  },
  {
    title: 'Speech ducking',
    key: 'sf_audio_ducking',
    body: 'When you talk, game audio is attenuated (−12 dB by default) so speech stays intelligible during loud gameplay, ramping in and out smoothly to avoid audible pumping.',
  },
  {
    title: 'Noise gate',
    key: 'sf_audio_noise_gate',
    body: 'Zeroes any frame below the threshold to kill constant background hiss, fan noise, or open-mic floor noise, and leaves louder content alone.',
  },
  {
    title: 'Opus encoder tuning',
    key: 'sf_opus_*',
    body: 'Full control over the audio codec: LOWDELAY / VOIP / AUDIO application mode, CBR or VBR, complexity, in-band FEC with an expected-loss hint, and fullband bandwidth extension.',
  },
]

export function AudioFx() {
  return (
    <section id="audio" className="relative border-t border-border py-20 md:py-28">
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="max-w-2xl">
          <p className="font-mono text-xs uppercase tracking-wider text-primary">
            Audio FX
          </p>
          <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
            A pre-encoder DSP chain, built into the host
          </h2>
          <p className="mt-4 text-pretty leading-relaxed text-muted-foreground">
            SolarFlare runs a lightweight audio pre-processor between the
            PipeWire capture callback and the Opus encoder. Every stage is
            opt-in and defaults to off, so a vanilla install sounds exactly like
            upstream until you turn a stage on from the Web UI.
          </p>
        </div>

        {/* signal chain */}
        <div className="mt-12 flex flex-wrap items-center gap-2 rounded-2xl border border-border bg-card p-5">
          {CHAIN.map((c, i) => (
            <div key={c.label} className="flex items-center gap-2">
              <div className="inline-flex items-center gap-2 rounded-lg border border-border bg-background px-3 py-2">
                <c.icon className="h-4 w-4 text-primary" aria-hidden="true" />
                <span className="text-sm text-foreground">{c.label}</span>
              </div>
              {i < CHAIN.length - 1 && (
                <ChevronRight
                  className="h-4 w-4 shrink-0 text-primary/50"
                  aria-hidden="true"
                />
              )}
            </div>
          ))}
        </div>

        {/* stages */}
        <div className="mt-6 grid gap-px overflow-hidden rounded-2xl border border-border bg-border md:grid-cols-2 lg:grid-cols-3">
          {STAGES.map((s) => (
            <div key={s.title} className="bg-card p-6">
              <code className="font-mono text-xs text-primary">{s.key}</code>
              <h3 className="mt-2 text-base font-semibold text-foreground">
                {s.title}
              </h3>
              <p className="mt-2 text-sm leading-relaxed text-muted-foreground">
                {s.body}
              </p>
            </div>
          ))}
          <div className="flex flex-col justify-center bg-card p-6">
            <p className="text-sm leading-relaxed text-muted-foreground">
              All <span className="font-mono text-foreground">24</span>{' '}
              <span className="font-mono text-primary">sf_audio_*</span> /{' '}
              <span className="font-mono text-primary">sf_opus_*</span> keys are
              exposed in the Web UI&apos;s Audio / Video tab and validated by a
              source-to-docs consistency test.
            </p>
          </div>
        </div>
      </div>
    </section>
  )
}
