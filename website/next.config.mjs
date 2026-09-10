import { dirname } from 'node:path'
import { fileURLToPath } from 'node:url'

/** @type {import('next').NextConfig} */
const basePath = process.env.NEXT_PUBLIC_BASE_PATH || ''

const nextConfig = {
  output: 'export',
  basePath,
  assetPrefix: basePath ? `${basePath}/` : undefined,
  // The repo root also contains a lockfile for the C++ Web UI; pin the
  // workspace root to this package so Turbopack does not misdetect it.
  turbopack: {
    root: dirname(fileURLToPath(import.meta.url)),
  },
  // Quiet dev-server warnings when previewing through SSH forwards / proxies.
  allowedDevOrigins: ['127.0.0.1', 'localhost'],
  images: {
    unoptimized: true,
  },
}

export default nextConfig
