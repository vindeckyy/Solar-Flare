'use client'

import { useState } from 'react'
import { DocsHeader } from '@/components/docs/docs-header'
import { DocsSidebar } from '@/components/docs/docs-sidebar'
import { DocsSearchModal } from '@/components/docs/docs-search-modal'
import { X } from 'lucide-react'

export default function DocsLayout({ children }: { children: React.ReactNode }) {
  const [isSearchOpen, setIsSearchOpen] = useState(false)
  const [isMobileSidebarOpen, setIsMobileSidebarOpen] = useState(false)

  return (
    <div className="min-h-screen bg-background text-foreground flex flex-col">
      {/* Search Modal */}
      <DocsSearchModal isOpen={isSearchOpen} onClose={() => setIsSearchOpen(false)} />

      {/* Docs Header */}
      <DocsHeader
        onOpenSearch={() => setIsSearchOpen(true)}
        onToggleSidebar={() => setIsMobileSidebarOpen(!isMobileSidebarOpen)}
      />

      {/* Mobile Sidebar Overlay */}
      {isMobileSidebarOpen && (
        <div className="fixed inset-0 z-50 flex lg:hidden">
          <div
            className="fixed inset-0 bg-background/80 backdrop-blur-sm"
            onClick={() => setIsMobileSidebarOpen(false)}
          />
          <div className="relative flex w-full max-w-xs flex-1 flex-col bg-card border-r border-border p-6 shadow-xl">
            <div className="flex items-center justify-between pb-4 border-b border-border">
              <span className="font-mono text-sm font-semibold text-primary uppercase tracking-wider">
                Documentation
              </span>
              <button
                onClick={() => setIsMobileSidebarOpen(false)}
                className="rounded-md p-1.5 text-muted-foreground hover:bg-muted"
              >
                <X className="h-5 w-5" />
              </button>
            </div>
            <div className="flex-1 overflow-y-auto pt-4">
              <DocsSidebar onLinkClick={() => setIsMobileSidebarOpen(false)} />
            </div>
          </div>
        </div>
      )}

      {/* Main Body with Sidebar */}
      <div className="flex-1 mx-auto w-full max-w-[88rem] px-4 sm:px-6 lg:px-8">
        <div className="flex gap-8 lg:gap-10">
          {/* Desktop Left Sidebar */}
          <div className="hidden lg:block w-72 xl:w-80 shrink-0 border-r border-border/60 min-h-[calc(100vh-4rem)]">
            <div className="sticky top-16 h-[calc(100vh-4rem)] overflow-y-auto">
              <DocsSidebar />
            </div>
          </div>

          {/* Main Article Container */}
          <main className="flex-1 min-w-0 py-8 lg:py-10">{children}</main>
        </div>
      </div>
    </div>
  )
}
