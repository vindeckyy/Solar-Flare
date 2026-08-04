<script setup>
import { ref } from 'vue'
import Checkbox from "../../../Checkbox.vue";

const props = defineProps([
  'platform',
  'config',
])

</script>

<template>
  <div id="vaapi-encoder" class="config-page">
    <!-- Strict RC Buffer -->
    <Checkbox class="mb-3"
              id="vaapi_strict_rc_buffer"
              locale-prefix="config"
              v-model="config.vaapi_strict_rc_buffer"
              default="false"
    ></Checkbox>

    <!-- Rate Control Mode -->
    <div class="mb-3">
      <label for="vaapi_rc_mode" class="form-label">{{ $t('config.vaapi_rc_mode') }}</label>
      <select id="vaapi_rc_mode" class="form-select" v-model.number="config.vaapi_rc_mode">
        <option value="0">{{ $t('_common.auto') }}</option>
        <option value="1">{{ $t('config.vaapi_rc_cqp') }}</option>
        <option value="2">{{ $t('config.vaapi_rc_cbr') }}</option>
        <option value="3">{{ $t('config.vaapi_rc_vbr') }}</option>
        <option value="4">{{ $t('config.vaapi_rc_icq') }}</option>
        <option value="5">{{ $t('config.vaapi_rc_qvbr') }}</option>
        <option value="6">{{ $t('config.vaapi_rc_avbr') }}</option>
      </select>
      <div class="form-text">{{ $t('config.vaapi_rc_mode_desc') }}</div>
      <div class="alert alert-warning mt-2 mb-0" v-if="config.vaapi_rc_mode !== 0" role="alert">
        {{ $t('config.vaapi_rc_mode_explicit_warning') }}
      </div>
    </div>

    <!-- Quality Level -->
    <div class="mb-3">
      <label for="vaapi_quality" class="form-label">{{ $t('config.vaapi_quality') }}</label>
      <input id="vaapi_quality" type="number" min="0" max="10" class="form-control"
        v-model.number="config.vaapi_quality" />
      <div class="form-text">{{ $t('config.vaapi_quality_desc') }}</div>
    </div>

    <!-- QP bounds -->
    <div class="mb-3">
      <label for="vaapi_min_qp" class="form-label">{{ $t('config.vaapi_min_qp') }}</label>
      <input id="vaapi_min_qp" type="number" min="0" max="63" class="form-control"
        v-model.number="config.vaapi_min_qp" />
      <div class="form-text">{{ $t('config.vaapi_min_qp_desc') }}</div>
    </div>

    <div class="mb-3">
      <label for="vaapi_max_qp" class="form-label">{{ $t('config.vaapi_max_qp') }}</label>
      <input id="vaapi_max_qp" type="number" min="0" max="63" class="form-control"
        v-model.number="config.vaapi_max_qp" />
      <div class="form-text">{{ $t('config.vaapi_max_qp_desc') }}</div>
    </div>

    <!-- Slice Count -->
    <div class="mb-3">
      <label for="vaapi_slice_count" class="form-label">{{ $t('config.vaapi_slice_count') }}</label>
      <input id="vaapi_slice_count" type="number" min="0" max="255" class="form-control"
        v-model.number="config.vaapi_slice_count" />
      <div class="form-text">{{ $t('config.vaapi_slice_count_desc') }}</div>
    </div>

    <!-- Encoder Queue Depth -->
    <div class="mb-3">
      <label for="vaapi_async_depth" class="form-label">{{ $t('config.vaapi_async_depth') }}</label>
      <input id="vaapi_async_depth" type="number" min="0" max="64" class="form-control"
        v-model.number="config.vaapi_async_depth" />
      <div class="form-text">{{ $t('config.vaapi_async_depth_desc') }}</div>
    </div>

    <!-- Rate-Control Buffer Size -->
    <div class="mb-3">
      <label for="vaapi_rc_buffer_frames" class="form-label">{{ $t('config.vaapi_rc_buffer_frames') }}</label>
      <input id="vaapi_rc_buffer_frames" type="number" min="0" max="100" class="form-control"
        v-model.number="config.vaapi_rc_buffer_frames" />
      <div class="form-text">{{ $t('config.vaapi_rc_buffer_frames_desc') }}</div>
    </div>
  </div>
</template>

<style scoped>

</style>
