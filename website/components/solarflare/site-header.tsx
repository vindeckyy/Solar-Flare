import Link from 'next/link'
import { GitFork, BookOpen } from 'lucide-react'
import { Logo } from './logo'
import { Button } from '@/components/ui/button'

const NAV = [
  { label: 'Overview', href: '#overview' },
  { label: 'Architecture', href: '#architecture' },
  { label: 'Interface', href: '#interface' },
  { label: 'Configuration', href: '#configuration' },
  { label: 'Audio', href: '#audio' },
  { label: 'Install', href: '#install' },
]

const REPO = 'https://github.com/vindeckyy/Solar-Flare'

export function SiteHeader() {
  return (
    <header className="sticky top-0 z-50 border-b border-border/80 bg-background/80 backdrop-blur-md">
      <div className="mx-auto flex h-16 max-w-6xl items-center justify-between gap-4 px-4 md:px-6">
        <Link href="#top" className="shrink-0">
          <Logo />
        </Link>

        <nav className="hidden items-center gap-7 md:flex" aria-label="Primary">
          {NAV.map((item) => (
            <a
              key={item.href}
              href={item.href}
              className="text-sm text-muted-foreground transition-colors hover:text-foreground"
            >
              {item.label}
            </a>
          ))}
        </nav>

        <div className="flex items-center gap-2">
          <Button
            variant="outline"
            size="sm"
            nativeButton={false}
            className="gap-1.5 border-border hover:border-primary/50 text-foreground font-medium"
            render={<Link href="/docs" />}
          >
            <BookOpen className="h-4 w-4 text-primary" aria-hidden="true" />
            Docs
          </Button>

          <Button
            variant="ghost"
            size="sm"
            nativeButton={false}
            className="hidden text-muted-foreground hover:text-foreground sm:inline-flex"
            render={
              <a href={`${REPO}/releases/latest`} target="_blank" rel="noreferrer" />
            }
          >
            Releases
          </Button>

          <Button
            size="sm"
            nativeButton={false}
            className="gap-2 font-medium"
            render={<a href={REPO} target="_blank" rel="noreferrer" />}
          >
            <GitFork className="h-4 w-4" aria-hidden="true" />
            <span className="hidden sm:inline">Star on </span>GitHub
          </Button>
        </div>
      </div>
    </header>
  )
}

