<template>
  <div class="card my-4 sf-telemetry">
    <div class="card-body">
      <div class="d-flex align-items-center justify-content-between mb-3">
        <div>
          <h2 class="mb-0">{{ $t('index.telemetry_title') }}</h2>
          <small class="text-muted">{{ $t('index.telemetry_desc') }}</small>
        </div>
        <span v-if="hasSamples" class="badge text-bg-success">{{ $t('index.telemetry_live') }}</span>
      </div>

      <div v-if="!hasSamples" class="text-muted">
        {{ $t('index.telemetry_unavailable') }}
      </div>

      <div v-else class="row g-3">
        <div v-for="m in metrics" :key="m.key" class="col-md-4 col-sm-6">
          <div class="border rounded p-3">
            <div class="d-flex justify-content-between align-items-baseline mb-1">
              <small class="text-muted">{{ m.label }}</small>
              <strong class="fs-5">{{ formatValue(m.last, m.unit) }}</strong>
            </div>
            <svg class="sf-sparkline w-100" :viewBox="`0 0 ${w} ${h}`" preserveAspectRatio="none" role="img"
                 :aria-label="m.label">
              <polyline v-if="m.points.length > 1" :points="m.points" fill="none" stroke="currentColor"
                        stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"/>
            </svg>
            <small class="text-muted d-block mt-1">
              {{ $t('index.telemetry_avg', { value: formatValue(m.avg, m.unit) }) }}
              &middot; {{ $t('index.telemetry_peak', { value: formatValue(m.max, m.unit) }) }}
            </small>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
/**
 * Host resource telemetry panel. Polls /api/stream/telemetry every second
 * and renders lightweight SVG sparklines (no charting dependency) for CPU,
 * memory and GPU utilisation over the last 10 minutes.
 *
 * The backend store is Linux-only; non-Linux hosts return an empty series
 * object and the panel shows the "unavailable" view.
 */
export default {
  name: 'TelemetryCharts',
  data() {
    return {
      telemetry: null,
      pollTimer: null,
      w: 240,
      h: 48,
    }
  },
  computed: {
    hasSamples() {
      const t = this.telemetry?.telemetry
      return !!(t && (t.host_cpu_pct?.length || t.host_ram_used_mb?.length || t.host_gpu_pct?.length))
    },
    metrics() {
      const t = this.telemetry?.telemetry
      if (!t) {
        return []
      }
      const defs = [
        ['host_cpu_pct', this.$t('index.telemetry_cpu'), '%'],
        ['host_gpu_pct', this.$t('index.telemetry_gpu'), '%'],
        ['host_ram_used_mb', this.$t('index.telemetry_ram'), 'MB'],
      ]
      return defs.map(([key, label, unit]) => {
        const series = t[key] || []
        const points = this.pointsFor(series)
        return {
          key,
          label,
          unit,
          points,
          last: series.length ? series[series.length - 1] : 0,
          avg: series.length ? series.reduce((a, b) => a + b, 0) / series.length : 0,
          max: series.length ? Math.max(...series) : 0,
        }
      })
    },
  },
  mounted() {
    this.refresh()
    this.pollTimer = setInterval(() => this.refresh(), 1000)
  },
  beforeUnmount() {
    if (this.pollTimer) {
      clearInterval(this.pollTimer)
      this.pollTimer = null
    }
  },
  methods: {
    formatValue(v, unit) {
      return `${Number(v ?? 0).toFixed(1)} ${unit}`
    },
    /** Map a series into SVG polyline points, normalised to the viewBox. */
    pointsFor(series) {
      const n = series.length
      if (!n) {
        return []
      }
      const max = Math.max(...series, 1)
      const step = this.w / Math.max(n - 1, 1)
      return series.map((v, i) => {
        const x = i * step
        const y = this.h - (v / max) * this.h * 0.9 - this.h * 0.05
        return `${x.toFixed(1)},${y.toFixed(1)}`
      })
    },
    async refresh() {
      try {
        const r = await fetch('./api/stream/telemetry')
        if (!r.ok) {
          return
        }
        this.telemetry = await r.json()
      }
      catch (e) {
        console.error('TelemetryCharts: poll failed', e)
      }
    },
  },
}
</script>

<style scoped>
.sf-telemetry {
  border-top: 1px solid rgba(var(--bs-secondary-rgb), 0.25);
}
.sf-sparkline {
  color: var(--bs-primary);
  display: block;
  height: 48px;
}
</style>
