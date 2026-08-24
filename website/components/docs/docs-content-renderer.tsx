'use client'

import { useState } from 'react'
import Link from 'next/link'
import {
  Check,
  Copy,
  Info,
  Lightbulb,
  AlertTriangle,
  ShieldAlert,
  Flame,
  ArrowLeft,
  ArrowRight,
  Clock,
  Calendar,
} from 'lucide-react'
import { DocArticle, DOC_CATEGORIES } from '@/lib/docs-data'

interface DocsContentRendererProps {
  article: DocArticle
}

export function DocsContentRenderer({ article }: DocsContentRendererProps) {
  // Find prev / next articles in sequence
  const allItems = DOC_CATEGORIES.flatMap((c) => c.items)
  const currentIndex = allItems.findIndex((item) => item.slug === article.slug)
  const prevItem = currentIndex > 0 ? allItems[currentIndex - 1] : null
  const nextItem = currentIndex < allItems.length - 1 ? allItems[currentIndex + 1] : null

  return (
    <article className="prose prose-invert max-w-none space-y-12">
      {/* Article Header */}
      <div className="border-b border-border pb-8 space-y-3">
        <div className="flex items-center gap-2">
          <span className="font-mono text-xs uppercase tracking-wider text-primary font-semibold">
            {article.category}
          </span>
          {article.badge && (
            <span className="rounded bg-primary/10 border border-primary/20 px-2 py-0.5 font-mono text-xs text-primary font-medium">
              {article.badge}
            </span>
          )}
        </div>

        <h1 className="text-3xl sm:text-4xl font-bold tracking-tight text-foreground">
          {article.title}
        </h1>

        <p className="text-base sm:text-lg text-muted-foreground leading-relaxed">
          {article.description}
        </p>

        <div className="flex items-center gap-4 text-xs font-mono text-muted-foreground pt-2">
          <span className="flex items-center gap-1.5">
            <Clock className="h-3.5 w-3.5 text-primary" />
            {article.readTime}
          </span>
          <span>•</span>
          <span className="flex items-center gap-1.5">
            <Calendar className="h-3.5 w-3.5 text-primary" />
            Updated {article.lastUpdated}
          </span>
        </div>
      </div>

      {/* Article Sections */}
      <div className="space-y-12">
        {article.sections.map((section) => (
          <section key={section.id} id={section.id} className="scroll-mt-24 space-y-4">
            <h2 className="text-2xl font-semibold tracking-tight text-foreground border-b border-border/50 pb-2">
              <a href={`#${section.id}`} className="hover:text-primary transition-colors">
                {section.title}
              </a>
            </h2>

            {/* Markdown/Text content */}
            {section.content && (
              <div className="text-sm sm:text-base leading-relaxed text-muted-foreground whitespace-pre-line space-y-3">
                {section.content}
              </div>
            )}

            {/* Callout Box */}
            {section.callout && <CalloutBox type={section.callout.type} text={section.callout.text} />}

            {/* Code Block */}
            {section.code && (
              <CodeBlockWithCopy
                code={section.code.code}
                language={section.code.language}
              />
            )}

            {/* Parameter Cards (for config settings) */}
            {section.params && (
              <div className="space-y-3 pt-2">
                {section.params.map((param) => (
                  <ParamCard key={param.name} param={param} />
                ))}
              </div>
            )}

            {/* Endpoint Cards (for REST APIs) */}
            {section.endpoints && (
              <div className="space-y-4 pt-2">
                {section.endpoints.map((endpoint, idx) => (
                  <EndpointCard key={`${endpoint.method}-${endpoint.path}-${idx}`} endpoint={endpoint} />
                ))}
              </div>
            )}

            {/* Data Tables */}
            {section.table && (
              <div className="overflow-x-auto rounded-lg border border-border bg-card/40 my-4">
                <table className="w-full text-left text-sm">
                  <thead className="border-b border-border bg-muted/40 font-mono text-xs text-foreground">
                    <tr>
                      {section.table.headers.map((h, i) => (
                        <th key={i} className="px-4 py-3 font-semibold">
                          {h}
                        </th>
                      ))}
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-border/60">
                    {section.table.rows.map((row, rIdx) => (
                      <tr key={rIdx} className="hover:bg-muted/20 transition-colors">
                        {row.map((cell, cIdx) => (
                          <td key={cIdx} className="px-4 py-3 text-muted-foreground font-mono text-xs">
                            {cell}
                          </td>
                        ))}
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            )}
          </section>
        ))}
      </div>

      {/* Prev / Next Page Navigation */}
      <div className="flex flex-col sm:flex-row items-center justify-between gap-4 border-t border-border pt-8 mt-16">
        {prevItem ? (
          <Link
            href={`/docs/${prevItem.slug}`}
            className="group flex flex-col items-start w-full sm:w-auto p-4 rounded-xl border border-border bg-card/40 hover:border-primary/50 transition-all"
          >
            <span className="flex items-center gap-1 text-xs font-mono text-muted-foreground group-hover:text-primary transition-colors">
              <ArrowLeft className="h-3.5 w-3.5" /> Previous
            </span>
            <span className="text-sm font-semibold text-foreground mt-1">
              {prevItem.title}
            </span>
          </Link>
        ) : (
          <div />
        )}

        {nextItem ? (
          <Link
            href={`/docs/${nextItem.slug}`}
            className="group flex flex-col items-end w-full sm:w-auto p-4 rounded-xl border border-border bg-card/40 hover:border-primary/50 transition-all ml-auto text-right"
          >
            <span className="flex items-center gap-1 text-xs font-mono text-muted-foreground group-hover:text-primary transition-colors">
              Next <ArrowRight className="h-3.5 w-3.5" />
            </span>
            <span className="text-sm font-semibold text-foreground mt-1">
              {nextItem.title}
            </span>
          </Link>
        ) : (
          <div />
        )}
      </div>
    </article>
  )
}

function CodeBlockWithCopy({ code, language }: { code: string; language: string }) {
  const [copied, setCopied] = useState(false)

  const handleCopy = () => {
    navigator.clipboard.writeText(code)
    setCopied(true)
    setTimeout(() => setCopied(false), 2000)
  }

  return (
    <div className="relative group my-4 rounded-xl border border-border bg-[#0d0c0a] overflow-hidden shadow-lg">
      <div className="flex items-center justify-between px-4 py-2 border-b border-border/40 bg-muted/20 text-xs font-mono text-muted-foreground">
        <span>{language}</span>
        <button
          onClick={handleCopy}
          className="flex items-center gap-1 px-2 py-1 rounded bg-muted/40 hover:bg-muted text-foreground transition-colors"
          aria-label="Copy code"
        >
          {copied ? (
            <>
              <Check className="h-3.5 w-3.5 text-green-500" />
              <span className="text-green-500 text-[11px]">Copied</span>
            </>
          ) : (
            <>
              <Copy className="h-3.5 w-3.5" />
              <span className="text-[11px]">Copy</span>
            </>
          )}
        </button>
      </div>
      <pre className="p-4 overflow-x-auto text-xs sm:text-sm font-mono text-[#f3ede2] leading-relaxed">
        <code>{code}</code>
      </pre>
    </div>
  )
}

function CalloutBox({ type, text }: { type: string; text: string }) {
  const styles: Record<string, { border: string; bg: string; textCol: string; icon: React.ElementType; title: string }> = {
    note: { border: 'border-blue-500/40', bg: 'bg-blue-500/10', textCol: 'text-blue-400', icon: Info, title: 'Note' },
    tip: { border: 'border-green-500/40', bg: 'bg-green-500/10', textCol: 'text-green-400', icon: Lightbulb, title: 'Tip' },
    important: { border: 'border-primary/40', bg: 'bg-primary/10', textCol: 'text-primary', icon: Flame, title: 'Important' },
    warning: { border: 'border-amber-500/40', bg: 'bg-amber-500/10', textCol: 'text-amber-400', icon: AlertTriangle, title: 'Warning' },
    caution: { border: 'border-red-500/40', bg: 'bg-red-500/10', textCol: 'text-red-400', icon: ShieldAlert, title: 'Caution' },
  }

  const s = styles[type] || styles.note
  const Icon = s.icon

  return (
    <div className={`my-4 flex items-start gap-3 rounded-xl border ${s.border} ${s.bg} p-4 text-sm`}>
      <Icon className={`h-5 w-5 shrink-0 ${s.textCol} mt-0.5`} />
      <div className="space-y-1">
        <p className={`font-semibold ${s.textCol}`}>{s.title}</p>
        <p className="text-muted-foreground text-xs sm:text-sm leading-relaxed">{text}</p>
      </div>
    </div>
  )
}

function ParamCard({
  param,
}: {
  param: {
    name: string
    type: string
    defaultVal: string
    range?: string
    description: string
    example?: string
  }
}) {
  return (
    <div className="rounded-xl border border-border bg-card/60 p-4 space-y-2 hover:border-primary/40 transition-colors">
      <div className="flex flex-wrap items-center justify-between gap-2">
        <code className="font-mono text-sm font-bold text-primary">{param.name}</code>
        <div className="flex items-center gap-2 text-xs font-mono">
          <span className="rounded bg-muted px-2 py-0.5 text-muted-foreground">{param.type}</span>
          <span className="rounded border border-primary/30 bg-primary/10 px-2 py-0.5 text-primary">
            default: {param.defaultVal}
          </span>
          {param.range && (
            <span className="rounded bg-muted/60 px-2 py-0.5 text-muted-foreground">
              range: {param.range}
            </span>
          )}
        </div>
      </div>
      <p className="text-sm text-muted-foreground leading-relaxed">{param.description}</p>
      {param.example && (
        <div className="pt-1">
          <code className="block rounded bg-[#0f0e0c] px-3 py-1.5 font-mono text-xs text-[#e8dfd1] border border-border/40">
            {param.example}
          </code>
        </div>
      )}
    </div>
  )
}

function EndpointCard({
  endpoint,
}: {
  endpoint: {
    method: 'GET' | 'POST' | 'DELETE' | 'PUT'
    path: string
    auth: string
    description: string
    requestBody?: string
    responseBody?: string
  }
}) {
  const methodBadgeColors: Record<string, string> = {
    GET: 'bg-emerald-500/15 text-emerald-400 border-emerald-500/30',
    POST: 'bg-primary/15 text-primary border-primary/30',
    DELETE: 'bg-rose-500/15 text-rose-400 border-rose-500/30',
    PUT: 'bg-sky-500/15 text-sky-400 border-sky-500/30',
  }

  return (
    <div className="rounded-xl border border-border bg-card/60 p-5 space-y-4 shadow-sm">
      <div className="flex flex-wrap items-center justify-between gap-2">
        <div className="flex items-center gap-3">
          <span
            className={`rounded-md border px-2.5 py-1 font-mono text-xs font-bold ${
              methodBadgeColors[endpoint.method] || 'bg-muted text-foreground'
            }`}
          >
            {endpoint.method}
          </span>
          <code className="font-mono text-sm font-semibold text-foreground">{endpoint.path}</code>
        </div>
        <span className="rounded bg-muted px-2.5 py-1 text-xs font-mono text-muted-foreground">
          Auth: {endpoint.auth}
        </span>
      </div>

      <p className="text-sm text-muted-foreground">{endpoint.description}</p>

      {endpoint.requestBody && (
        <div className="space-y-1">
          <p className="font-mono text-xs text-muted-foreground font-semibold">Request Body</p>
          <pre className="rounded-lg bg-[#0d0c0a] p-3 font-mono text-xs text-[#f3ede2] overflow-x-auto border border-border/40">
            <code>{endpoint.requestBody}</code>
          </pre>
        </div>
      )}

      {endpoint.responseBody && (
        <div className="space-y-1">
          <p className="font-mono text-xs text-muted-foreground font-semibold">Response Example</p>
          <pre className="rounded-lg bg-[#0d0c0a] p-3 font-mono text-xs text-[#f3ede2] overflow-x-auto border border-border/40">
            <code>{endpoint.responseBody}</code>
          </pre>
        </div>
      )}
    </div>
  )
}
