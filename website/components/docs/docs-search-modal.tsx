'use client'

import { useState, useEffect, useMemo, useRef } from 'react'
import Link from 'next/link'
import { useRouter } from 'next/navigation'
import { Search, X, FileText, Hash, ArrowRight } from 'lucide-react'
import { DocSection, DocTab, DOC_ARTICLES, DOC_CATEGORIES } from '@/lib/docs-data'

interface DocsSearchModalProps {
  isOpen: boolean
  onClose: () => void
}

interface SearchHit {
  type: 'article' | 'section'
  slug: string
  title: string
  category: string
  sectionId?: string
  snippet?: string
}

/**
 * @brief Collect every searchable text field on a section or tab, including
 * nested params, endpoints, tables, code samples, callouts, and sub-tabs.
 */
function collectSectionText(section: DocSection | DocTab): string[] {
  const parts: (string | undefined)[] = [
    'title' in section ? section.title : section.label,
    section.content,
    section.callout?.text,
    section.code?.code,
    ...(section.codeTabs?.flatMap((t) => [t.label, t.code]) ?? []),
    ...(section.params?.flatMap((p) => [
      p.name,
      p.type,
      p.defaultVal,
      p.range,
      p.description,
      p.example,
      p.note,
    ]) ?? []),
    ...(section.endpoints?.flatMap((e) => [
      e.method,
      e.path,
      e.auth,
      e.description,
      e.requestBody,
      e.responseBody,
      ...(e.scopes ?? []),
    ]) ?? []),
    ...(section.table ? [...section.table.headers, ...section.table.rows.flat()] : []),
    ...(section.image ? [section.image.alt, section.image.caption] : []),
  ]
  if ('tabs' in section && section.tabs) {
    section.tabs.forEach((tab) => parts.push(...collectSectionText(tab)))
  }
  return parts.filter((part): part is string => Boolean(part))
}

function makeSnippet(text: string, query: string): string {
  const idx = text.toLowerCase().indexOf(query)
  if (idx === -1) {
    return text.slice(0, 100)
  }
  const start = Math.max(0, idx - 40)
  const end = Math.min(text.length, idx + query.length + 80)
  const prefix = start > 0 ? '…' : ''
  const suffix = end < text.length ? '…' : ''
  return `${prefix}${text.slice(start, end).trim()}${suffix}`
}

const MAX_RESULTS = 10

