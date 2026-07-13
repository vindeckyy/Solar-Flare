<template>
  <div>
    <!--
      SolarFlare fork: global "update available" banner. Shown on EVERY page
      that includes Navbar.vue (apps, config, featured, pin, home, ...) so
      the user can't avoid it by navigating away from the home page.
      Hard to dismiss on purpose: clicking the dismiss button only hides it
      for the current installed version, not for any future version bumps.
    -->
    <div v-if="bannerVisible" class="solarflare-global-update-banner alert alert-danger d-flex align-items-center gap-3 mb-0" role="alert">
      <AlertOctagon :size="28" class="solarflare-update-icon flex-shrink-0"></AlertOctagon>
      <div class="flex-grow-1">
        <div class="solarflare-update-eyebrow">{{ $t('index.outdated_eyebrow') }}</div>
        <div class="solarflare-update-title">
          {{ $t('index.outdated_title', { installed: installedVersion, latest: updateInfo.latestVersion }) }}
        </div>
      </div>
      <a class="btn btn-danger flex-shrink-0" :href="updateInfo.htmlUrl" target="_blank">
        {{ $t('index.download') }}
      </a>
      <button type="button" class="btn-close flex-shrink-0" :aria-label="$t('_common.dismiss')" @click="dismissForVersion(updateInfo.latestVersion)"></button>
    </div>

    <nav class="navbar navbar-expand-lg navbar-sunshine">
      <div class="container-fluid">
        <a class="navbar-brand solarflare-brand" href="./" title="SolarFlare">
          <img src="/images/logo-solarflare-45.svg" height="40" alt="SolarFlare" class="solarflare-logo">
          <span class="solarflare-wordmark">SolarFlare</span>
        </a>
        <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarSupportedContent"
                aria-controls="navbarSupportedContent" aria-expanded="false" aria-label="Toggle navigation">
          <span class="navbar-toggler-icon"></span>
        </button>
        <div class="collapse navbar-collapse" id="navbarSupportedContent">
          <ul class="navbar-nav me-auto mb-2 mb-lg-0">
            <li class="nav-item">
              <a class="nav-link" href="./">
                <Home :size="18" class="icon"></Home>
                {{ $t('navbar.home') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./pin">
                <Lock :size="18" class="icon"></Lock>
                {{ $t('navbar.pin') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./apps">
                <Layers :size="18" class="icon"></Layers>
                {{ $t('navbar.applications') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./featured">
                <Star :size="18" class="icon"></Star>
                {{ $t('navbar.featured') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./config">
                <Settings :size="18" class="icon"></Settings>
                {{ $t('navbar.configuration') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./troubleshooting">
                <Info :size="18" class="icon"></Info>
                {{ $t('navbar.troubleshoot') }}
              </a>
            </li>
          </ul>
          <ul class="navbar-nav ms-auto mb-2 mb-lg-0">
            <li class="nav-item">
              <ThemeToggle/>
            </li>
            <li class="nav-item dropdown">
              <button class="nav-link dropdown-toggle" type="button" id="navbarUserMenu"
                      data-bs-toggle="dropdown" aria-expanded="false" aria-label="User menu" title="User menu">
                <CircleUserRound :size="18" class="icon"></CircleUserRound>
              </button>
              <ul class="dropdown-menu dropdown-menu-end" aria-labelledby="navbarUserMenu">
                <li>
                  <a class="dropdown-item d-flex align-items-center" href="./password">
                    <Shield :size="18" class="icon"></Shield>
                    {{ $t('navbar.password') }}
                  </a>
                </li>
                <li><hr class="dropdown-divider"></li>
                <li>
                  <button type="button" class="dropdown-item d-flex align-items-center" @click="logout">
                    <LogOut :size="18" class="icon"></LogOut>
                    {{ $t('navbar.logout') }}
                  </button>
                </li>
              </ul>
            </li>
          </ul>
        </div>
      </div>
    </nav>
    <Notification></Notification>
    <CommandPalette></CommandPalette>
  </div>
</template>

<script>
import { AlertOctagon, CircleUserRound, Home, Info, Layers, Lock, LogOut, Settings, Shield, Star } from '@lucide/vue'
import ThemeToggle from './ThemeToggle.vue'
import Notification from './Notification.vue'
import CommandPalette from './CommandPalette.vue'
import { checkForUpdate } from './sunshine_version'

// localStorage key for "user has dismissed the banner for this specific
// latest version". A new version bump resets the dismissal so the banner
// shows again. Stored separately from the 24-hour update-cache so
// dismissals survive a cache expiry.
const DISMISS_KEY = 'solarflare.update-dismiss.v1'

export default {
  components: {
    ThemeToggle,
    Notification,
    CommandPalette,
    AlertOctagon,
    Home,
    Lock,
    Layers,
    Star,
    Settings,
    Shield,
    Info,
    CircleUserRound,
    LogOut
  },
  data() {
    return {
      updateInfo: null,
      installedVersion: '',
      _dismissedFor: this._readDismissedFor(),
    }
  },
  computed: {
    // Re-evaluate when _dismissedFor changes so the v-if flips back on
    // after the user clicks the dismiss button.
    bannerVisible() {
      return this.updateInfo && this.updateInfo.outdated && this._dismissedFor !== this.updateInfo.latestVersion
    }
  },
  async created() {
    // Pull the installed version from /api/config so we don't have to
    // accept it as a prop. This makes the banner self-contained: any
    // page that renders Navbar.vue gets the update check for free.
    try {
      const config = await fetch('./api/config').then((r) => r.json())
      this.installedVersion = config.version || ''
      this.updateInfo = await checkForUpdate(this.installedVersion)
    } catch (e) {
      console.error('Navbar: update check failed:', e)
    }
  },
  mounted() {
    const currentPath = globalThis.location.pathname.replace(/\/$/, '') || '/'
    const links = document.querySelectorAll('.navbar-sunshine a[href]')

    for (const link of links) {
      const href = link.getAttribute('href')
      if (!href || href === '#') {
        continue
      }

      const linkPath = new URL(href, globalThis.location.href).pathname.replace(/\/$/, '') || '/'
      if (linkPath !== currentPath) {
        continue
      }

      link.classList.add('active')
    }
  },
  methods: {
    _readDismissedFor() {
      try { return localStorage.getItem(DISMISS_KEY) || '' } catch (e) { return '' }
    },
    /**
     * Dismiss the banner for the currently-displayed latest version.
     * Any future version bump resets this (since the stored value no
     * longer matches the new `latestVersion`), so the banner comes
     * back automatically when there's something genuinely new.
     */
    dismissForVersion(version) {
      try { localStorage.setItem(DISMISS_KEY, version) } catch (e) { /* ignore quota errors */ }
      this._dismissedFor = version
    },
    logout() {
      const cacheBuster = Date.now().toString()
      const logoutPageUrl = new URL('/logout', globalThis.location.href)
      const request = new XMLHttpRequest()
      const finish = () => {
        globalThis.location.replace(logoutPageUrl.toString())
      }

      request.open('GET', '/', true, 'sunshine-logout', cacheBuster)
      request.setRequestHeader('Cache-Control', 'no-store')
      request.onload = finish
      request.onerror = finish
      request.ontimeout = finish
      request.timeout = 5000
      request.send()
    }
  }
}
</script>

<style>
/* Navbar toggler icon for dark text on light background */
.navbar-sunshine .navbar-toggler-icon {
  --bs-navbar-toggler-icon-bg: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 30 30'%3e%3cpath stroke='rgba%28255, 255, 255, 0.9%29' stroke-linecap='round' stroke-miterlimit='10' stroke-width='2' d='M4 7h22M4 15h22M4 23h22'/%3e%3c/svg%3e") !important;
}

/* SolarFlare brand: SVG logo + bold wordmark in the navbar */
.solarflare-brand {
  display: inline-flex !important;
  align-items: center;
  gap: 0.6rem;
  padding-top: 0.25rem;
  padding-bottom: 0.25rem;
}
.solarflare-logo {
  filter: drop-shadow(0 0 4px rgba(251, 191, 36, 0.35));
  transition: transform var(--transition-base), filter var(--transition-base);
}
.solarflare-brand:hover .solarflare-logo {
  transform: rotate(-6deg) scale(1.05);
  filter: drop-shadow(0 0 8px rgba(251, 191, 36, 0.6));
}
.solarflare-wordmark {
  font-weight: 700;
  font-size: 1.25rem;
  letter-spacing: -0.01em;
  color: var(--navbar-text);
  background: linear-gradient(90deg, #FBBF24 0%, #FB923C 50%, #FDBA74 100%);
  -webkit-background-clip: text;
  background-clip: text;
  -webkit-text-fill-color: transparent;
  color: transparent;
}

/* Glassmorphism navbar (subtle, only on wide screens) */
@supports (backdrop-filter: blur(12px)) or (-webkit-backdrop-filter: blur(12px)) {
  .navbar-sunshine {
    backdrop-filter: blur(12px) saturate(140%);
    -webkit-backdrop-filter: blur(12px) saturate(140%);
  }
}
</style>
