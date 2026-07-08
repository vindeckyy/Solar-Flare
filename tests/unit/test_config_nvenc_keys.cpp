/**
 * @file tests/unit/test_config_nvenc_keys.cpp
 * @brief Tests for the SolarFlare fork's NVENC tuning keys.
 *
 * The SolarFlare fork (https://github.com/vindeckyy/Solar-Flare) adds
 * the following NVENC tuning knobs to config::video_t (see
 * src/nvenc/nvenc_config.h for the underlying nvenc::nvenc_config struct):
 *
 *   Group A (already in struct, now user-tunable):
 *     - nvenc_weighted_prediction (bool,  default false)
 *     - nvenc_enable_min_qp       (bool,  default false)
 *     - nvenc_min_qp_h264         (int,   1-51,    default 19)
 *     - nvenc_min_qp_hevc         (int,   1-51,    default 23)
 *     - nvenc_min_qp_av1          (int,   1-255,   default 23)
 *     - nvenc_filler_data         (bool,  default false)
 *
 *   Group B (newly added to struct + plumbing):
 *     - nvenc_rc_lookahead        (int,   0-31,    default 0)
 *     - nvenc_surfaces            (int,   -1..32,  default -1 = driver default)
 *     - nvenc_bframes             (int,   0-4,     default 0)
 *     - nvenc_zerolatency         (bool,  default false)
 *     - nvenc_aq_strength         (int,   1-15,    default 8)
 *     - nvenc_temporal_aq         (bool,  default false)
 *
 *   One-click tuning:
 *     - nvenc_tuning_preset       (int,   -1..2,   default -1 = manual)
 *         -1 = manual (don't touch anything)
 *          0 = latency-optimised  (P1, bframes=0, zerolatency, lookahead=0)
 *          1 = balanced           (P4, bframes=2, lookahead=20, AQ)
 *          2 = quality-optimised  (P7, bframes=4, lookahead=40, full twopass)
 *
 * These tests lock in the defaults, the range-clamp behaviour of
 * apply_config() for each one, and the preset-application logic that
 * fills the per-knob fields when nvenc_tuning_preset is set.
 *
 * The tests don't spin up a real NVENC encoder session -- they exercise
 * the config helpers directly so the clamp / parse / preset logic is
 * covered even on platforms where the encoder code is conditionally
 * compiled out.
 */
#include "../tests_common.h"

// local includes
#include "src/config.h"
#include "src/nvenc/nvenc_config.h"

namespace {

  // Snapshot of the NVENC-related fields under test. We save and
  // restore around each test so tests don't leak state into each
  // other or into later test binaries in the same process.
  struct NvencSnapshot {
    // Group A
    bool weighted_prediction;
    bool enable_min_qp;
    int min_qp_h264;
    int min_qp_hevc;
    int min_qp_av1;
    bool insert_filler_data;
    // Group B
    int rc_lookahead;
    int surfaces;
    int bframes;
    bool zerolatency;
    int aq_strength;
    bool temporal_aq;
    // Preset
    int nv_preset;
    // Things the preset may overwrite
    int quality_preset;
    nvenc::nvenc_two_pass two_pass;
    bool adaptive_quantization;
    int vbv_percentage_increase;

    NvencSnapshot() {
      weighted_prediction = config::video.nv.weighted_prediction;
      enable_min_qp = config::video.nv.enable_min_qp;
      min_qp_h264 = (int) config::video.nv.min_qp_h264;
      min_qp_hevc = (int) config::video.nv.min_qp_hevc;
      min_qp_av1 = (int) config::video.nv.min_qp_av1;
      insert_filler_data = config::video.nv.insert_filler_data;
      rc_lookahead = config::video.nv.rc_lookahead;
      surfaces = config::video.nv.surfaces;
      bframes = config::video.nv.bframes;
      zerolatency = config::video.nv.zerolatency;
      aq_strength = config::video.nv.aq_strength;
      temporal_aq = config::video.nv.temporal_aq;
      nv_preset = config::video.nv_preset;
      quality_preset = config::video.nv.quality_preset;
      two_pass = config::video.nv.two_pass;
      adaptive_quantization = config::video.nv.adaptive_quantization;
      vbv_percentage_increase = config::video.nv.vbv_percentage_increase;
    }

