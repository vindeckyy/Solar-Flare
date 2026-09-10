import Link from 'next/link'
import { Home, BookOpen } from 'lucide-react'

export default function NotFound() {
  return (
    <div className="min-h-screen bg-background text-foreground flex items-center justify-center px-4">
      <div className="relative overflow-hidden rounded-2xl border border-primary/25 bg-card p-10 text-center max-w-md w-full shadow-[0_0_50px_-14px_color-mix(in_oklch,var(--primary)_50%,transparent)]">
        <div
          className="pointer-events-none absolute inset-0"
          style={{
            background:
              'radial-gradient(500px circle at 50% 0%, color-mix(in oklch, var(--primary) 25%, transparent), transparent 60%)',
          }}
        />
        <div className="relative space-y-4">
          <p className="font-mono text-5xl font-bold text-primary">404</p>
          <h1 className="text-xl font-semibold tracking-tight">Page not found</h1>
          <p className="text-sm text-muted-foreground leading-relaxed">
            The page you are looking for does not exist or was moved. Try the documentation
            portal or head back to the landing page.
          </p>
          <div className="flex items-center justify-center gap-3 pt-2">
            <Link
              href="/docs"
              className="flex items-center gap-2 rounded-lg bg-primary px-4 py-2 text-sm font-semibold text-primary-foreground hover:bg-primary/90 transition-colors"
            >
              <BookOpen className="h-4 w-4" />
              Docs
            </Link>
            <Link
              href="/"
              className="flex items-center gap-2 rounded-lg border border-border bg-background/40 px-4 py-2 text-sm font-medium text-foreground hover:border-primary/50 transition-colors"
            >
              <Home className="h-4 w-4 text-primary" />
              Home
            </Link>
          </div>
        </div>
      </div>
    </div>
  )
}
