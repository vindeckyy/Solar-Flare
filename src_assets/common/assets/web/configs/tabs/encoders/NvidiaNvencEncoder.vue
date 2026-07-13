<script setup>
import { ref, watch } from 'vue'
import Checkbox from "../../../Checkbox.vue";

const props = defineProps([
  'platform',
  'config',
])


// SolarFlare fork: one-click tuning presets that override the
// individual knobs below. nvenc_tuning_preset:
//   -1 = manual (don't touch anything)
//    0 = latency-optimised  (P1, bframes=0, zerolatency, lookahead=0)
//    1 = balanced           (P4, bframes=2, lookahead=20, AQ)
//    2 = quality-optimised  (P7, bframes=4, lookahead=40, full twopass)
// When the user picks a preset we auto-fill the per-knob fields so the
// UI shows what the preset will actually do, then re-apply on submit.
function apply_tuning_preset() {
  const p = parseInt(props.config.nvenc_tuning_preset, 10);
  if (p === 0) {  // latency
    props.config.nvenc_preset = "1";
    props.config.nvenc_bframes = 0;
    props.config.nvenc_zerolatency = true;
    props.config.nvenc_rc_lookahead = 0;
    props.config.nvenc_twopass = "quarter_res";
    props.config.nvenc_spatial_aq = false;
    props.config.nvenc_temporal_aq = false;
    props.config.nvenc_weighted_prediction = false;
    props.config.nvenc_enable_min_qp = false;
    props.config.nvenc_vbv_increase = 0;
  } else if (p === 1) {  // balanced
    props.config.nvenc_preset = "4";
    props.config.nvenc_bframes = 2;
    props.config.nvenc_zerolatency = false;
    props.config.nvenc_rc_lookahead = 20;
    props.config.nvenc_twopass = "quarter_res";
    props.config.nvenc_spatial_aq = true;
    props.config.nvenc_aq_strength = 8;
    props.config.nvenc_temporal_aq = true;
    props.config.nvenc_weighted_prediction = true;
    props.config.nvenc_enable_min_qp = false;
    props.config.nvenc_vbv_increase = 50;
  } else if (p === 2) {  // quality
    props.config.nvenc_preset = "7";
    props.config.nvenc_bframes = 4;
    props.config.nvenc_zerolatency = false;
    props.config.nvenc_rc_lookahead = 40;
    props.config.nvenc_twopass = "full_res";
    props.config.nvenc_spatial_aq = true;
    props.config.nvenc_aq_strength = 12;
    props.config.nvenc_temporal_aq = true;
    props.config.nvenc_weighted_prediction = true;
    props.config.nvenc_enable_min_qp = true;
    props.config.nvenc_min_qp_h264 = 22;
    props.config.nvenc_min_qp_hevc = 26;
    props.config.nvenc_min_qp_av1 = 26;
    props.config.nvenc_vbv_increase = 100;
  }
  // p === -1 (manual) or unknown: leave the user's knobs alone
}

// Re-apply the preset when the dropdown changes, but only when the
// user picks -2 (i.e. explicitly picks a preset, not the "manual"
// sentinel that lets them edit knobs freely).
const PRESET_REAPPLY = -2;
const lastPresetSeen = ref(props.config.nvenc_tuning_preset);
watch(() => props.config.nvenc_tuning_preset, (newVal) => {
  if (newVal !== PRESET_REAPPLY && newVal !== lastPresetSeen.value) {
    apply_tuning_preset();
  }
  lastPresetSeen.value = newVal;
});
</script>

