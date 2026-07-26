import { cn } from '@/lib/utils'

const basePath = process.env.NEXT_PUBLIC_BASE_PATH || ''

export function Logo({ className }: { className?: string }) {
  return (
    <img
      src={`${basePath}/solarflare-lockup-transparent.svg`}
      width="640"
      height="188"
      alt="SolarFlare — Host Observatory"
      className={cn('h-10 w-auto', className)}
    />
  )
}
