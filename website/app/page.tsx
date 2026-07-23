import { SiteHeader } from '@/components/solarflare/site-header'
import { Hero } from '@/components/solarflare/hero'
import { Overview } from '@/components/solarflare/overview'
import { Features } from '@/components/solarflare/features'
import { Architecture } from '@/components/solarflare/architecture'
import { ControlSurface } from '@/components/solarflare/control-surface'
import { Configuration } from '@/components/solarflare/configuration'
import { AudioFx } from '@/components/solarflare/audio-fx'
import { Install } from '@/components/solarflare/install'
import { SiteFooter } from '@/components/solarflare/site-footer'

export default function Page() {
  return (
    <div className="min-h-screen bg-background">
      <SiteHeader />
      <main>
        <Hero />
        <Overview />
        <Features />
        <Architecture />
        <ControlSurface />
        <Configuration />
        <AudioFx />
        <Install />
      </main>
      <SiteFooter />
    </div>
  )
}
