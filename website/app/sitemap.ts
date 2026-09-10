import type { MetadataRoute } from 'next'
import { DOC_ARTICLES } from '@/lib/docs-data'

const SITE_URL = 'https://vindeckyy.github.io/Solar-Flare'

export const dynamic = 'force-static'

export default function sitemap(): MetadataRoute.Sitemap {
  const routes = ['', '/docs', ...Object.keys(DOC_ARTICLES).map((slug) => `/docs/${slug}`)]

  return routes.map((route) => ({
    url: `${SITE_URL}${route}`,
    changeFrequency: route === '' ? 'monthly' : 'weekly',
    priority: route === '' ? 1 : route === '/docs' ? 0.9 : 0.7,
  }))
}