export function DocsSearchModal({ isOpen, onClose }: DocsSearchModalProps) {
  const router = useRouter()
  const [query, setQuery] = useState('')
  const [activeIndex, setActiveIndex] = useState(0)
  const listRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (isOpen) {
      setQuery('')
      setActiveIndex(0)
      document.body.style.overflow = 'hidden'
      return () => {
        document.body.style.overflow = ''
      }
    }
  }, [isOpen])

  const results = useMemo<SearchHit[]>(() => {
    const q = query.trim().toLowerCase()
    if (!q) return []

    const articleHits: SearchHit[] = []
    const sectionHits: SearchHit[] = []

    Object.values(DOC_ARTICLES).forEach((article) => {
      if (
        article.title.toLowerCase().includes(q) ||
        article.description.toLowerCase().includes(q) ||
        article.category.toLowerCase().includes(q)
      ) {
        articleHits.push({
          type: 'article',
          slug: article.slug,
          title: article.title,
          category: article.category,
          snippet: article.description,
        })
      }

      article.sections.forEach((section) => {
        const fields = collectSectionText(section)
        const matched = fields.find((field) => field.toLowerCase().includes(q))
        if (!matched) return
        sectionHits.push({
          type: 'section',
          slug: article.slug,
          title: `${article.title} › ${section.title}`,
          category: article.category,
          sectionId: section.id,
          snippet: makeSnippet(matched, q),
        })
      })
    })

    return [...articleHits, ...sectionHits].slice(0, MAX_RESULTS)
  }, [query])

  useEffect(() => {
    setActiveIndex(0)
  }, [results])

  useEffect(() => {
    if (!isOpen) return
    const active = listRef.current?.querySelector('[data-active="true"]')
    active?.scrollIntoView({ block: 'nearest' })
  }, [activeIndex, isOpen])

  const openHit = (hit: SearchHit) => {
    router.push(hit.sectionId ? `/docs/${hit.slug}#${hit.sectionId}` : `/docs/${hit.slug}`)
    onClose()
    if (hit.sectionId) {
      // Same-page hash changes do not always re-scroll; nudge manually.
      setTimeout(() => {
        document.getElementById(hit.sectionId!)?.scrollIntoView({ block: 'start' })
      }, 50)
    }
  }

  const onInputKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'ArrowDown') {
      e.preventDefault()
      setActiveIndex((i) => Math.min(i + 1, results.length - 1))
    } else if (e.key === 'ArrowUp') {
      e.preventDefault()
      setActiveIndex((i) => Math.max(i - 1, 0))
    } else if (e.key === 'Enter' && results[activeIndex]) {
      e.preventDefault()
      openHit(results[activeIndex])
    }
  }

  useEffect(() => {
    if (!isOpen) return
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        onClose()
      }
    }
    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [isOpen, onClose])

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
      <div
        role="dialog"
        aria-modal="true"
        aria-label="Search documentation"
        className="relative w-full max-w-2xl overflow-hidden rounded-xl border border-border bg-card shadow-2xl"
      >
        {/* Search input bar */}
        <div className="flex items-center border-b border-border px-4 py-3">
          <Search className="h-5 w-5 text-muted-foreground shrink-0 mr-3" />
          <input
            type="text"
            role="combobox"
            aria-expanded="true"
            aria-controls="docs-search-results"
            aria-activedescendant={
              results.length > 0 ? `docs-search-option-${activeIndex}` : undefined
            }
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            onKeyDown={onInputKeyDown}
            placeholder="Search documentation, guides, APIs, tunables..."
            className="w-full bg-transparent text-sm text-foreground placeholder:text-muted-foreground focus:outline-none"
            autoFocus
          />
          {query && (
            <button
              onClick={() => setQuery('')}
              className="text-muted-foreground hover:text-foreground mr-2"
              aria-label="Clear search"
            >
              <X className="h-4 w-4" />
            </button>
          )}
          <kbd className="hidden sm:inline-flex items-center rounded border border-border bg-muted px-2 py-0.5 text-xs text-muted-foreground font-mono">
            ESC
          </kbd>
        </div>

        {/* Search Results list */}
        <div ref={listRef} className="max-h-96 overflow-y-auto p-2" aria-live="polite">
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
            <div className="space-y-1" role="listbox" id="docs-search-results" aria-label="Search results">
              {results.map((hit, idx) => {
                const targetHref = hit.sectionId
                  ? `/docs/${hit.slug}#${hit.sectionId}`
                  : `/docs/${hit.slug}`
                const isActive = idx === activeIndex
                return (
                  <Link
                    key={`${hit.slug}-${hit.sectionId || 'article'}`}
                    id={`docs-search-option-${idx}`}
                    role="option"
                    aria-selected={isActive}
                    data-active={isActive}
                    href={targetHref}
                    onClick={onClose}
                    onMouseEnter={() => setActiveIndex(idx)}
                    className={`group flex items-start gap-3 rounded-lg p-3 transition-colors ${
                      isActive ? 'bg-muted/70' : 'hover:bg-muted/60'
                    }`}
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
          <span>
            {results.length > 0 ? `${results.length} result${results.length === 1 ? '' : 's'}` : 'Navigation'}
          </span>
          <div className="flex items-center gap-3">
            <span className="flex items-center gap-1">
              <kbd className="rounded border border-border bg-muted px-1.5 py-0.5 font-mono">↑↓</kbd>
              <span>to navigate</span>
            </span>
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
