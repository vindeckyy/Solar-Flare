'use client'

import { useEffect, useState } from 'react'
import { DocSection } from '@/lib/docs-data'
import { AlignLeft, MessageSquare, ExternalLink } from 'lucide-react'

interface DocsTocProps {
  sections: DocSection[]
}

const REPO = 'https://github.com/vindeckyy/Solar-Flare'

export function DocsToc({ sections }: DocsTocProps) {
  const [activeId, setActiveId] = useState<string>(sections[0]?.id || '')

  useEffect(() => {
    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            setActiveId(entry.target.id)
          }
        })
      },
      {
        rootMargin: '0px 0px -60% 0px',
        threshold: 0.1,
      }
    )

    sections.forEach((sec) => {
      const el = document.getElementById(sec.id)
      if (el) observer.observe(el)
    })

    return () => observer.disconnect()
  }, [sections])

  if (!sections || sections.length === 0) return null

  return (
    <div className="space-y-6">
      <div className="space-y-3">
        <div className="flex items-center gap-2 text-xs font-mono font-semibold uppercase tracking-wider text-muted-foreground">
          <AlignLeft className="h-3.5 w-3.5 text-primary" />
          <span>On this page</span>
        </div>

        <nav className="space-y-1 text-sm" aria-label="Table of contents">
          {sections.map((sec) => {
            const isActive = activeId === sec.id
            return (
              <a
                key={sec.id}
                href={`#${sec.id}`}
                className={`block py-1 text-xs transition-colors truncate ${
                  isActive
                    ? 'text-primary font-medium pl-2 border-l border-primary'
                    : 'text-muted-foreground hover:text-foreground pl-2 border-l border-transparent'
                }`}
              >
                {sec.title}
              </a>
            )
          })}
        </nav>
      </div>

      <div className="border-t border-border pt-4 space-y-2">
        <a
          href={`${REPO}/issues/new/choose`}
          target="_blank"
          rel="noreferrer"
          className="flex items-center gap-2 text-xs text-muted-foreground hover:text-foreground transition-colors"
        >
          <MessageSquare className="h-3.5 w-3.5 text-primary" />
          <span>Report doc feedback</span>
          <ExternalLink className="h-3 w-3 opacity-60 ml-auto" />
        </a>
      </div>
    </div>
  )
}
