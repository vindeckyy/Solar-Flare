/**
 * SolarFlare PWA service worker.
 *
 * Network-first for pages, API, locales, images, and other unhashed files so
 * host updates appear without a hard refresh. Cache-first only for Vite
 * content-hashed JS/CSS under /assets/ (immutable filenames).
 */
const CACHE_NAME = 'solarflare-static-v2';

/**
 * @brief Return true when the path is a Vite content-hashed JS/CSS asset.
 * @param {string} pathname URL pathname to inspect.
 * @return {boolean} Whether the path may be cached immutably.
 */
function isHashedAsset(pathname) {
  return /\/assets\/(?!locale\/).+-[A-Za-z0-9_-]{8,}\.(js|css)$/.test(pathname);
}

self.addEventListener('install', () => {
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(keys.filter((k) => k !== CACHE_NAME).map((k) => caches.delete(k)))
    )
  );
  self.clients.claim();
});

self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url);

  // Only handle same-origin GET requests.
  if (event.request.method !== 'GET' || url.origin !== self.location.origin) {
    return;
  }

  // Let the browser fetch the worker script itself for update checks.
  if (url.pathname.endsWith('/sw.js')) {
    return;
  }

  // Cache-first only for content-hashed build assets.
  if (isHashedAsset(url.pathname)) {
    event.respondWith(
      caches.match(event.request).then(
        (cached) =>
          cached ||
          fetch(event.request).then((response) => {
            if (response.ok) {
              const copy = response.clone();
              caches.open(CACHE_NAME).then((cache) => cache.put(event.request, copy));
            }
            return response;
          })
      )
    );
    return;
  }

  // Network-first for pages, API, locales, images, and other mutable files.
  event.respondWith(
    fetch(event.request)
      .then((response) => {
        // Keep a limited offline fallback for non-API GETs, but never prefer it
        // over a fresh network response.
        if (response.ok && !url.pathname.startsWith('/api/')) {
          const copy = response.clone();
          caches.open(CACHE_NAME).then((cache) => cache.put(event.request, copy));
        }
        return response;
      })
      .catch(() => caches.match(event.request))
  );
});
