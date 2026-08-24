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
import {
  DocArticle,
  DocCallout,
  DocEndpoint,
  DocParam,
  DocSection,
  DocTab,
  DocTable,
  DOC_CATEGORIES,
} from '@/lib/docs-data'
import { DocsTabs } from '@/components/docs/docs-tabs'
import { DocsMarkdown } from '@/components/docs/docs-markdown'

interface DocsContentRendererProps {
  article: DocArticle
}

export function DocsContentRenderer({ article }: DocsContentRendererProps) {
  const allItems = DOC_CATEGORIES.flatMap((c) => c.items)
  const currentIndex = allItems.findIndex((item) => item.slug === article.slug)
  const prevItem = currentIndex > 0 ? allItems[currentIndex - 1] : null
  const nextItem = currentIndex < allItems.length - 1 ? allItems[currentIndex + 1] : null

  return (
    <article className="max-w-none space-y-6">
      <header className="relative overflow-hidden rounded-2xl border border-primary/25 bg-card p-6 sm:p-8 shadow-[0_0_40px_-12px_color-mix(in_oklch,var(--primary)_45%,transparent)]">
        <div
          className="pointer-events-none absolute inset-0"
          style={{
            background:
              'radial-gradient(720px circle at 0% 0%, color-mix(in oklch, var(--primary) 28%, transparent), transparent 58%)',
          }}
        />
        <div className="relative space-y-3">
          <div className="flex flex-wrap items-center gap-2">
            <span className="font-mono text-xs uppercase tracking-wider text-primary font-semibold">
              {article.category}
            </span>
            {article.badge && (
              <span className="rounded-full bg-primary/10 border border-primary/20 px-2.5 py-0.5 font-mono text-[11px] text-primary font-medium">
                {article.badge}
              </span>
            )}
          </div>

          <h1 className="text-3xl sm:text-4xl font-bold tracking-tight text-foreground text-balance">
            {article.title}
          </h1>

          <p className="text-base sm:text-lg text-foreground/80 leading-relaxed max-w-3xl">
            {article.description}
          </p>

          <div className="flex flex-wrap items-center gap-4 text-xs font-mono text-muted-foreground pt-1">
            <span className="flex items-center gap-1.5">
              <Clock className="h-3.5 w-3.5 text-primary" />
              {article.readTime}
            </span>
            <span className="hidden sm:inline text-border">|</span>
            <span className="flex items-center gap-1.5">
              <Calendar className="h-3.5 w-3.5 text-primary" />
              Updated {article.lastUpdated}
            </span>
          </div>
        </div>
      </header>

      <div className="space-y-5">
        {article.sections.map((section) => (
          <section
            key={section.id}
            id={section.id}
            className="scroll-mt-32 rounded-2xl border border-border bg-card p-5 sm:p-6 shadow-lg shadow-black/20 space-y-4"
          >
            <h2 className="flex items-center gap-3 text-xl font-semibold tracking-tight text-foreground">
              <span className="h-6 w-1 rounded-full bg-primary shadow-[0_0_12px_color-mix(in_oklch,var(--primary)_70%,transparent)]" />
              <a href={`#${section.id}`} className="hover:text-primary transition-colors">
                {section.title}
              </a>
            </h2>
            <SectionBody section={section} />
          </section>
        ))}
      </div>

      <div className="flex flex-col sm:flex-row items-stretch justify-between gap-4 border-t border-border pt-8 mt-16">
        {prevItem ? (
          <Link
            href={`/docs/${prevItem.slug}`}
            className="group flex flex-col items-start w-full sm:w-1/2 p-4 rounded-xl border border-border bg-card/40 hover:border-primary/50 transition-all"
          >
            <span className="flex items-center gap-1 text-xs font-mono text-muted-foreground group-hover:text-primary transition-colors">
              <ArrowLeft className="h-3.5 w-3.5" /> Previous
            </span>
            <span className="text-sm font-semibold text-foreground mt-1">{prevItem.title}</span>
          </Link>
        ) : (
          <div className="hidden sm:block sm:w-1/2" />
        )}

        {nextItem ? (
          <Link
            href={`/docs/${nextItem.slug}`}
            className="group flex flex-col items-end w-full sm:w-1/2 p-4 rounded-xl border border-border bg-card/40 hover:border-primary/50 transition-all text-right"
          >
            <span className="flex items-center gap-1 text-xs font-mono text-muted-foreground group-hover:text-primary transition-colors">
              Next <ArrowRight className="h-3.5 w-3.5" />
            </span>
            <span className="text-sm font-semibold text-foreground mt-1">{nextItem.title}</span>
          </Link>
        ) : null}
      </div>
    </article>
  )
}

