<template>
  <div class="card my-4 sf-stream-stats">
    <div class="card-body">
      <div class="d-flex align-items-center justify-content-between mb-3">
        <div>
          <h2 class="mb-0">{{ $t('index.stream_stats_title') }}</h2>
          <small class="text-muted">{{ $t('index.stream_stats_desc') }}</small>
        </div>
        <span v-if="streaming" class="badge text-bg-success">{{ $t('index.stream_stats_streaming') }}</span>
      </div>

      <div v-if="!hasSamples" class="text-muted">
        {{ $t('index.stream_stats_no_stream') }}
      </div>

      <div v-else class="row g-3">
        <div v-for="metric in metrics" :key="metric.key" class="col-md-3 col-sm-6">
          <div class="border rounded p-3">
            <small class="text-muted d-block">{{ metric.label }}</small>
            <strong class="fs-4">{{ formatValue(metric.avg) }}</strong>
            <small class="text-muted d-block">
              min {{ formatValue(metric.min) }} / max {{ formatValue(metric.max) }}
            </small>
            <small class="text-muted d-block">{{ $t('index.stream_stats_samples', { metric: metric.label, n: metric.samples }) }}</small>
          </div>
        </div>
      </div>

      <details v-if="hasSamples" class="mt-3">
        <summary>{{ $t('index.stream_stats_effective') }}</summary>
        <dl class="row mb-0 mt-2">
          <template v-for="(item, key) in effectiveRows" :key="key">
            <dt class="col-sm-4">{{ item.label }}</dt>
            <dd class="col-sm-8 mb-1">{{ item.value }}</dd>
          </template>
        </dl>
      </details>
    </div>
  </div>
</template>

<script>
/**
 * Host-side stream latency panel. Polls /api/stream/latency every second
 * and shows the capture/convert/encode/network breakdown plus the
 * effective encoder settings of the active stream.
 */
export default {
  name: 'StreamStats',
    data() {
      return {
        stats: null,
        pollTimer: null,
      }
    },
  computed: {
    hasSamples() {
      return !!(this.stats && this.stats.capture_ms && this.stats.capture_ms.samples > 0)
    },
    streaming() {
      return this.hasSamples
    },
    metrics() {
      if (!this.stats) {
        return []
      }
      const defs = [
        ['capture_ms', this.$t('index.stream_stats_capture')],
        ['convert_ms', this.$t('index.stream_stats_convert')],
        ['encode_ms', this.$t('index.stream_stats_encode')],
        ['network_total_ms', this.$t('index.stream_stats_network')],
        ['network_queue_dwell_ms', this.$t('index.stream_stats_queue')],
        ['network_fec_ms', this.$t('index.stream_stats_fec')],
        ['network_send_ms', this.$t('index.stream_stats_send')],
        ['rtt_ms', this.$t('index.stream_stats_rtt')],
      ]
      return defs.map(([key, label]) => {
        const s = this.stats[key] || { min: 0, max: 0, avg: 0, samples: 0 }
        return { key, label, ...s }
      })
    },
    effectiveRows() {
      const s = this.stats?.effective_settings
      if (!s) {
        return []
      }
      const qpBounds = s.qmin > 0 || s.qmax > 0
        ? `${s.qmin || '-'} / ${s.qmax || '-'}`
        : 'default'
      return [
        { label: this.$t('index.stream_stats_codec'), value: s.codec || '-' },
        { label: this.$t('index.stream_stats_hwdevice'), value: s.hwdevice || '-' },
        { label: this.$t('index.stream_stats_vendor'), value: s.vendor || '-' },
        { label: this.$t('index.stream_stats_rc_mode'), value: s.rc_mode || 'auto' },
        { label: this.$t('index.stream_stats_slices'), value: s.slices || '-' },
        { label: this.$t('index.stream_stats_async_depth'), value: s.async_depth || '-' },
        { label: this.$t('index.stream_stats_qp_bounds'), value: qpBounds },
        { label: this.$t('index.stream_stats_vbv'), value: s.rc_buffer_size ? s.rc_buffer_size.toLocaleString() : '-' },
        { label: this.$t('index.stream_stats_bitrate'), value: s.bit_rate ? `${(s.bit_rate / 1000).toLocaleString()} kbit/s` : '-' },
        { label: this.$t('index.stream_stats_framerate'), value: s.framerate ? `${s.framerate} fps` : '-' },
      ]
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
    formatValue(v) {
      return `${Number(v ?? 0).toFixed(2)} ms`
    },
    async refresh() {
      try {
        const r = await fetch('./api/stream/latency')
        if (!r.ok) {
          return
        }
        this.stats = await r.json()
      }
      catch (e) {
        console.error('StreamStats: latency poll failed', e)
      }
    },
  },
}
</script>

<style scoped>
.sf-stream-stats {
  border-top: 1px solid rgba(var(--bs-secondary-rgb), 0.25);
}
</style>
