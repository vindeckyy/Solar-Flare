<template>
  <div class="sf-chrome">
    <!--
      SolarFlare fork: global "update available" banner. Shown on EVERY page
      that includes Navbar.vue so the user can't avoid it by navigating away
      from the home page. Hard to dismiss on purpose: clicking dismiss only
      hides it for the current latest version.
    -->
    <UpdateBanner
      :visible="bannerVisible"
      :installed-version="installedVersion"
      :latest-version="updateInfo ? updateInfo.latestVersion : ''"
      :html-url="updateInfo ? updateInfo.htmlUrl : ''"
      :dismissable="true"
      :compact="true"
      @dismiss="dismissForVersion"
    ></UpdateBanner>

    <nav class="navbar navbar-expand-lg navbar-sunshine" aria-label="Primary navigation">
      <div class="container-fluid sf-nav-frame">
        <a class="navbar-brand solarflare-brand" href="./" title="SolarFlare home">
          <span class="sf-brand-orbit" aria-hidden="true"></span>
          <img src="/images/solarflare-mark.svg" height="42" alt="" class="solarflare-logo">
          <span class="sf-brand-copy">
            <span class="solarflare-wordmark">SolarFlare</span>
            <span class="sf-brand-descriptor">Game streaming host</span>
          </span>
        </a>
        <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarSupportedContent"
                aria-controls="navbarSupportedContent" aria-expanded="false" aria-label="Toggle navigation">
          <span class="navbar-toggler-icon" aria-hidden="true"></span>
        </button>
        <div class="collapse navbar-collapse sf-nav-content" id="navbarSupportedContent">
          <div class="sf-nav-section-label">Vector</div>
          <ul class="navbar-nav me-auto mb-2 mb-lg-0 sf-primary-nav">
            <li class="nav-item">
              <a class="nav-link" href="./" data-nav-code="01">
                <Home :size="18" class="icon"></Home>
                {{ $t('navbar.home') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./pin" data-nav-code="02">
                <Lock :size="18" class="icon"></Lock>
                {{ $t('navbar.pin') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./apps" data-nav-code="03">
                <Layers :size="18" class="icon"></Layers>
                {{ $t('navbar.applications') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./featured" data-nav-code="04">
                <Star :size="18" class="icon"></Star>
                {{ $t('navbar.featured') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./config" data-nav-code="05">
                <Settings :size="18" class="icon"></Settings>
                {{ $t('navbar.configuration') }}
              </a>
            </li>
            <li class="nav-item">
              <a class="nav-link" href="./troubleshooting" data-nav-code="06">
                <Info :size="18" class="icon"></Info>
                {{ $t('navbar.troubleshoot') }}
              </a>
            </li>
          </ul>
          <div class="sf-nav-utility">
            <div class="sf-node-readout" aria-label="Local host connection">
              <span class="sf-node-signal" aria-hidden="true"></span>
              <span>
                <strong>LOCAL NODE</strong>
                <small>CONTROL LINK</small>
              </span>
            </div>
          <ul class="navbar-nav ms-auto mb-2 mb-lg-0 sf-utility-nav">
            <li class="nav-item">
              <ThemeToggle/>
            </li>
            <li class="nav-item dropdown dropend">
              <button class="nav-link dropdown-toggle" type="button" id="navbarUserMenu"
                      data-bs-toggle="dropdown" aria-expanded="false" aria-haspopup="true" aria-label="User menu" title="User menu">
                <CircleUserRound :size="18" class="icon" aria-hidden="true"></CircleUserRound>
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
      </div>
    </nav>
    <Notification></Notification>
    <CommandPalette></CommandPalette>
  </div>
</template>

<script>
import { CircleUserRound, Home, Info, Layers, Lock, LogOut, Settings, Shield, Star } from '@lucide/vue'
import ThemeToggle from './ThemeToggle.vue'
import Notification from './Notification.vue'
import CommandPalette from './CommandPalette.vue'
import UpdateBanner from './UpdateBanner.vue'
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
    UpdateBanner,
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
    const currentPath = globalThis.location.pathname.replace(/\/$/, "") || "/"
    const links = document.querySelectorAll(".navbar-sunshine a[href]")

    for (const link of links) {
      const href = link.getAttribute("href")
      if (!href || href === "#") {
        continue
      }

      const linkPath = new URL(href, globalThis.location.href).pathname.replace(/\/$/, "") || "/"
      if (linkPath !== currentPath) {
        continue
      }

      link.classList.add("active")
      link.setAttribute("aria-current", "page")
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
/**
 * @brief Navbar-specific overrides; global layout lives in sunshine.css.
 */
.navbar-sunshine .nav-link:focus-visible,
.navbar-sunshine .navbar-brand:focus-visible,
.navbar-sunshine .navbar-toggler:focus-visible {
  outline: 2px solid var(--color-primary, #ffad42);
  outline-offset: 2px;
}

/* Navbar toggler icon for the compact mobile control bar. */
.navbar-sunshine .navbar-toggler-icon {
  --bs-navbar-toggler-icon-bg: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 30 30'%3e%3cpath stroke='rgba%28255, 255, 255, 0.9%29' stroke-linecap='round' stroke-miterlimit='10' stroke-width='2' d='M4 7h22M4 15h22M4 23h22'/%3e%3c/svg%3e") !important;
}

/* SolarFlare wordmark. Layout is defined in sunshine.css so the
 * shared rail and responsive control bar remain consistent on every page. */
.solarflare-brand {
  display: inline-flex !important;
  align-items: center;
  gap: 0.6rem;
  padding-top: 0.25rem;
  padding-bottom: 0.25rem;
}
.solarflare-logo {
  position: relative;
  z-index: 1;
  filter: drop-shadow(0 0 10px rgba(255, 174, 66, 0.38));
  transition: transform var(--transition-base), filter var(--transition-base);
}
.solarflare-brand:hover .solarflare-logo {
  transform: rotate(-8deg);
  filter: drop-shadow(0 0 14px rgba(255, 174, 66, 0.58));
}
.solarflare-wordmark {
  display: block;
  font-weight: 760;
  font-size: 1.18rem;
  letter-spacing: -0.035em;
  color: var(--navbar-text);
}
</style>
