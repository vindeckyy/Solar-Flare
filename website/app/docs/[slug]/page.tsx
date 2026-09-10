import { notFound } from 'next/navigation'
import Link from 'next/link'
import { ChevronRight, Home, AlignLeft } from 'lucide-react'
import { DOC_ARTICLES } from '@/lib/docs-data'
import { DocsContentRenderer } from '@/components/docs/docs-content-renderer'
import { DocsToc } from '@/components/docs/docs-toc'
import { DocsProgress } from '@/components/docs/docs-progress'

interface PageProps {
  params: Promise<{
    slug: string
  }>
}

export const dynamicParams = false

export async function generateStaticParams() {
  return Object.keys(DOC_ARTICLES).map((slug) => ({
    slug,
  }))
}

export async function generateMetadata({ params }: PageProps) {
  const { slug } = await params
  const article = DOC_ARTICLES[slug]
  if (!article) {
    return { title: 'Documentation | SolarFlare' }
  }

  return {
    title: `${article.title} | SolarFlare Documentation`,
    description: article.description,
    openGraph: {
      title: article.title,
      description: article.description,
      type: 'article',
    },
  }
}

export default async function DocArticlePage({ params }: PageProps) {
  const { slug } = await params
  const article = DOC_ARTICLES[slug]

  if (!article) {
    notFound()
  }

  return (
    <div className="flex gap-10">
      <DocsProgress />

      {/* Article main column */}
      <div className="flex-1 min-w-0 space-y-6">
        {/* Breadcrumb Navigation */}
        <nav className="flex items-center gap-2 text-xs font-mono text-muted-foreground">
          <Link href="/docs" className="hover:text-foreground transition-colors flex items-center gap-1">
            <Home className="h-3.5 w-3.5" />
            <span>Docs</span>
          </Link>
          <ChevronRight className="h-3.5 w-3.5 opacity-60" />
          <span className="text-muted-foreground">{article.category}</span>
          <ChevronRight className="h-3.5 w-3.5 opacity-60" />
          <span className="text-foreground font-semibold truncate">{article.title}</span>
        </nav>

        {/* Collapsible TOC for small screens (sticky sidebar shows on xl+) */}
        {article.sections.length > 1 && (
          <details className="xl:hidden rounded-2xl border border-border bg-card px-4 py-3 shadow-lg shadow-black/20 group">
            <summary className="flex cursor-pointer items-center gap-2 text-xs font-mono font-semibold uppercase tracking-wider text-muted-foreground select-none">
              <AlignLeft className="h-3.5 w-3.5 text-primary" />
              <span>On this page</span>
              <ChevronRight className="ml-auto h-3.5 w-3.5 transition-transform group-open:rotate-90" />
            </summary>
            <nav className="mt-3 space-y-1 border-t border-border pt-3" aria-label="Table of contents">
              {article.sections.map((sec) => (
                <a
                  key={sec.id}
                  href={`#${sec.id}`}
                  className="block py-1 pl-2 text-xs text-muted-foreground hover:text-foreground border-l border-transparent hover:border-primary transition-colors truncate"
                >
                  {sec.title}
                </a>
              ))}
            </nav>
          </details>
        )}

        {/* Rendered Article Content */}
        <DocsContentRenderer article={article} />
      </div>

      {/* Right Column: Sticky Table of Contents */}
      <div className="hidden xl:block w-64 shrink-0 print:hidden">
        <div className="sticky top-[7rem] max-h-[calc(100vh-9rem)] overflow-y-auto rounded-2xl border border-border bg-card p-4 shadow-lg shadow-black/20">
          <DocsToc sections={article.sections} />
        </div>
      </div>
    </div>
  )
}
