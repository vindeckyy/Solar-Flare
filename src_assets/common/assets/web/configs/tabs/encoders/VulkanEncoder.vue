<script setup>
import { ref } from 'vue'

const props = defineProps([
  'platform',
  'config',
])

</script>

<template>
  <div id="vulkan-encoder" class="config-page">
    <!-- Tuning -->
    <div class="mb-3">
      <label for="vk_tune" class="form-label">{{ $t('config.vk_tune') }}</label>
      <select id="vk_tune" class="form-select" v-model="config.vk_tune">
        <option value="0">{{ $t('_common.auto') }}</option>
        <option value="1">{{ $t('config.vk_tune_hq') }}</option>
        <option value="2">{{ $t('config.vk_tune_ll') }}</option>
        <option value="3">{{ $t('config.vk_tune_ull') }}</option>
      </select>
      <div class="form-text">{{ $t('config.vk_tune_desc') }}</div>
    </div>

    <!-- Rate Control -->
    <div class="mb-3">
      <label for="vk_rc_mode" class="form-label">{{ $t('config.vk_rc_mode') }}</label>
      <select id="vk_rc_mode" class="form-select" v-model="config.vk_rc_mode">
        <option value="0">{{ $t('_common.auto') }}</option>
        <option value="1">{{ $t('config.vk_rc_cqp') }}</option>
        <option value="2">{{ $t('config.vk_rc_cbr') }}</option>
        <option value="4">{{ $t('config.vk_rc_vbr') }}</option>
      </select>
      <div class="form-text">{{ $t('config.vk_rc_mode_desc') }}</div>
    </div>

    <!-- QP bounds: flatten encode-time variance for predictable per-frame latency. -->
    <div class="mb-3">
      <label for="vk_min_qp" class="form-label">{{ $t('config.vk_min_qp') }}</label>
      <input id="vk_min_qp" type="number" min="0" max="63" class="form-control"
        v-model.number="config.vk_min_qp" />
      <div class="form-text">{{ $t('config.vk_min_qp_desc') }}</div>
    </div>

    <div class="mb-3">
      <label for="vk_max_qp" class="form-label">{{ $t('config.vk_max_qp') }}</label>
      <input id="vk_max_qp" type="number" min="0" max="63" class="form-control"
        v-model.number="config.vk_max_qp" />
      <div class="form-text">{{ $t('config.vk_max_qp_desc') }}</div>
    </div>
  </div>
</template>
