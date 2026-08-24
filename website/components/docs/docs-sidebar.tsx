'use client'

import Link from 'next/link'
import { usePathname } from 'next/navigation'
import { Rocket, Sliders, Zap, Code2, Shield, ChevronRight } from 'lucide-react'
import { DOC_CATEGORIES } from '@/lib/docs-data'

const ICONS: Record<string, React.ElementType> = {
  Rocket,
  Sliders,
  Zap,
  Code2,
  Shield,
}

interface DocsSidebarProps {
  onLinkClick?: () => void
}

export function DocsSidebar({ onLinkClick }: DocsSidebarProps) {
  const pathname = usePathname()

  return (
    <aside className="w-full h-full py-6 pr-4 space-y-8 overflow-y-auto">
      {/* Quick Overview Link */}
      <div>
        <Link
          href="/docs"
          onClick={onLinkClick}
          className={`flex items-center justify-between px-3 py-2 text-sm font-medium rounded-lg transition-colors ${
            pathname === '/docs'
              ? 'bg-primary/10 text-primary font-semibold'
              : 'text-muted-foreground hover:bg-muted hover:text-foreground'
          }`}
        >
          <span>Documentation Overview</span>
          <ChevronRight className="h-4 w-4 opacity-60" />
        </Link>
      </div>

      {/* Categories */}
      {DOC_CATEGORIES.map((category) => {
        const Icon = ICONS[category.iconName] || Rocket
        return (
          <div key={category.name} className="space-y-2">
            <div className="flex items-center gap-2 px-3 text-xs font-mono font-semibold uppercase tracking-wider text-muted-foreground">
              <Icon className="h-3.5 w-3.5 text-primary" />
              <span>{category.name}</span>
            </div>

            <div className="space-y-1">
              {category.items.map((item) => {
                const itemHref = `/docs/${item.slug}`
                const isActive = pathname === itemHref

                return (
                  <Link
                    key={item.slug}
                    href={itemHref}
                    onClick={onLinkClick}
                    className={`group flex items-center justify-between gap-2 rounded-lg px-3 py-2 text-sm transition-all ${
                      isActive
                        ? 'bg-primary/15 font-medium text-primary border-l-2 border-primary rounded-l-none pl-2.5'
                        : 'text-muted-foreground hover:bg-muted/60 hover:text-foreground'
                    }`}
                  >
                    <span className="truncate flex-1">{item.title}</span>
                    {item.badge && (
                      <span
                        className={`text-[10px] font-mono px-2 py-0.5 rounded-full border whitespace-nowrap shrink-0 transition-colors ${
                          isActive
                            ? 'bg-primary/20 text-primary border-primary/40 font-semibold'
                            : 'bg-muted/80 text-muted-foreground border-border group-hover:border-primary/30 group-hover:text-foreground'
                        }`}
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
