'use client'

import { useState, useEffect, useMemo } from 'react'
import Link from 'next/link'
import { Search, X, FileText, Hash, ArrowRight, CornerDownLeft } from 'lucide-react'
import { DOC_ARTICLES, DOC_CATEGORIES } from '@/lib/docs-data'

interface DocsSearchModalProps {
  isOpen: boolean
  onClose: () => void
}

export function DocsSearchModal({ isOpen, onClose }: DocsSearchModalProps) {
  const [query, setQuery] = useState('')

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault()
        if (isOpen) {
          onClose()
        } else {
          // Open handled by parent listener
        }
      }
      if (e.key === 'Escape' && isOpen) {
        onClose()
      }
    }
    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [isOpen, onClose])

  const results = useMemo(() => {
    if (!query.trim()) return []
    const q = query.toLowerCase()

    const hits: {
      type: 'article' | 'section'
      slug: string
      title: string
      category: string
      sectionId?: string
      snippet?: string
    }[] = []

    Object.values(DOC_ARTICLES).forEach((article) => {
      if (
        article.title.toLowerCase().includes(q) ||
        article.description.toLowerCase().includes(q) ||
        article.category.toLowerCase().includes(q)
      ) {
        hits.push({
          type: 'article',
          slug: article.slug,
          title: article.title,
          category: article.category,
          snippet: article.description,
        })
      }

      article.sections.forEach((sec) => {
        const haystack = [
          sec.title,
          sec.content,
          ...(sec.tabs?.flatMap((tab) => [tab.label, tab.content]) || []),
        ]
          .filter(Boolean)
          .join(' ')
          .toLowerCase()
        if (haystack.includes(q)) {
          hits.push({
            type: 'section',
            slug: article.slug,
            title: `${article.title} > ${sec.title}`,
            category: article.category,
            sectionId: sec.id,
            snippet: sec.content ? sec.content.slice(0, 100) + '...' : undefined,
          })
        }
      })
    })

    return hits.slice(0, 8)
  }, [query])

  if (!isOpen) return null

  return (
    <div className="fixed inset-0 z-50 flex items-start justify-center p-4 pt-16 sm:pt-24">
      {/* Backdrop */}
      <div
        className="fixed inset-0 bg-background/80 backdrop-blur-sm transition-opacity"
        onClick={onClose}
        aria-hidden="true"
      />

      {/* Dialog */}
      <div className="relative w-full max-w-2xl overflow-hidden rounded-xl border border-border bg-card shadow-2xl">
        {/* Search input bar */}
        <div className="flex items-center border-b border-border px-4 py-3">
          <Search className="h-5 w-5 text-muted-foreground shrink-0 mr-3" />
          <input
            type="text"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            placeholder="Search documentation, guides, APIs, tunables..."
            className="w-full bg-transparent text-sm text-foreground placeholder:text-muted-foreground focus:outline-none"
            autoFocus
          />
          {query && (
            <button
              onClick={() => setQuery('')}
              className="text-muted-foreground hover:text-foreground mr-2"
            >
              <X className="h-4 w-4" />
            </button>
          )}
          <kbd className="hidden sm:inline-flex items-center rounded border border-border bg-muted px-2 py-0.5 text-xs text-muted-foreground font-mono">
            ESC
          </kbd>
        </div>

        {/* Search Results list */}
        <div className="max-h-96 overflow-y-auto p-2">
          {query.trim() === '' ? (
            <div className="p-6 text-center text-sm text-muted-foreground">
              <p className="font-medium text-foreground">Quick Suggestions</p>
              <div className="mt-3 flex flex-wrap justify-center gap-2">
                {DOC_CATEGORIES.flatMap((c) => c.items)
                  .slice(0, 6)
                  .map((item) => (
                    <Link
                      key={item.slug}
                      href={`/docs/${item.slug}`}
                      onClick={onClose}
                      className="rounded-lg border border-border bg-muted/40 px-3 py-1.5 text-xs text-foreground hover:border-primary hover:text-primary transition-colors"
                    >
                      {item.title}
                    </Link>
                  ))}
              </div>
            </div>
          ) : results.length === 0 ? (
            <div className="p-8 text-center text-sm text-muted-foreground">
              No results found for &ldquo;<span className="text-foreground">{query}</span>&rdquo;
            </div>
          ) : (
            <div className="space-y-1">
              {results.map((hit, idx) => {
                const targetHref = hit.sectionId
                  ? `/docs/${hit.slug}#${hit.sectionId}`
                  : `/docs/${hit.slug}`
                return (
                  <Link
                    key={`${hit.slug}-${hit.sectionId || ''}-${idx}`}
                    href={targetHref}
                    onClick={onClose}
                    className="group flex items-start gap-3 rounded-lg p-3 hover:bg-muted/60 transition-colors"
                  >
                    <div className="mt-0.5 rounded p-1 text-primary bg-primary/10">
                      {hit.type === 'article' ? (
                        <FileText className="h-4 w-4" />
                      ) : (
                        <Hash className="h-4 w-4" />
                      )}
                    </div>
                    <div className="flex-1 min-w-0">
                      <div className="flex items-center gap-2">
                        <span className="text-xs font-mono uppercase text-muted-foreground">
                          {hit.category}
                        </span>
                      </div>
                      <p className="text-sm font-medium text-foreground group-hover:text-primary transition-colors truncate">
                        {hit.title}
                      </p>
                      {hit.snippet && (
                        <p className="text-xs text-muted-foreground line-clamp-1 mt-0.5">
                          {hit.snippet}
                        </p>
                      )}
                    </div>
                    <ArrowRight className="h-4 w-4 text-muted-foreground opacity-0 group-hover:opacity-100 transition-opacity self-center shrink-0" />
                  </Link>
                )
              })}
            </div>
          )}
        </div>

        {/* Footer shortcuts */}
        <div className="flex items-center justify-between border-t border-border bg-muted/20 px-4 py-2 text-xs text-muted-foreground">
          <span>Navigation</span>
          <div className="flex items-center gap-3">
            <span className="flex items-center gap-1">
              <kbd className="rounded border border-border bg-muted px-1.5 py-0.5 font-mono">↵</kbd>
              <span>to select</span>
            </span>
            <span className="flex items-center gap-1">
              <kbd className="rounded border border-border bg-muted px-1.5 py-0.5 font-mono">esc</kbd>
              <span>to close</span>
            </span>
          </div>
        </div>
      </div>
    </div>
  )
}
