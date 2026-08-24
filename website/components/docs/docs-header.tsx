'use client'

import Link from 'next/link'
import { Search, GitFork, Menu, Home, ExternalLink } from 'lucide-react'
import { Logo } from '@/components/solarflare/logo'
import { Button } from '@/components/ui/button'

const REPO = 'https://github.com/vindeckyy/Solar-Flare'

interface DocsHeaderProps {
  onOpenSearch: () => void
  onToggleSidebar?: () => void
}

export function DocsHeader({ onOpenSearch, onToggleSidebar }: DocsHeaderProps) {
  return (
    <header className="sticky top-0 z-40 border-b border-border/80 bg-background/80 backdrop-blur-md">
      <div className="mx-auto flex h-16 max-w-[88rem] items-center justify-between gap-4 px-4 sm:px-6 lg:px-8">
        <div className="flex items-center gap-3">
          {onToggleSidebar && (
            <button
              onClick={onToggleSidebar}
              className="rounded-md p-1.5 text-muted-foreground hover:bg-muted hover:text-foreground lg:hidden"
              aria-label="Toggle navigation menu"
            >
              <Menu className="h-5 w-5" />
            </button>
          )}
          <Link href="/" className="flex items-center gap-2">
            <Logo />
            <span className="rounded-md bg-primary/10 px-2 py-0.5 font-mono text-xs font-semibold text-primary border border-primary/20">
              Docs
            </span>
          </Link>
        </div>

        {/* Global Search Bar Trigger */}
        <div className="flex-1 max-w-md mx-4 hidden md:block">
          <button
            onClick={onOpenSearch}
            className="flex w-full items-center justify-between rounded-lg border border-border bg-card/60 px-3.5 py-1.5 text-sm text-muted-foreground shadow-sm hover:border-primary/50 hover:text-foreground transition-all"
          >
            <span className="flex items-center gap-2">
              <Search className="h-4 w-4 text-primary" />
              <span>Search docs, settings, APIs...</span>
            </span>
            <kbd className="inline-flex items-center rounded border border-border bg-muted px-1.5 py-0.5 text-[10px] font-mono text-muted-foreground">
              ⌘K
            </kbd>
          </button>
        </div>

        {/* Actions */}
        <div className="flex items-center gap-2">
          <button
            onClick={onOpenSearch}
            className="rounded-md p-2 text-muted-foreground hover:bg-muted hover:text-foreground md:hidden"
            aria-label="Search documentation"
          >
            <Search className="h-5 w-5" />
          </button>

          <Button
            variant="ghost"
            size="sm"
            nativeButton={false}
            className="gap-1.5 text-muted-foreground hover:text-foreground hidden sm:inline-flex"
            render={<Link href="/" />}
          >
            <Home className="h-4 w-4" />
            Landing Page
          </Button>

          <Button
            size="sm"
            nativeButton={false}
            className="gap-1.5 font-medium"
            render={<a href={REPO} target="_blank" rel="noreferrer" />}
          >
            <GitFork className="h-4 w-4" />
            <span className="hidden sm:inline">GitHub</span>
          </Button>
        </div>
      </div>
    </header>
  )
}
