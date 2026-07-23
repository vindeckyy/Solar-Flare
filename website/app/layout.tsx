import { Analytics } from '@vercel/analytics/next'
import type { Metadata, Viewport } from 'next'
import { Geist, Geist_Mono } from 'next/font/google'
import './globals.css'

const geistSans = Geist({
  subsets: ['latin'],
  variable: '--font-geist-sans',
})

const geistMono = Geist_Mono({
  subsets: ['latin'],
  variable: '--font-geist-mono',
})

export const metadata: Metadata = {
  title: 'SolarFlare — A precision game-streaming host for Moonlight',
  description:
    'SolarFlare is a self-hosted, Linux & AMD-first game-streaming host for Moonlight, with an observatory-style Web UI, low-latency transport tuning, and advanced host controls. Own the host. Instrument the path. Stream without the cloud.',
  keywords: [
    'SolarFlare',
    'Moonlight',
    'Sunshine',
    'game streaming',
    'self-hosted',
    'low latency',
    'Linux',
    'AMD',
    'remote desktop',
  ],
  openGraph: {
    title: 'SolarFlare — A precision game-streaming host for Moonlight',
    description:
      'Linux & AMD-first capture, transport, and host control engineered for predictable local-network latency.',
    type: 'website',
  },
}

export const viewport: Viewport = {
  colorScheme: 'dark',
  themeColor: '#1a1712',
}

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode
}>) {
  return (
    <html lang="en" className={`dark ${geistSans.variable} ${geistMono.variable}`}>
      <body className="antialiased bg-background font-sans">
        {children}
        {process.env.NODE_ENV === 'production' && <Analytics />}
      </body>
    </html>
  )
}