<template>
  <div id="nvidia-nvenc-encoder" class="config-page">
    <!-- SolarFlare fork: tuning preset (one-click latency/balanced/quality) -->
    <div class="mb-3">
      <label for="nvenc_tuning_preset" class="form-label">{{ $t('config.nvenc_tuning_preset') }}</label>
      <select id="nvenc_tuning_preset" class="form-select" v-model="config.nvenc_tuning_preset">
        <option value="-1">{{ $t('config.nvenc_tuning_preset_manual') }}</option>
        <option value="0">{{ $t('config.nvenc_tuning_preset_latency') }}</option>
        <option value="1">{{ $t('config.nvenc_tuning_preset_balanced') }}</option>
        <option value="2">{{ $t('config.nvenc_tuning_preset_quality') }}</option>
      </select>
      <div class="form-text">{{ $t('config.nvenc_tuning_preset_desc') }}</div>
    </div>

    <!-- Performance preset -->
    <div class="mb-3">
      <label for="nvenc_preset" class="form-label">{{ $t('config.nvenc_preset') }}</label>
      <select id="nvenc_preset" class="form-select" v-model="config.nvenc_preset">
        <option value="1">P1 {{ $t('config.nvenc_preset_1') }}</option>
        <option value="2">P2</option>
        <option value="3">P3</option>
        <option value="4">P4</option>
        <option value="5">P5</option>
        <option value="6">P6</option>
        <option value="7">P7 {{ $t('config.nvenc_preset_7') }}</option>
      </select>
      <div class="form-text">{{ $t('config.nvenc_preset_desc') }}</div>
    </div>

    <!-- Split frame encoding -->
    <div class="mb-3" v-if="platform === 'windows'">
      <label for="nvenc_split_encode" class="form-label">{{ $t('config.nvenc_split_encode') }}</label>
      <select id="nvenc_split_encode" class="form-select" v-model="config.nvenc_split_encode">
        <option value="disabled">{{ $t('_common.disabled') }}</option>
        <option value="driver_decides">{{ $t('config.nvenc_split_encode_driver_decides_def') }}</option>
        <option value="enabled">{{ $t('_common.enabled') }}</option>
      </select>
      <div class="form-text">{{ $t('config.nvenc_split_encode_desc') }}</div>
    </div>

    <!-- Two-pass mode -->
    <div class="mb-3">
      <label for="nvenc_twopass" class="form-label">{{ $t('config.nvenc_twopass') }}</label>
      <select id="nvenc_twopass" class="form-select" v-model="config.nvenc_twopass">
        <option value="disabled">{{ $t('config.nvenc_twopass_disabled') }}</option>
        <option value="quarter_res">{{ $t('config.nvenc_twopass_quarter_res') }}</option>
        <option value="full_res">{{ $t('config.nvenc_twopass_full_res') }}</option>
      </select>
      <div class="form-text">{{ $t('config.nvenc_twopass_desc') }}</div>
    </div>

    <!-- Spatial AQ -->
    <Checkbox class="mb-3"
              id="nvenc_spatial_aq"
              locale-prefix="config"
              v-model="config.nvenc_spatial_aq"
              default="false"
    ></Checkbox>

    <!-- AQ strength (only meaningful when spatial AQ is on) -->
    <div class="mb-3" v-if="config.nvenc_spatial_aq">
      <label for="nvenc_aq_strength" class="form-label">{{ $t('config.nvenc_aq_strength') }}</label>
      <input type="number" min="1" max="15" class="form-control" id="nvenc_aq_strength" placeholder="8"
             v-model="config.nvenc_aq_strength" />
      <div class="form-text">{{ $t('config.nvenc_aq_strength_desc') }}</div>
    </div>

    <!-- Temporal AQ (SolarFlare fork) -->
    <Checkbox class="mb-3"
              id="nvenc_temporal_aq"
              locale-prefix="config"
              v-model="config.nvenc_temporal_aq"
              default="false"
    ></Checkbox>

    <!-- Weighted prediction (SolarFlare fork: improves fades compression) -->
    <Checkbox class="mb-3"
              id="nvenc_weighted_prediction"
              locale-prefix="config"
              v-model="config.nvenc_weighted_prediction"
              default="false"
    ></Checkbox>

    <!-- Single-frame VBV/HRD percentage increase -->
    <div class="mb-3">
      <label for="nvenc_vbv_increase" class="form-label">{{ $t('config.nvenc_vbv_increase') }}</label>
      <input type="number" min="0" max="400" class="form-control" id="nvenc_vbv_increase" placeholder="0"
             v-model="config.nvenc_vbv_increase" />
      <div class="form-text">
        {{ $t('config.nvenc_vbv_increase_desc') }}<br>
        <br>
        <a href="https://en.wikipedia.org/wiki/Video_buffering_verifier">VBV/HRD</a>
      </div>
    </div>

    <!-- Rate-control lookahead (SolarFlare fork) -->
    <div class="mb-3">
      <label for="nvenc_rc_lookahead" class="form-label">{{ $t('config.nvenc_rc_lookahead') }}</label>
      <input type="number" min="0" max="31" class="form-control" id="nvenc_rc_lookahead" placeholder="0"
             v-model="config.nvenc_rc_lookahead" />
      <div class="form-text">{{ $t('config.nvenc_rc_lookahead_desc') }}</div>
    </div>

    <!-- B-frames (SolarFlare fork) -->
    <div class="mb-3">
      <label for="nvenc_bframes" class="form-label">{{ $t('config.nvenc_bframes') }}</label>
      <input type="number" min="0" max="4" class="form-control" id="nvenc_bframes" placeholder="0"
             v-model="config.nvenc_bframes" />
      <div class="form-text">{{ $t('config.nvenc_bframes_desc') }}</div>
    </div>

    <!-- Zero-latency tune flag (SolarFlare fork) -->
    <Checkbox class="mb-3"
              id="nvenc_zerolatency"
              locale-prefix="config"
              v-model="config.nvenc_zerolatency"
              default="false"
    ></Checkbox>

    <!-- Miscellaneous options -->
    <div class="mb-3 accordion">
      <div class="accordion-item">
        <h2 class="accordion-header">
          <button class="accordion-button" type="button" data-bs-toggle="collapse"
                  data-bs-target="#panelsStayOpen-collapseOne">
            {{ $t('config.misc') }}
          </button>
        </h2>
        <div id="panelsStayOpen-collapseOne" class="accordion-collapse collapse show"
             aria-labelledby="panelsStayOpen-headingOne">
          <div class="accordion-body">
            <!-- Min-QP clamp (SolarFlare fork; per-codec) -->
            <Checkbox class="mb-3"
                      id="nvenc_enable_min_qp"
                      locale-prefix="config"
                      v-model="config.nvenc_enable_min_qp"
                      default="false"
            ></Checkbox>

            <div class="mb-3" v-if="config.nvenc_enable_min_qp">
              <label for="nvenc_min_qp_h264" class="form-label">{{ $t('config.nvenc_min_qp_h264') }}</label>
              <input type="number" min="1" max="51" class="form-control" id="nvenc_min_qp_h264" placeholder="19"
                     v-model="config.nvenc_min_qp_h264" />
              <div class="form-text">{{ $t('config.nvenc_min_qp_h264_desc') }}</div>
            </div>
            <div class="mb-3" v-if="config.nvenc_enable_min_qp">
              <label for="nvenc_min_qp_hevc" class="form-label">{{ $t('config.nvenc_min_qp_hevc') }}</label>
              <input type="number" min="1" max="51" class="form-control" id="nvenc_min_qp_hevc" placeholder="23"
                     v-model="config.nvenc_min_qp_hevc" />
              <div class="form-text">{{ $t('config.nvenc_min_qp_hevc_desc') }}</div>
            </div>
            <div class="mb-3" v-if="config.nvenc_enable_min_qp">
              <label for="nvenc_min_qp_av1" class="form-label">{{ $t('config.nvenc_min_qp_av1') }}</label>
              <input type="number" min="1" max="255" class="form-control" id="nvenc_min_qp_av1" placeholder="23"
                     v-model="config.nvenc_min_qp_av1" />
              <div class="form-text">{{ $t('config.nvenc_min_qp_av1_desc') }}</div>
            </div>

            <!-- NVENC Realtime HAGS priority -->
            <Checkbox v-if="platform === 'windows'"
                      class="mb-3"
                      id="nvenc_realtime_hags"
                      locale-prefix="config"
                      v-model="config.nvenc_realtime_hags"
                      default="true"
            >
              <br>
              <br>
              <a href="https://devblogs.microsoft.com/directx/hardware-accelerated-gpu-scheduling/">HAGS</a>
            </Checkbox>

            <!-- Prefer lower encoding latency over power savings -->
            <Checkbox v-if="platform === 'windows'"
                      class="mb-3"
                      id="nvenc_latency_over_power"
                      locale-prefix="config"
                      v-model="config.nvenc_latency_over_power"
                      default="true"
            ></Checkbox>

            <!-- Present OpenGL/Vulkan on top of DXGI -->
            <Checkbox v-if="platform === 'windows'"
                      class="mb-3"
                      id="nvenc_opengl_vulkan_on_dxgi"
                      locale-prefix="config"
                      v-model="config.nvenc_opengl_vulkan_on_dxgi"
                      default="true"
            ></Checkbox>

            <!-- NVENC H264 CAVLC -->
            <Checkbox class="mb-3"
                      id="nvenc_h264_cavlc"
                      locale-prefix="config"
                      v-model="config.nvenc_h264_cavlc"
                      default="false"
            ></Checkbox>

            <!-- Filler data (testing) -->
            <Checkbox class="mb-3"
                      id="nvenc_filler_data"
                      locale-prefix="config"
                      v-model="config.nvenc_filler_data"
                      default="false"
            ></Checkbox>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>

</style>
