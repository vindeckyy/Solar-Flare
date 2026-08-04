<template>
  <div v-if="visible" class="alert alert-danger solarflare-update-banner mb-0" :class="bannerClass" role="alert">
    <div class="d-flex align-items-center justify-content-between gap-3 flex-wrap">
      <div class="d-flex align-items-center gap-3 flex-grow-1 flex-wrap">
        <AlertOctagon :size="iconSize" class="solarflare-update-icon flex-shrink-0"></AlertOctagon>
        <div class="flex-grow-1" style="min-width: 12rem;">
          <div class="solarflare-update-eyebrow">{{ $t('index.outdated_eyebrow') }}</div>
          <div class="solarflare-update-title" :class="titleClass">
            {{ $t('index.outdated_title', { installed: installedVersion, latest: latestVersion }) }}
          </div>
          <div v-if="statusBarVisible" class="sf-update-status mt-2">
            <div class="d-flex align-items-center justify-content-between gap-2 mb-1">
              <span class="sf-update-phase">{{ phaseLabel }}</span>
              <span v-if="updateStatus && updateStatus.percent >= 0" class="sf-update-percent">{{ updateStatus.percent }}%</span>
            </div>
            <div class="progress sf-update-progress" role="progressbar"
                 :aria-valuenow="progressValue" aria-valuemin="0" aria-valuemax="100">
              <div class="progress-bar"
                   :class="{ 'progress-bar-striped progress-bar-animated': indeterminate }"
                   :style="{ width: progressWidth }"></div>
            </div>
            <div v-if="updateStatus && updateStatus.message" class="sf-update-message mt-1">{{ updateStatus.message }}</div>
          </div>
        </div>
      </div>
      <div class="d-flex align-items-center gap-2 flex-shrink-0 flex-wrap">
        <button type="button" class="btn btn-link btn-sm sf-update-terminal-toggle text-decoration-none"
                :aria-expanded="terminalOpen ? 'true' : 'false'"
                :aria-label="$t('index.update_toggle_terminal')"
                @click="toggleTerminal">
          <ChevronDown v-if="!terminalOpen" :size="18" class="icon"></ChevronDown>
          <ChevronUp v-else :size="18" class="icon"></ChevronUp>
        </button>
        <button type="button" class="btn btn-danger"
                :disabled="updateBusy"
                @click="onUpdateNow">
          {{ primaryButtonLabel }}
        </button>
        <button v-if="canCancel"
                type="button"
                class="btn btn-outline-light"
                :disabled="cancelInFlight"
                @click="onCancelUpdate">
          {{ $t('index.update_cancel') }}
        </button>
        <a v-if="htmlUrl" class="btn btn-outline-light btn-sm" :href="htmlUrl" target="_blank" rel="noopener">
          {{ $t('index.release_notes_short') }}
        </a>
        <button v-if="dismissable" type="button" class="btn-close"
                :aria-label="$t('_common.dismiss')"
                @click="$emit('dismiss', latestVersion)"></button>
      </div>
    </div>

    <div v-show="terminalOpen" class="sf-update-terminal mt-3" aria-live="polite">
      <pre ref="terminal" class="sf-update-terminal-body mb-0">{{ terminalText }}</pre>
    </div>
  </div>
</template>

<script>
import { AlertOctagon, ChevronDown, ChevronUp } from '@lucide/vue'
import { apiFetch } from './fetch_utils'

const TERMINAL_KEY = 'solarflare.update-terminal-open.v1'

/**
 * Shared SolarFlare update banner with status bar and expandable command log.
 */
