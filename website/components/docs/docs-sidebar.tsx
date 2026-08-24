'use client'

import Link from 'next/link'
import { usePathname } from 'next/navigation'
import { Rocket, Sliders, Zap, Code2, Shield, Boxes, ChevronRight } from 'lucide-react'
import { DOC_CATEGORIES } from '@/lib/docs-data'
import { cn } from '@/lib/utils'

const ICONS: Record<string, React.ElementType> = {
  Rocket,
  Sliders,
  Zap,
  Code2,
  Shield,
  Boxes,
}

interface DocsSidebarProps {
  onLinkClick?: () => void
}

export function DocsSidebar({ onLinkClick }: DocsSidebarProps) {
  const pathname = usePathname()

  return (
    <aside className="w-full h-full py-6 pr-3 space-y-7 overflow-y-auto">
      <div>
        <Link
          href="/docs"
          onClick={onLinkClick}
          className={cn(
            'flex items-center justify-between px-3 py-2 text-sm font-medium rounded-lg transition-colors',
            pathname === '/docs'
              ? 'bg-primary/10 text-primary font-semibold'
              : 'text-muted-foreground hover:bg-muted hover:text-foreground',
          )}
        >
          <span>Documentation Overview</span>
          <ChevronRight className="h-4 w-4 opacity-60" />
        </Link>
      </div>

      {DOC_CATEGORIES.map((category) => {
        const Icon = ICONS[category.iconName] || Rocket
        return (
          <div key={category.name} className="space-y-2">
            <div className="flex items-center gap-2 px-3 text-xs font-mono font-semibold uppercase tracking-wider text-muted-foreground">
              <Icon className="h-3.5 w-3.5 text-primary" />
              <span>{category.name}</span>
            </div>

            <div className="space-y-0.5">
              {category.items.map((item) => {
                const itemHref = `/docs/${item.slug}`
                const isActive = pathname === itemHref

                return (
                  <Link
                    key={item.slug}
                    href={itemHref}
                    onClick={onLinkClick}
                    className={cn(
              'group flex items-center justify-between gap-2 rounded-md px-3 py-2 text-sm transition-all',
              isActive
                ? 'bg-primary text-primary-foreground font-semibold shadow-sm shadow-primary/30'
                : 'text-foreground/75 hover:bg-muted hover:text-foreground',
            )}
                  >
                    <span className="truncate flex-1">{item.title}</span>
                    {item.badge && (
                      <span
                        className={cn(
                          'text-[10px] font-mono px-2 py-0.5 rounded-full border whitespace-nowrap shrink-0 transition-colors',
                          isActive
                            ? 'bg-primary-foreground/15 text-primary-foreground border-primary-foreground/30 font-semibold'
                            : 'bg-muted text-muted-foreground border-border group-hover:border-primary/30 group-hover:text-foreground',
                        )}
                      >
                        {item.badge}
                      </span>
                    )}
                  </Link>
                )
              })}
            </div>
          </div>
        )
      })}
    </aside>
  )
}