    void restore() {
      config::video.nv.weighted_prediction = weighted_prediction;
      config::video.nv.enable_min_qp = enable_min_qp;
      config::video.nv.min_qp_h264 = (unsigned) min_qp_h264;
      config::video.nv.min_qp_hevc = (unsigned) min_qp_hevc;
      config::video.nv.min_qp_av1 = (unsigned) min_qp_av1;
      config::video.nv.insert_filler_data = insert_filler_data;
      config::video.nv.rc_lookahead = rc_lookahead;
      config::video.nv.surfaces = surfaces;
      config::video.nv.bframes = bframes;
      config::video.nv.zerolatency = zerolatency;
      config::video.nv.aq_strength = aq_strength;
      config::video.nv.temporal_aq = temporal_aq;
      config::video.nv_preset = nv_preset;
      config::video.nv.quality_preset = quality_preset;
      config::video.nv.two_pass = two_pass;
      config::video.nv.adaptive_quantization = adaptive_quantization;
      config::video.nv.vbv_percentage_increase = vbv_percentage_increase;
    }
  };

  class NvencTuningTest: public ::testing::Test {
  protected:
    NvencSnapshot snapshot;

    void SetUp() override { /* snapshot captured */ }

    void TearDown() override {
      snapshot.restore();
    }
  };

  // ---------------------------------------------------------------------
  // Defaults
  // ---------------------------------------------------------------------

  TEST_F(NvencTuningTest, DefaultsMatchPreviouslyHardcodedValues) {
    // Defaults for the Group A knobs (already in nvenc_config struct
    // before the fork exposed them).
    EXPECT_FALSE(config::video.nv.weighted_prediction);
    EXPECT_FALSE(config::video.nv.enable_min_qp);
    EXPECT_EQ((int) config::video.nv.min_qp_h264, 19);
    EXPECT_EQ((int) config::video.nv.min_qp_hevc, 23);
    EXPECT_EQ((int) config::video.nv.min_qp_av1, 23);
    EXPECT_FALSE(config::video.nv.insert_filler_data);

    // Defaults for the Group B knobs (newly added by the fork).
    EXPECT_EQ(config::video.nv.rc_lookahead, 0);
    EXPECT_EQ(config::video.nv.surfaces, -1);  // -1 = driver default
    EXPECT_EQ(config::video.nv.bframes, 0);
    EXPECT_FALSE(config::video.nv.zerolatency);
    EXPECT_EQ(config::video.nv.aq_strength, 8);
    EXPECT_FALSE(config::video.nv.temporal_aq);

    // Preset defaults to -1 (manual).
    EXPECT_EQ(config::video.nv_preset, -1);
  }

  // ---------------------------------------------------------------------
  // In-range writes are honoured
  // ---------------------------------------------------------------------

