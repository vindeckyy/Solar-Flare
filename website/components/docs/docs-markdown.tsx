import Link from 'next/link'
import { type ReactNode } from 'react'

function isSafeHref(href: string): boolean {
  return (
    href.startsWith('https://') ||
    href.startsWith('http://') ||
    href.startsWith('/') ||
    href.startsWith('#')
  )
}

function renderInline(text: string, keyPrefix: string): ReactNode[] {
  const nodes: ReactNode[] = []
  const re = /(\*\*[^*]+\*\*|`[^`]+`|\[[^\]]+\]\([^)]+\))/g
  let last = 0
  let match: RegExpExecArray | null
  let i = 0

  while ((match = re.exec(text)) !== null) {
    if (match.index > last) {
      nodes.push(text.slice(last, match.index))
    }

    const token = match[0]
    if (token.startsWith('**')) {
      nodes.push(
        <strong key={`${keyPrefix}-b-${i}`} className="font-semibold text-foreground">
          {token.slice(2, -2)}
        </strong>,
      )
    } else if (token.startsWith('`')) {
      nodes.push(
        <code
          key={`${keyPrefix}-c-${i}`}
          className="rounded-md border border-border/80 bg-muted px-1.5 py-0.5 font-mono text-[0.82em] text-primary"
        >
          {token.slice(1, -1)}
        </code>,
      )
    } else {
      const link = /^\[([^\]]+)\]\(([^)]+)\)$/.exec(token)
      if (link && isSafeHref(link[2])) {
        const href = link[2]
        const external = href.startsWith('http')
        nodes.push(
          <Link
            key={`${keyPrefix}-l-${i}`}
            href={href}
            className="font-medium text-primary underline-offset-4 hover:underline"
            {...(external ? { target: '_blank', rel: 'noreferrer' } : {})}
          >
            {link[1]}
          </Link>,
        )
      } else {
        nodes.push(token)
      }
    }

    last = match.index + token.length
    i += 1
  }

  if (last < text.length) {
    nodes.push(text.slice(last))
  }

  return nodes
}

function headingLevel(line: string): { level: number; text: string } | null {
  const match = /^(#{1,4})\s+(.+)$/.exec(line)
  if (!match) {
    return null
  }
  return { level: match[1].length, text: match[2] }
}

/**
 * @brief Render a subset of Markdown used in docs-data article bodies.
 */
export function DocsMarkdown({ text }: { text: string }) {
  const blocks: ReactNode[] = []
  const lines = text.replace(/\r\n/g, '\n').split('\n')
  let i = 0
  let block = 0

  while (i < lines.length) {
    const line = lines[i]

    if (line.trim() === '') {
      i += 1
      continue
    }

    if (line.startsWith('```')) {
      const lang = line.slice(3).trim() || 'text'
      const body: string[] = []
      i += 1
      while (i < lines.length && !lines[i].startsWith('```')) {
        body.push(lines[i])
        i += 1
      }
      if (i < lines.length) {
        i += 1
      }
      blocks.push(
        <pre
          key={`fence-${block}`}
          className="overflow-x-auto rounded-lg border border-border bg-[#0d0c0a] p-3 font-mono text-xs text-[#f3ede2]"
        >
          <code data-language={lang}>{body.join('\n')}</code>
        </pre>,
      )
      block += 1
      continue
    }

    const heading = headingLevel(line)
    if (heading) {
      const className =
        heading.level <= 2
          ? 'text-lg font-semibold tracking-tight text-foreground'
          : 'text-sm font-semibold uppercase tracking-wider text-primary'
      const Tag = (heading.level <= 2 ? 'h3' : 'h4') as 'h3' | 'h4'
      blocks.push(
        <Tag key={`h-${block}`} className={`${className} pt-1`}>
          {renderInline(heading.text, `h-${block}`)}
        </Tag>,
      )
      i += 1
      block += 1
      continue
    }

    const isUl = /^[-*]\s+/.test(line)
    const isOl = /^\d+\.\s+/.test(line)
    if (isUl || isOl) {
      const items: string[] = []
      while (i < lines.length && (isUl ? /^[-*]\s+/.test(lines[i]) : /^\d+\.\s+/.test(lines[i]))) {
        items.push(lines[i].replace(/^([-*]|\d+\.)\s+/, ''))
        i += 1
      }
      const ListTag = isOl ? 'ol' : 'ul'
      blocks.push(
        <ListTag
          key={`list-${block}`}
          className={
            isOl
              ? 'list-decimal space-y-2 pl-5 text-[15px] leading-relaxed text-foreground/90'
              : 'list-disc space-y-2 pl-5 text-[15px] leading-relaxed text-foreground/90'
          }
        >
          {items.map((item, idx) => (
            <li key={idx}>{renderInline(item, `li-${block}-${idx}`)}</li>
          ))}
        </ListTag>,
      )
      block += 1
      continue
    }

    const para: string[] = []
    while (
      i < lines.length &&
      lines[i].trim() !== '' &&
      !headingLevel(lines[i]) &&
      !lines[i].startsWith('```') &&
      !/^[-*]\s+/.test(lines[i]) &&
      !/^\d+\.\s+/.test(lines[i])
    ) {
      para.push(lines[i])
      i += 1
    }
    blocks.push(
      <p key={`p-${block}`} className="text-[15px] leading-relaxed text-foreground/90">
        {renderInline(para.join(' '), `p-${block}`)}
      </p>,
    )
    block += 1
  }

  return <div className="space-y-3">{blocks}</div>
}
