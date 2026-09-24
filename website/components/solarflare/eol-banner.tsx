import { TriangleAlert } from 'lucide-react'

/**
 * Site-wide end-of-life notice. Rendered at the top of every page via the
 * root layout. SolarFlare reached end of life in September 2026 and the
 * project is no longer maintained.
 */
export function EolBanner() {
  return (
    <div
      role="note"
      aria-label="End of life notice"
      className="border-b border-destructive/30 bg-destructive/10"
    >
      <div className="mx-auto flex max-w-6xl items-start gap-3 px-4 py-3 md:items-center md:px-6">
        <TriangleAlert
          className="mt-0.5 h-5 w-5 shrink-0 text-destructive md:mt-0"
          aria-hidden="true"
        />
        <p className="text-sm leading-relaxed text-foreground">
          <strong className="font-semibold">End of life — September 2026.</strong>{' '}
          <span className="text-muted-foreground">
            SolarFlare is no longer maintained and this site is kept for
            reference only. No further releases, updates, bug fixes, or
            security patches will be published. Users are encouraged to migrate
            to{' '}
          </span>
          <a
            href="https://github.com/LizardByte/Sunshine"
            className="font-medium text-primary underline underline-offset-4 hover:text-primary/80"
          >
            LizardByte/Sunshine
          </a>
          <span className="text-muted-foreground">
            , the upstream project, which remains actively maintained.
          </span>
        </p>
      </div>
    </div>
  )
}
