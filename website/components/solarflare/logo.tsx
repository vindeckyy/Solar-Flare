import { cn } from '@/lib/utils'

export function Logo({ className }: { className?: string }) {
  return (
    <span className={cn('flex items-center gap-2.5', className)}>
      <span className="relative flex h-7 w-7 items-center justify-center">
        <svg
          viewBox="0 0 32 32"
          fill="none"
          className="h-7 w-7 text-primary"
          aria-hidden="true"
        >
          <circle cx="16" cy="16" r="6" fill="currentColor" />
          <g stroke="currentColor" strokeWidth="1.6" strokeLinecap="round">
            <path d="M16 1.5v5" />
            <path d="M16 25.5v5" />
            <path d="M1.5 16h5" />
            <path d="M25.5 16h5" />
            <path d="M5.8 5.8l3.5 3.5" />
            <path d="M22.7 22.7l3.5 3.5" />
            <path d="M26.2 5.8l-3.5 3.5" />
            <path d="M9.3 22.7l-3.5 3.5" />
          </g>
        </svg>
      </span>
      <span className="font-mono text-[15px] font-semibold tracking-tight text-foreground">
        Solar<span className="text-primary">Flare</span>
      </span>
    </span>
  )
}