function SectionBody({ section }: { section: DocSection | DocTab }) {
  return (
    <>
      {section.content && <DocsMarkdown text={section.content} />}

      {section.callout && <CalloutBox type={section.callout.type} text={section.callout.text} />}

      {section.codeTabs && section.codeTabs.length > 0 ? (
        <DocsTabs
          tabs={section.codeTabs.map((tab, i) => ({
            id: `${tab.label}-${i}`,
            label: tab.label,
          }))}
          ariaLabel="Code samples"
        >
          {(activeId) => {
            const tab =
              section.codeTabs!.find((t, i) => `${t.label}-${i}` === activeId) || section.codeTabs![0]
            return <CodeBlockWithCopy code={tab.code} language={tab.language} />
          }}
        </DocsTabs>
      ) : (
        section.code && <CodeBlockWithCopy code={section.code.code} language={section.code.language} />
      )}

      {'tabs' in section && section.tabs && section.tabs.length > 0 && (
        <DocsTabs
          tabs={section.tabs.map((tab) => ({ id: tab.id, label: tab.label }))}
          ariaLabel={'title' in section && typeof section.title === 'string' ? section.title : 'Details'}
        >
          {(activeId) => {
            const tab = section.tabs!.find((t) => t.id === activeId) || section.tabs![0]
            return (
              <div className="space-y-3 rounded-xl border border-border bg-background/50 p-4 sm:p-5">
                <SectionBody section={tab} />
              </div>
            )
          }}
        </DocsTabs>
      )}

      {section.params && (
        <div className="space-y-3 pt-1">
          {section.params.map((param) => (
            <ParamCard key={param.name} param={param} />
          ))}
        </div>
      )}

      {section.endpoints && (
        <div className="space-y-4 pt-1">
          {section.endpoints.map((endpoint, idx) => (
            <EndpointCard key={`${endpoint.method}-${endpoint.path}-${idx}`} endpoint={endpoint} />
          ))}
        </div>
      )}

      {section.table && <DataTable table={section.table} />}
    </>
  )
}

function DataTable({ table }: { table: DocTable }) {
  return (
    <div className="overflow-x-auto rounded-xl border border-border bg-card/40 my-2">
      <table className="w-full text-left text-sm">
        <thead className="border-b border-border bg-muted/40 font-mono text-xs text-foreground">
          <tr>
            {table.headers.map((h, i) => (
              <th key={i} className="px-4 py-3 font-semibold">
                {h}
              </th>
            ))}
          </tr>
        </thead>
        <tbody className="divide-y divide-border/60">
          {table.rows.map((row, rIdx) => (
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
    <div className="relative group my-2 rounded-xl border border-border bg-[#0d0c0a] overflow-hidden shadow-lg shadow-black/20">
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

function CalloutBox({ type, text }: { type: DocCallout['type'] | string; text: string }) {
  const styles: Record<
    string,
    { border: string; bg: string; textCol: string; icon: React.ElementType; title: string }
  > = {
    note: { border: 'border-sky-500/40', bg: 'bg-sky-500/10', textCol: 'text-sky-400', icon: Info, title: 'Note' },
    tip: {
      border: 'border-emerald-500/40',
      bg: 'bg-emerald-500/10',
      textCol: 'text-emerald-400',
      icon: Lightbulb,
      title: 'Tip',
    },
    important: {
      border: 'border-primary/40',
      bg: 'bg-primary/10',
      textCol: 'text-primary',
      icon: Flame,
      title: 'Important',
    },
    warning: {
      border: 'border-amber-500/40',
      bg: 'bg-amber-500/10',
      textCol: 'text-amber-400',
      icon: AlertTriangle,
      title: 'Warning',
    },
    caution: {
      border: 'border-red-500/40',
      bg: 'bg-red-500/10',
      textCol: 'text-red-400',
      icon: ShieldAlert,
      title: 'Caution',
    },
  }

  const s = styles[type] || styles.note
  const Icon = s.icon

  return (
    <div className={`my-3 flex items-start gap-3 rounded-xl border ${s.border} ${s.bg} p-4 text-sm`}>
      <Icon className={`h-5 w-5 shrink-0 ${s.textCol} mt-0.5`} />
      <div className="space-y-1">
        <p className={`font-semibold ${s.textCol}`}>{s.title}</p>
        <DocsMarkdown text={text} />
      </div>
    </div>
  )
}

function ParamCard({ param }: { param: DocParam }) {
  return (
    <div className="rounded-xl border border-border bg-card/60 p-4 space-y-2 hover:border-primary/40 transition-colors">
      <div className="flex flex-wrap items-center justify-between gap-2">
        <code className="font-mono text-sm font-bold text-primary">{param.name}</code>
        <div className="flex flex-wrap items-center gap-2 text-xs font-mono">
          <span className="rounded bg-muted px-2 py-0.5 text-muted-foreground">{param.type}</span>
          <span className="rounded border border-primary/30 bg-primary/10 px-2 py-0.5 text-primary">
            default: {param.defaultVal}
          </span>
          {param.range && (
            <span className="rounded bg-muted/60 px-2 py-0.5 text-muted-foreground">range: {param.range}</span>
          )}
        </div>
      </div>
      <DocsMarkdown text={param.description} />
      {param.example && (
        <code className="block rounded bg-[#0f0e0c] px-3 py-1.5 font-mono text-xs text-[#e8dfd1] border border-border/40">
          {param.example}
        </code>
      )}
      {param.note && <p className="text-xs text-muted-foreground/80">{param.note}</p>}
    </div>
  )
}

function EndpointCard({ endpoint }: { endpoint: DocEndpoint }) {
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

      <DocsMarkdown text={endpoint.description} />
      {endpoint.scopes && endpoint.scopes.length > 0 && (
        <p className="text-xs font-mono text-muted-foreground">Scopes: {endpoint.scopes.join(', ')}</p>
      )}

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
