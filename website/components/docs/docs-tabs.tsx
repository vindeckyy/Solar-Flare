'use client'

import { useId, useState } from 'react'
import { cn } from '@/lib/utils'

export interface DocsTabItem {
  id: string
  label: string
  disabled?: boolean
}

interface DocsTabsProps {
  tabs: DocsTabItem[]
  defaultTab?: string
  ariaLabel?: string
  children: (activeId: string) => React.ReactNode
}

/**
 * @brief Pill tab strip used throughout the docs portal.
 */
export function DocsTabs({ tabs, defaultTab, ariaLabel, children }: DocsTabsProps) {
  const groupId = useId()
  const [active, setActive] = useState(defaultTab || tabs[0]?.id)

  if (!tabs.length) {
    return null
  }

  return (
    <div className="my-4 space-y-4">
      <div
        role="tablist"
        aria-label={ariaLabel || 'Content tabs'}
        className="flex flex-wrap gap-1 rounded-xl border border-border bg-background p-1"
      >
        {tabs.map((tab) => {
          const selected = active === tab.id
          return (
            <button
              key={tab.id}
              id={`${groupId}-${tab.id}`}
              type="button"
              role="tab"
              aria-selected={selected}
              aria-controls={`${groupId}-panel-${tab.id}`}
              disabled={tab.disabled}
              onClick={() => setActive(tab.id)}
              className={cn(
                'relative rounded-lg px-3 py-1.5 text-xs font-medium transition-all sm:text-sm',
                'focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring/60',
                selected
                  ? 'bg-primary text-primary-foreground shadow-sm shadow-primary/30'
                  : 'text-muted-foreground hover:bg-muted/70 hover:text-foreground',
              )}
            >
              {tab.label}
            </button>
          )
        })}
      </div>
      <div
        id={`${groupId}-panel-${active}`}
        role="tabpanel"
        aria-labelledby={`${groupId}-${active}`}
        className="min-h-[4rem]"
      >
        {children(active || tabs[0].id)}
      </div>
    </div>
  )
}
