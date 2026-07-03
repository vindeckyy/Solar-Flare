<template>
  <div id="headless" class="config-page">
    <Checkbox class="mb-3"
              id="headless_mode"
              locale-prefix="config"
              v-model="config.headless_mode"
              default="false"
    ></Checkbox>

    <template v-if="config.headless_mode === true">
      <Checkbox class="mb-3"
                id="linux_use_cage_compositor"
                locale-prefix="config"
                v-model="config.linux_use_cage_compositor"
                default="false"
      ></Checkbox>

      <Checkbox class="mb-3"
                id="linux_prefer_gpu_native_capture"
                locale-prefix="config"
                v-model="config.linux_prefer_gpu_native_capture"
                default="false"
      ></Checkbox>

      <div class="mb-3">
        <label for="compositor_backend" class="form-label">{{ $t('config.compositor_backend') }}</label>
        <select id="compositor_backend" class="form-select" v-model="config.compositor_backend">
          <option value="auto">{{ $t('config.compositor_backend_auto') }}</option>
          <option value="krfb">{{ $t('config.compositor_backend_krfb') }}</option>
          <option value="gamescope">{{ $t('config.compositor_backend_gamescope') }}</option>
          <option value="labwc">{{ $t('config.compositor_backend_labwc') }}</option>
        </select>
        <div class="form-text">{{ $t('config.compositor_backend_desc') }}</div>
      </div>
    </template>
  </div>
</template>

<script>
  import Checkbox from '../../Checkbox.vue'

  export default {
    props: {
      config: { type: Object, required: true },
      platform: { type: String, required: true },
    },
    components: {
      Checkbox,
    },
  }
</script>