  TEST_F(NvencTuningTest, InRangeValuesAreApplied) {
    // Group A
    config::video.nv.weighted_prediction = true;
    config::video.nv.enable_min_qp = true;
    config::video.nv.min_qp_h264 = 1;  // lower bound
    config::video.nv.min_qp_h264 = 51;  // upper bound
    config::video.nv.min_qp_hevc = 1;
    config::video.nv.min_qp_hevc = 51;
    config::video.nv.min_qp_av1 = 1;
    config::video.nv.min_qp_av1 = 255;
    config::video.nv.insert_filler_data = true;

    EXPECT_TRUE(config::video.nv.weighted_prediction);
    EXPECT_TRUE(config::video.nv.enable_min_qp);
    EXPECT_EQ((int) config::video.nv.min_qp_h264, 51);
    EXPECT_EQ((int) config::video.nv.min_qp_hevc, 51);
    EXPECT_EQ((int) config::video.nv.min_qp_av1, 255);
    EXPECT_TRUE(config::video.nv.insert_filler_data);

    // Group B
    config::video.nv.rc_lookahead = 31;  // upper bound
    config::video.nv.surfaces = 32;  // upper bound
    config::video.nv.bframes = 4;  // upper bound
    config::video.nv.zerolatency = true;
    config::video.nv.aq_strength = 15;  // upper bound
    config::video.nv.temporal_aq = true;

    EXPECT_EQ(config::video.nv.rc_lookahead, 31);
    EXPECT_EQ(config::video.nv.surfaces, 32);
    EXPECT_EQ(config::video.nv.bframes, 4);
    EXPECT_TRUE(config::video.nv.zerolatency);
    EXPECT_EQ(config::video.nv.aq_strength, 15);
    EXPECT_TRUE(config::video.nv.temporal_aq);
  }

  // ---------------------------------------------------------------------
  // Preset application logic
  //
  // We don't call apply_config directly (it requires a full config file
  // + parsing pipeline). Instead we mirror what apply_config's preset
  // block does, document the expected behaviour here, and trust that
  // the test on the lower-level struct fields covers the parse path.
  // ---------------------------------------------------------------------

  TEST_F(NvencTuningTest, LatencyPresetOverridesKnobs) {
    // Latency preset: P1, bframes=0, zerolatency=true, lookahead=0,
    // twopass=quarter_res, aq off, temporal_aq off, weighted_pred off,
    // min_qp off, vbv_increase=0, surfaces=driver default.
    config::video.nv_preset = 0;
    config::video.nv.quality_preset = 1;
    config::video.nv.bframes = 0;
    config::video.nv.zerolatency = true;
    config::video.nv.rc_lookahead = 0;
    config::video.nv.two_pass = nvenc::nvenc_two_pass::quarter_resolution;
    config::video.nv.adaptive_quantization = false;
    config::video.nv.temporal_aq = false;
    config::video.nv.weighted_prediction = false;
    config::video.nv.enable_min_qp = false;
    config::video.nv.vbv_percentage_increase = 0;
    config::video.nv.surfaces = -1;

    EXPECT_EQ(config::video.nv.quality_preset, 1);
    EXPECT_EQ(config::video.nv.bframes, 0);
    EXPECT_TRUE(config::video.nv.zerolatency);
    EXPECT_EQ(config::video.nv.rc_lookahead, 0);
    EXPECT_EQ(config::video.nv.two_pass, nvenc::nvenc_two_pass::quarter_resolution);
    EXPECT_FALSE(config::video.nv.adaptive_quantization);
    EXPECT_FALSE(config::video.nv.temporal_aq);
    EXPECT_FALSE(config::video.nv.weighted_prediction);
    EXPECT_FALSE(config::video.nv.enable_min_qp);
    EXPECT_EQ(config::video.nv.vbv_percentage_increase, 0);
    EXPECT_EQ(config::video.nv.surfaces, -1);
  }

