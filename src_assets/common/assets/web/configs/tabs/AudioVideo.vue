<script setup>
import {ref} from 'vue'
import {$tp} from '../../platform-i18n'
import PlatformLayout from '../../PlatformLayout.vue'
import AdapterNameSelector from './audiovideo/AdapterNameSelector.vue'
import DisplayOutputSelector from './audiovideo/DisplayOutputSelector.vue'
import DisplayDeviceOptions from "./audiovideo/DisplayDeviceOptions.vue";
import DisplayModesSettings from "./audiovideo/DisplayModesSettings.vue";
import Checkbox from "../../Checkbox.vue";

const props = defineProps([
  'platform',
  'config',
])

</script>

<template>
  <div id="audio-video" class="config-page">
    <!-- Audio Sink -->
    <div class="mb-3">
      <label for="audio_sink" class="form-label">{{ $t('config.audio_sink') }}</label>
      <input type="text" class="form-control" id="audio_sink"
             :placeholder="$tp('config.audio_sink_placeholder', 'alsa_output.pci-0000_09_00.3.analog-stereo')"
             v-model="config.audio_sink" />
      <div class="form-text">
        {{ $tp('config.audio_sink_desc') }}<br>
        <PlatformLayout :platform="platform">
          <template #windows>
            <pre>tools\audio-info.exe</pre>
          </template>
          <template #freebsd>
            <pre>pacmd list-sinks | grep "name:"</pre>
            <pre>pactl info | grep Source</pre>
          </template>
          <template #linux>
            <pre>pacmd list-sinks | grep "name:"</pre>
            <pre>pactl info | grep Source</pre>
          </template>
          <template #macos>
            <a href="https://github.com/mattingalls/Soundflower" target="_blank">Soundflower</a><br>
            <a href="https://github.com/ExistentialAudio/BlackHole" target="_blank">BlackHole</a>.
          </template>
        </PlatformLayout>
      </div>
    </div>


    <PlatformLayout :platform="platform">
      <template #windows>
        <!-- Virtual Sink -->
        <div class="mb-3">
          <label for="virtual_sink" class="form-label">{{ $t('config.virtual_sink') }}</label>
          <input type="text" class="form-control" id="virtual_sink" :placeholder="$t('config.virtual_sink_placeholder')"
                 v-model="config.virtual_sink" />
          <div class="form-text">{{ $t('config.virtual_sink_desc') }}</div>
        </div>

        <!-- Install Steam Audio Drivers -->
        <Checkbox class="mb-3"
                  id="install_steam_audio_drivers"
                  locale-prefix="config"
                  v-model="config.install_steam_audio_drivers"
                  default="true"
        ></Checkbox>
      </template>
    </PlatformLayout>

    <!-- Disable Audio -->
    <Checkbox class="mb-3"
              id="stream_audio"
              locale-prefix="config"
              v-model="config.stream_audio"
              default="true"
    ></Checkbox>

    <AdapterNameSelector
        :platform="platform"
        :config="config"
    />

    <DisplayOutputSelector
      :platform="platform"
      :config="config"
    />

    <DisplayDeviceOptions
      :platform="platform"
      :config="config"
    />

    <!-- Display Modes -->
    <DisplayModesSettings
        :platform="platform"
        :config="config"
    />

    <!-- SolarFlare Audio — pre-encoder effects + Opus tuning -->
    <hr class="my-4">
    <h4>{{ $t('config.sf_audio_section') }}</h4>
    <div class="form-text mb-3">{{ $t('config.sf_audio_section_desc') }}</div>

    <!-- Effect enable checkboxes -->
    <div class="mb-3">
      <Checkbox id="sf_audio_agc"
                locale-prefix="config"
                v-model="config.sf_audio_agc"
                default="false"
      ></Checkbox>
      <Checkbox id="sf_audio_vad"
                locale-prefix="config"
                v-model="config.sf_audio_vad"
                default="false"
      ></Checkbox>
      <Checkbox id="sf_audio_ducking"
                locale-prefix="config"
                v-model="config.sf_audio_ducking"
                default="false"
      ></Checkbox>
      <Checkbox id="sf_audio_noise_gate"
                locale-prefix="config"
                v-model="config.sf_audio_noise_gate"
                default="false"
      ></Checkbox>
    </div>

    <!-- AGC tuning -->
    <div class="card mb-3" v-if="config.sf_audio_agc === true">
      <div class="card-body">
        <h6 class="card-title">AGC Tuning</h6>
        <div class="row g-2">
          <div class="col-md-6">
            <label for="sf_audio_agc_target_db" class="form-label">{{ $t('config.sf_audio_agc_target_db') }}</label>
            <input type="number" class="form-control" id="sf_audio_agc_target_db"
                   step="0.5" min="-60" max="0"
                   v-model.number="config.sf_audio_agc_target_db" />
            <div class="form-text">{{ $t('config.sf_audio_agc_target_db_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_agc_max_gain_db" class="form-label">{{ $t('config.sf_audio_agc_max_gain_db') }}</label>
            <input type="number" class="form-control" id="sf_audio_agc_max_gain_db"
                   step="0.5" min="0" max="30"
                   v-model.number="config.sf_audio_agc_max_gain_db" />
            <div class="form-text">{{ $t('config.sf_audio_agc_max_gain_db_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_agc_min_gain_db" class="form-label">{{ $t('config.sf_audio_agc_min_gain_db') }}</label>
            <input type="number" class="form-control" id="sf_audio_agc_min_gain_db"
                   step="0.5" min="-30" max="0"
                   v-model.number="config.sf_audio_agc_min_gain_db" />
            <div class="form-text">{{ $t('config.sf_audio_agc_min_gain_db_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_agc_attack_ms" class="form-label">{{ $t('config.sf_audio_agc_attack_ms') }}</label>
            <input type="number" class="form-control" id="sf_audio_agc_attack_ms"
                   step="1" min="0" max="500"
                   v-model.number="config.sf_audio_agc_attack_ms" />
            <div class="form-text">{{ $t('config.sf_audio_agc_attack_ms_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_agc_hold_ms" class="form-label">{{ $t('config.sf_audio_agc_hold_ms') }}</label>
            <input type="number" class="form-control" id="sf_audio_agc_hold_ms"
                   step="10" min="0" max="2000"
                   v-model.number="config.sf_audio_agc_hold_ms" />
            <div class="form-text">{{ $t('config.sf_audio_agc_hold_ms_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_agc_release_ms" class="form-label">{{ $t('config.sf_audio_agc_release_ms') }}</label>
            <input type="number" class="form-control" id="sf_audio_agc_release_ms"
                   step="10" min="0" max="2000"
                   v-model.number="config.sf_audio_agc_release_ms" />
            <div class="form-text">{{ $t('config.sf_audio_agc_release_ms_desc') }}</div>
          </div>
        </div>
      </div>
    </div>

    <!-- VAD tuning -->
    <div class="card mb-3" v-if="config.sf_audio_vad === true || config.sf_audio_ducking === true">
      <div class="card-body">
        <h6 class="card-title">VAD Tuning</h6>
        <div class="row g-2">
          <div class="col-md-6">
            <label for="sf_audio_vad_threshold_db" class="form-label">{{ $t('config.sf_audio_vad_threshold_db') }}</label>
            <input type="number" class="form-control" id="sf_audio_vad_threshold_db"
                   step="0.5" min="-80" max="-10"
                   v-model.number="config.sf_audio_vad_threshold_db" />
            <div class="form-text">{{ $t('config.sf_audio_vad_threshold_db_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_vad_hysteresis_db" class="form-label">{{ $t('config.sf_audio_vad_hysteresis_db') }}</label>
            <input type="number" class="form-control" id="sf_audio_vad_hysteresis_db"
                   step="0.5" min="0" max="30"
                   v-model.number="config.sf_audio_vad_hysteresis_db" />
            <div class="form-text">{{ $t('config.sf_audio_vad_hysteresis_db_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_vad_min_speech_ms" class="form-label">{{ $t('config.sf_audio_vad_min_speech_ms') }}</label>
            <input type="number" class="form-control" id="sf_audio_vad_min_speech_ms"
                   step="10" min="0" max="2000"
                   v-model.number="config.sf_audio_vad_min_speech_ms" />
            <div class="form-text">{{ $t('config.sf_audio_vad_min_speech_ms_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_vad_min_silence_ms" class="form-label">{{ $t('config.sf_audio_vad_min_silence_ms') }}</label>
            <input type="number" class="form-control" id="sf_audio_vad_min_silence_ms"
                   step="10" min="0" max="2000"
                   v-model.number="config.sf_audio_vad_min_silence_ms" />
            <div class="form-text">{{ $t('config.sf_audio_vad_min_silence_ms_desc') }}</div>
          </div>
        </div>
      </div>
    </div>

    <!-- Ducker tuning -->
    <div class="card mb-3" v-if="config.sf_audio_ducking === true">
      <div class="card-body">
        <h6 class="card-title">Ducker Tuning</h6>
        <div class="row g-2">
          <div class="col-md-6">
            <label for="sf_audio_ducker_attenuation_db" class="form-label">{{ $t('config.sf_audio_ducker_attenuation_db') }}</label>
            <input type="number" class="form-control" id="sf_audio_ducker_attenuation_db"
                   step="0.5" min="-40" max="0"
                   v-model.number="config.sf_audio_ducker_attenuation_db" />
            <div class="form-text">{{ $t('config.sf_audio_ducker_attenuation_db_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_ducker_attack_ms" class="form-label">{{ $t('config.sf_audio_ducker_attack_ms') }}</label>
            <input type="number" class="form-control" id="sf_audio_ducker_attack_ms"
                   step="5" min="0" max="2000"
                   v-model.number="config.sf_audio_ducker_attack_ms" />
            <div class="form-text">{{ $t('config.sf_audio_ducker_attack_ms_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_audio_ducker_release_ms" class="form-label">{{ $t('config.sf_audio_ducker_release_ms') }}</label>
            <input type="number" class="form-control" id="sf_audio_ducker_release_ms"
                   step="50" min="0" max="5000"
                   v-model.number="config.sf_audio_ducker_release_ms" />
            <div class="form-text">{{ $t('config.sf_audio_ducker_release_ms_desc') }}</div>
          </div>
        </div>
      </div>
    </div>

    <!-- Noise gate tuning -->
    <div class="card mb-3" v-if="config.sf_audio_noise_gate === true">
      <div class="card-body">
        <h6 class="card-title">Noise Gate Tuning</h6>
        <div class="row g-2">
          <div class="col-md-6">
            <label for="sf_audio_noise_gate_db" class="form-label">{{ $t('config.sf_audio_noise_gate_db') }}</label>
            <input type="number" class="form-control" id="sf_audio_noise_gate_db"
                   step="1" min="-90" max="-10"
                   v-model.number="config.sf_audio_noise_gate_db" />
            <div class="form-text">{{ $t('config.sf_audio_noise_gate_db_desc') }}</div>
          </div>
        </div>
      </div>
    </div>

    <!-- Opus tuning -->
    <div class="card mb-3">
      <div class="card-body">
        <h6 class="card-title">Opus Encoder Tuning</h6>
        <div class="row g-2">
          <div class="col-md-6">
            <label for="sf_opus_application" class="form-label">{{ $t('config.sf_opus_application') }}</label>
            <select id="sf_opus_application" class="form-select"
                    v-model.number="config.sf_opus_application">
              <option :value="0">{{ $t('config.sf_opus_application_lowdelay') }}</option>
              <option :value="1">{{ $t('config.sf_opus_application_voip') }}</option>
              <option :value="2">{{ $t('config.sf_opus_application_audio') }}</option>
            </select>
            <div class="form-text">{{ $t('config.sf_opus_application_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_opus_vbr" class="form-label">{{ $t('config.sf_opus_vbr') }}</label>
            <select id="sf_opus_vbr" class="form-select"
                    v-model.number="config.sf_opus_vbr">
              <option :value="0">{{ $t('config.sf_opus_vbr_off') }}</option>
              <option :value="1">{{ $t('config.sf_opus_vbr_constrained') }}</option>
              <option :value="2">{{ $t('config.sf_opus_vbr_full') }}</option>
            </select>
            <div class="form-text">{{ $t('config.sf_opus_vbr_desc') }}</div>
          </div>
          <div class="col-md-6">
            <label for="sf_opus_complexity" class="form-label">{{ $t('config.sf_opus_complexity') }}</label>
            <input type="number" class="form-control" id="sf_opus_complexity"
                   min="0" max="10" step="1"
                   v-model.number="config.sf_opus_complexity" />
            <div class="form-text">{{ $t('config.sf_opus_complexity_desc') }}</div>
          </div>
          <div class="col-md-6 d-flex align-items-end">
            <Checkbox id="sf_opus_fec"
                      locale-prefix="config"
                      v-model="config.sf_opus_fec"
                      default="true"
            ></Checkbox>
          </div>
          <div class="col-md-6">
            <label for="sf_opus_expected_loss_pct" class="form-label">{{ $t('config.sf_opus_expected_loss_pct') }}</label>
            <input type="number" class="form-control" id="sf_opus_expected_loss_pct"
                   min="0" max="100" step="1"
                   v-model.number="config.sf_opus_expected_loss_pct" />
            <div class="form-text">{{ $t('config.sf_opus_expected_loss_pct_desc') }}</div>
          </div>
          <div class="col-md-6 d-flex align-items-end">
            <Checkbox id="sf_opus_bandwidth_extension"
                      locale-prefix="config"
                      v-model="config.sf_opus_bandwidth_extension"
                      default="true"
            ></Checkbox>
          </div>
        </div>
      </div>
    </div>

  </div>
</template>

<style scoped>
</style>
