import { cn } from '@/lib/utils'

export function CodeBlock({
  title,
  lines,
  className,
}: {
  title?: string
  lines: string[]
  className?: string
}) {
  return (
    <div
      className={cn(
        'min-w-0 overflow-hidden rounded-xl border border-border bg-background',
        className,
      )}
    >
      {title && (
        <div className="flex items-center gap-2 border-b border-border bg-card/60 px-4 py-2.5">
          <span className="h-2.5 w-2.5 rounded-full bg-muted-foreground/30" />
          <span className="font-mono text-xs text-muted-foreground">
            {title}
          </span>
        </div>
      )}
      <pre className="overflow-x-auto p-4 font-mono text-[13px] leading-relaxed">
        <code>
          {lines.map((line, i) => (
            <span key={i} className="grid grid-cols-[2ch_1fr] gap-4">
              <span className="select-none text-right text-muted-foreground/40">
                {i + 1}
              </span>
              <span className="text-foreground/90">{line || '\u00A0'}</span>
            </span>
          ))}
        </code>
      </pre>
    </div>
  )
}