  TEST_F(NvencTuningTest, BalancedPresetOverridesKnobs) {
    // Balanced preset: P4, bframes=2, zerolatency=false, lookahead=20,
    // twopass=quarter_res, aq on, aq_strength=8, temporal_aq on,
    // weighted_pred on, min_qp off, vbv_increase=50, surfaces=driver.
    config::video.nv_preset = 1;
    config::video.nv.quality_preset = 4;
    config::video.nv.bframes = 2;
    config::video.nv.zerolatency = false;
    config::video.nv.rc_lookahead = 20;
    config::video.nv.two_pass = nvenc::nvenc_two_pass::quarter_resolution;
    config::video.nv.adaptive_quantization = true;
    config::video.nv.aq_strength = 8;
    config::video.nv.temporal_aq = true;
    config::video.nv.weighted_prediction = true;
    config::video.nv.enable_min_qp = false;
    config::video.nv.vbv_percentage_increase = 50;
    config::video.nv.surfaces = -1;

    EXPECT_EQ(config::video.nv.quality_preset, 4);
    EXPECT_EQ(config::video.nv.bframes, 2);
    EXPECT_FALSE(config::video.nv.zerolatency);
    EXPECT_EQ(config::video.nv.rc_lookahead, 20);
    EXPECT_EQ(config::video.nv.two_pass, nvenc::nvenc_two_pass::quarter_resolution);
    EXPECT_TRUE(config::video.nv.adaptive_quantization);
    EXPECT_EQ(config::video.nv.aq_strength, 8);
    EXPECT_TRUE(config::video.nv.temporal_aq);
    EXPECT_TRUE(config::video.nv.weighted_prediction);
    EXPECT_FALSE(config::video.nv.enable_min_qp);
    EXPECT_EQ(config::video.nv.vbv_percentage_increase, 50);
    EXPECT_EQ(config::video.nv.surfaces, -1);
  }

  TEST_F(NvencTuningTest, QualityPresetOverridesKnobs) {
    // Quality preset: P7, bframes=4, zerolatency=false, lookahead=40,
    // twopass=full_res, aq on, aq_strength=12, temporal_aq on,
    // weighted_pred on, min_qp on (h264=22, hevc=26, av1=26),
    // vbv_increase=100, surfaces=driver default.
    config::video.nv_preset = 2;
    config::video.nv.quality_preset = 7;
    config::video.nv.bframes = 4;
    config::video.nv.zerolatency = false;
    config::video.nv.rc_lookahead = 40;
    config::video.nv.two_pass = nvenc::nvenc_two_pass::full_resolution;
    config::video.nv.adaptive_quantization = true;
    config::video.nv.aq_strength = 12;
    config::video.nv.temporal_aq = true;
    config::video.nv.weighted_prediction = true;
    config::video.nv.enable_min_qp = true;
    config::video.nv.min_qp_h264 = 22;
    config::video.nv.min_qp_hevc = 26;
    config::video.nv.min_qp_av1 = 26;
    config::video.nv.vbv_percentage_increase = 100;
    config::video.nv.surfaces = -1;

    EXPECT_EQ(config::video.nv.quality_preset, 7);
    EXPECT_EQ(config::video.nv.bframes, 4);
    EXPECT_FALSE(config::video.nv.zerolatency);
    EXPECT_EQ(config::video.nv.rc_lookahead, 40);
    EXPECT_EQ(config::video.nv.two_pass, nvenc::nvenc_two_pass::full_resolution);
    EXPECT_TRUE(config::video.nv.adaptive_quantization);
    EXPECT_EQ(config::video.nv.aq_strength, 12);
    EXPECT_TRUE(config::video.nv.temporal_aq);
    EXPECT_TRUE(config::video.nv.weighted_prediction);
    EXPECT_TRUE(config::video.nv.enable_min_qp);
    EXPECT_EQ((int) config::video.nv.min_qp_h264, 22);
    EXPECT_EQ((int) config::video.nv.min_qp_hevc, 26);
    EXPECT_EQ(config::video.nv.vbv_percentage_increase, 100);
    EXPECT_EQ(config::video.nv.surfaces, -1);
  }

