import { GitFork, ArrowRight } from 'lucide-react'
import { Logo } from './logo'
import { Button } from '@/components/ui/button'

const REPO = 'https://github.com/vindeckyy/Solar-Flare'

const LINKS = [
  { label: 'Configuration', href: `${REPO}/blob/master/docs/CONFIGURATION.md` },
  { label: 'Changelog', href: `${REPO}/blob/master/docs/CHANGELOG-SolarFlare.md` },
  { label: 'Porting guide', href: `${REPO}/blob/master/docs/PORTING.md` },
  { label: 'API', href: `${REPO}/blob/master/docs/api.md` },
  { label: 'Security', href: `${REPO}/blob/master/SECURITY.md` },
  { label: 'Releases', href: `${REPO}/releases/latest` },
]

export function SiteFooter() {
  return (
    <footer className="relative border-t border-border">
      {/* CTA */}
      <div className="relative overflow-hidden border-b border-border">
        <div className="absolute inset-0 grid-lines opacity-30" aria-hidden="true" />
        <div className="relative mx-auto max-w-6xl px-4 py-20 text-center md:px-6 md:py-28">
          <h2 className="mx-auto max-w-2xl text-balance text-3xl font-semibold tracking-tight text-foreground md:text-5xl">
            Self-hosted streaming on your LAN.{' '}
            <span className="text-primary">No cloud in the path.</span>
          </h2>
          <div className="mt-8 flex flex-wrap items-center justify-center gap-3">
            <Button
              size="lg"
              nativeButton={false}
              className="h-11 gap-2 px-5 text-sm font-medium"
              render={<a href={REPO} target="_blank" rel="noreferrer" />}
            >
              <GitFork className="h-4 w-4" aria-hidden="true" />
              Get SolarFlare
            </Button>
            <Button
              size="lg"
              variant="outline"
              nativeButton={false}
              className="h-11 gap-2 border-border px-5 text-sm font-medium"
              render={
                <a
                  href={`${REPO}/blob/master/docs/CONFIGURATION.md`}
                  target="_blank"
                  rel="noreferrer"
                />
              }
            >
              Read the docs
              <ArrowRight className="h-4 w-4" aria-hidden="true" />
            </Button>
          </div>
        </div>
      </div>

      {/* footer bar */}
      <div className="mx-auto max-w-6xl px-4 py-10 md:px-6">
        <div className="flex flex-col gap-8 md:flex-row md:items-start md:justify-between">
          <div className="max-w-sm">
            <Logo />
            <p className="mt-4 text-sm leading-relaxed text-muted-foreground">
              A Linux game-streaming host for Moonlight, tuned for modern AMD
              and Intel CPUs. A Sunshine-derived project distributed under
              GPL-3.0-only.
            </p>
          </div>
          <nav className="grid grid-cols-2 gap-x-12 gap-y-2 sm:grid-cols-3" aria-label="Footer">
            {LINKS.map((l) => (
              <a
                key={l.label}
                href={l.href}
                target="_blank"
                rel="noreferrer"
                className="text-sm text-muted-foreground transition-colors hover:text-foreground"
              >
                {l.label}
              </a>
            ))}
          </nav>
        </div>
        <div className="mt-10 flex flex-col gap-2 border-t border-border pt-6 text-xs text-muted-foreground md:flex-row md:items-center md:justify-between">
          <p>
            GameStream foundation from{' '}
            <a
              href="https://github.com/LizardByte/Sunshine"
              target="_blank"
              rel="noreferrer"
              className="text-foreground hover:text-primary"
            >
              LizardByte / Sunshine
            </a>
            .
          </p>
          <p className="font-mono">Not affiliated with NVIDIA or Valve.</p>
        </div>
      </div>
    </footer>
  )
}