export default {
  name: 'UpdateBanner',
  components: {
    AlertOctagon,
    ChevronDown,
    ChevronUp,
  },
  props: {
    installedVersion: { type: String, default: '' },
    latestVersion: { type: String, default: '' },
    htmlUrl: { type: String, default: '' },
    visible: { type: Boolean, default: false },
    dismissable: { type: Boolean, default: false },
    compact: { type: Boolean, default: false },
  },
  emits: ['dismiss'],
  data() {
    return {
      updateStatus: null,
      terminalOpen: sessionStorage.getItem(TERMINAL_KEY) === '1',
      pollTimer: null,
      actionError: '',
      cancelInFlight: false,
    }
  },
  computed: {
    iconSize() {
      return this.compact ? 28 : 32
    },
    bannerClass() {
      return this.compact ? 'solarflare-global-update-banner' : 'my-3'
    },
    titleClass() {
      return this.compact ? '' : 'h4 mb-0'
    },
    statusBarVisible() {
      return !!(this.updateStatus && this.updateStatus.phase && this.updateStatus.phase !== 'idle')
    },
    updateBusy() {
      return !!(this.updateStatus && this.updateStatus.busy)
    },
    canCancel() {
      return this.updateStatus?.phase === 'waiting_idle'
    },
    indeterminate() {
      return !this.updateStatus || this.updateStatus.percent < 0
    },
    progressValue() {
      if (!this.updateStatus || this.updateStatus.percent < 0) {
        return 100
      }
      return this.updateStatus.percent
    },
    progressWidth() {
      if (this.indeterminate) {
        return '100%'
      }
      return `${this.progressValue}%`
    },
    phaseLabel() {
      const phase = this.updateStatus?.phase || 'idle'
      const key = `index.update_phase_${phase}`
      return this.$te(key) ? this.$t(key) : phase
    },
    primaryButtonLabel() {
      if (this.updateStatus?.phase === 'ready') {
        return this.$t('index.update_apply')
      }
      if (this.updateStatus?.phase === 'waiting_idle') {
        return this.$t('index.update_waiting_idle')
      }
      if (this.updateBusy) {
        return this.$t('index.update_working')
      }
      return this.$t('index.update_now')
    },
    terminalText() {
      const lines = this.updateStatus?.log || []
      if (this.actionError) {
        return [...lines, `# error: ${this.actionError}`].join('\n')
      }
      if (!lines.length) {
        return this.$t('index.update_terminal_empty')
      }
      return lines.join('\n')
    },
  },
  watch: {
    terminalText() {
      this.$nextTick(() => {
        const el = this.$refs.terminal
        if (el) {
          el.scrollTop = el.scrollHeight
        }
      })
    },
    visible(val) {
      if (val) {
        this.refreshStatus()
      }
    },
  },
  mounted() {
    if (this.visible) {
      this.refreshStatus()
    }
  },
  beforeUnmount() {
    this.stopPoll()
  },
  methods: {
    toggleTerminal() {
      this.terminalOpen = !this.terminalOpen
      sessionStorage.setItem(TERMINAL_KEY, this.terminalOpen ? '1' : '0')
      if (this.terminalOpen) {
        this.refreshStatus()
      }
    },
    stopPoll() {
      if (this.pollTimer) {
        clearInterval(this.pollTimer)
        this.pollTimer = null
      }
    },
    startPoll() {
      if (this.pollTimer) {
        return
      }
      this.pollTimer = setInterval(() => this.refreshStatus(), 500)
    },
    async refreshStatus() {
      try {
        const r = await fetch('./api/update')
        if (!r.ok) {
          return
        }
        this.updateStatus = await r.json()
        if (this.updateStatus?.busy || this.updateStatus?.phase === 'waiting_idle') {
          this.startPoll()
        }
        else {
          this.stopPoll()
        }
      }
      catch (e) {
        console.error('UpdateBanner: status poll failed', e)
      }
    },
    async onUpdateNow() {
      this.actionError = ''
      const phase = this.updateStatus?.phase
      try {
        if (phase === 'ready' || phase === 'waiting_idle') {
          const r = await apiFetch('./api/update/apply', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ when_idle: true }),
          })
          const body = await r.json().catch(() => ({}))
          if (!r.ok) {
            this.actionError = body.error || r.statusText
            this.terminalOpen = true
            return
          }
          this.updateStatus = body
        }
        else {
          const r = await apiFetch('./api/update/start', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: '{}',
          })
          const body = await r.json().catch(() => ({}))
          if (!r.ok) {
            this.actionError = body.error || r.statusText
            this.terminalOpen = true
            return
          }
          this.updateStatus = body
        }
        this.terminalOpen = true
        sessionStorage.setItem(TERMINAL_KEY, '1')
        this.startPoll()
      }
      catch (e) {
        this.actionError = String(e)
        this.terminalOpen = true
      }
    },
    async onCancelUpdate() {
      if (this.cancelInFlight) {
        return
      }
      this.cancelInFlight = true
      this.actionError = ''
      try {
        const r = await apiFetch('./api/update/cancel', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: '{}',
        })
        const body = await r.json().catch(() => ({}))
        if (!r.ok) {
          this.actionError = body.error || r.statusText
          this.terminalOpen = true
          return
        }
        this.updateStatus = body
        this.startPoll()
      }
      catch (e) {
        this.actionError = String(e)
        this.terminalOpen = true
      }
      finally {
        this.cancelInFlight = false
      }
    },
  },
}
</script>