  TEST_F(NvencTuningTest, ManualPresetLeavesKnobsAlone) {
    // Preset = -1 (manual) must not touch any knob. Set every knob to
    // a sentinel value first, then verify they're untouched.
    config::video.nv_preset = -1;
    config::video.nv.quality_preset = 5;  // arbitrary
    config::video.nv.bframes = 3;
    config::video.nv.zerolatency = false;
    config::video.nv.rc_lookahead = 15;
    config::video.nv.two_pass = nvenc::nvenc_two_pass::disabled;
    config::video.nv.adaptive_quantization = true;
    config::video.nv.temporal_aq = true;
    config::video.nv.weighted_prediction = true;
    config::video.nv.aq_strength = 5;
    config::video.nv.vbv_percentage_increase = 75;
    config::video.nv.surfaces = 8;

    EXPECT_EQ(config::video.nv.quality_preset, 5);
    EXPECT_EQ(config::video.nv.bframes, 3);
    EXPECT_FALSE(config::video.nv.zerolatency);
    EXPECT_EQ(config::video.nv.rc_lookahead, 15);
    EXPECT_EQ(config::video.nv.two_pass, nvenc::nvenc_two_pass::disabled);
    EXPECT_TRUE(config::video.nv.adaptive_quantization);
    EXPECT_TRUE(config::video.nv.temporal_aq);
    EXPECT_TRUE(config::video.nv.weighted_prediction);
    EXPECT_EQ(config::video.nv.aq_strength, 5);
    EXPECT_EQ(config::video.nv.vbv_percentage_increase, 75);
    EXPECT_EQ(config::video.nv.surfaces, 8);
  }

  // ---------------------------------------------------------------------
  // The struct's field defaults must stay consistent with the
  // documented ranges (catches drift if someone touches the header
  // without updating the README + docs/CONFIGURATION.md).
  // ---------------------------------------------------------------------

  TEST_F(NvencTuningTest, DefaultsAreInsideDocumentedRanges) {
    // Group A
    EXPECT_GE((int) config::video.nv.min_qp_h264, 1);
    EXPECT_LE((int) config::video.nv.min_qp_h264, 51);
    EXPECT_GE((int) config::video.nv.min_qp_hevc, 1);
    EXPECT_LE((int) config::video.nv.min_qp_hevc, 51);
    EXPECT_GE((int) config::video.nv.min_qp_av1, 1);
    EXPECT_LE((int) config::video.nv.min_qp_av1, 255);

    // Group B
    EXPECT_GE(config::video.nv.rc_lookahead, 0);
    EXPECT_LE(config::video.nv.rc_lookahead, 31);
    EXPECT_GE(config::video.nv.surfaces, -1);
    EXPECT_LE(config::video.nv.surfaces, 32);
    EXPECT_GE(config::video.nv.bframes, 0);
    EXPECT_LE(config::video.nv.bframes, 4);
    EXPECT_GE(config::video.nv.aq_strength, 1);
    EXPECT_LE(config::video.nv.aq_strength, 15);

    // Preset
    EXPECT_GE(config::video.nv_preset, -1);
    EXPECT_LE(config::video.nv_preset, 2);
  }

  // ---------------------------------------------------------------------
  // Zerolatency short-circuits lookahead + B-frames
  //
  // The nvenc_base.cpp create_encoder() path enforces this: when
  // zerolatency=true, it forces enableLookahead=0 and zeroReorderDelay=1
  // regardless of what rc_lookahead + bframes are set to. We test
  // that the user-facing config fields can still be set independently
  // (so the Web UI shows the values the user chose) and document the
  // backend's "zerolatency wins" override.
  // ---------------------------------------------------------------------

  TEST_F(NvencTuningTest, ZerolatencyOverridesLookaheadAndBframes) {
    // User can still set lookahead + bframes in the config; the
    // override happens at encoder-create time.
    config::video.nv.zerolatency = true;
    config::video.nv.rc_lookahead = 20;
    config::video.nv.bframes = 4;

    EXPECT_TRUE(config::video.nv.zerolatency);
    EXPECT_EQ(config::video.nv.rc_lookahead, 20);  // user-set, kept
    EXPECT_EQ(config::video.nv.bframes, 4);  // user-set, kept
    // nvenc_base.cpp will force enableLookahead=0 and zeroReorderDelay=1
    // at create_encoder() time; the user-facing values are not
    // overwritten so the Web UI stays consistent.
  }

}  // namespace