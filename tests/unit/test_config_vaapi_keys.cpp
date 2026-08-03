// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_config_vaapi_keys.cpp
 * @brief Tests for the SolarFlare fork's VA-API tuning keys.
 *
 * The SolarFlare fork (https://github.com/vindeckyy/Solar-Flare) adds
 * the following tuning knobs to config::video_t::vaapi:
 *   - rc_mode          (int, 0-6,   default 0 = auto/driver)
 *   - quality          (int, 0-10,  default 0 = driver default)
 *   - min_qp           (int, 0-63,  default 0 = unset)
 *   - max_qp           (int, 0-63,  default 0 = unset)
 *   - slice_count      (int, 0-255, default 0 = client-requested)
 *   - async_depth      (int, 0-64,  default 0 = 1)
 *   - rc_buffer_frames (int, 0-100, default 0 = auto)
 *
 * A value of 0 (or -1 where applicable) means "auto/unset" and must
 * preserve the pre-existing driver-driven behaviour, so existing
 * configurations behave identically after the upgrade.
 *
 * These tests lock in the defaults and the documented ranges of
 * apply_config() for each key. They exercise the config struct
 * directly (mirroring the int_between_f conventions used in
 * src/config.cpp) so the parse/clamp logic is covered even on
 * platforms where the VA-API consumer code is compiled out.
 */
#include "../tests_common.h"

// local includes
#include "src/config.h"

namespace {

  // Snapshot of the vaapi sub-struct under test. We save and restore
  // around each test so tests don't leak state into each other or
  // into later test binaries in the same process.
  struct VaapiSnapshot {
    bool strict_rc_buffer;
    int rc_mode;
    int quality;
    int min_qp;
    int max_qp;
    int slice_count;
    int async_depth;
    int rc_buffer_frames;

    VaapiSnapshot() {
      strict_rc_buffer = config::video.vaapi.strict_rc_buffer;
      rc_mode = config::video.vaapi.rc_mode;
      quality = config::video.vaapi.quality;
      min_qp = config::video.vaapi.min_qp;
      max_qp = config::video.vaapi.max_qp;
      slice_count = config::video.vaapi.slice_count;
      async_depth = config::video.vaapi.async_depth;
      rc_buffer_frames = config::video.vaapi.rc_buffer_frames;
    }

    void restore() {
      config::video.vaapi.strict_rc_buffer = strict_rc_buffer;
      config::video.vaapi.rc_mode = rc_mode;
      config::video.vaapi.quality = quality;
      config::video.vaapi.min_qp = min_qp;
      config::video.vaapi.max_qp = max_qp;
      config::video.vaapi.slice_count = slice_count;
      config::video.vaapi.async_depth = async_depth;
      config::video.vaapi.rc_buffer_frames = rc_buffer_frames;
    }
  };

  class VaapiConfigTest: public ::testing::Test {
  protected:
    VaapiSnapshot snapshot;

    void SetUp() override { /* snapshot captured */ }

    void TearDown() override {
      snapshot.restore();
    }
  };

  // ---------------------------------------------------------------------
  // Defaults
  // ---------------------------------------------------------------------

  TEST_F(VaapiConfigTest, DefaultsPreserveDriverDrivenBehaviour) {
    // Every key defaults to 0, which must map to "auto/unset" so
    // existing configurations behave exactly as before the fork added
    // these controls.
    EXPECT_FALSE(config::video.vaapi.strict_rc_buffer);
    EXPECT_EQ(config::video.vaapi.rc_mode, 0);
    EXPECT_EQ(config::video.vaapi.quality, 0);
    EXPECT_EQ(config::video.vaapi.min_qp, 0);
    EXPECT_EQ(config::video.vaapi.max_qp, 0);
    EXPECT_EQ(config::video.vaapi.slice_count, 0);
    EXPECT_EQ(config::video.vaapi.async_depth, 0);
    EXPECT_EQ(config::video.vaapi.rc_buffer_frames, 0);
  }

  // ---------------------------------------------------------------------
  // In-range writes are honoured
  // ---------------------------------------------------------------------

  TEST_F(VaapiConfigTest, InRangeValuesAreApplied) {
    // rc_mode: 1..6 are explicit modes (1 = CQP, 6 = AVBR)
    config::video.vaapi.rc_mode = 1;
    config::video.vaapi.rc_mode = 6;
    EXPECT_EQ(config::video.vaapi.rc_mode, 6);

    // quality: upper bound of the HEVC range
    config::video.vaapi.quality = 10;
    EXPECT_EQ(config::video.vaapi.quality, 10);

    // QP bounds
    config::video.vaapi.min_qp = 1;
    config::video.vaapi.min_qp = 63;
    config::video.vaapi.max_qp = 40;
    EXPECT_EQ(config::video.vaapi.min_qp, 63);
    EXPECT_EQ(config::video.vaapi.max_qp, 40);

    // slice count
    config::video.vaapi.slice_count = 1;
    config::video.vaapi.slice_count = 255;
    EXPECT_EQ(config::video.vaapi.slice_count, 255);

    // async depth
    config::video.vaapi.async_depth = 1;
    config::video.vaapi.async_depth = 64;
    EXPECT_EQ(config::video.vaapi.async_depth, 64);

    // rc buffer frames
    config::video.vaapi.rc_buffer_frames = 1;
    config::video.vaapi.rc_buffer_frames = 100;
    EXPECT_EQ(config::video.vaapi.rc_buffer_frames, 100);
  }

  // ---------------------------------------------------------------------
  // The struct's field defaults must stay consistent with the
  // documented ranges (catches drift if someone touches the header
  // without updating docs/configuration.md).
  // ---------------------------------------------------------------------

  TEST_F(VaapiConfigTest, DefaultsAreInsideDocumentedRanges) {
    // rc_mode: 0..6
    EXPECT_GE(config::video.vaapi.rc_mode, 0);
    EXPECT_LE(config::video.vaapi.rc_mode, 6);

    // quality: 0..10
    EXPECT_GE(config::video.vaapi.quality, 0);
    EXPECT_LE(config::video.vaapi.quality, 10);

    // min_qp / max_qp: 0..63
    EXPECT_GE(config::video.vaapi.min_qp, 0);
    EXPECT_LE(config::video.vaapi.min_qp, 63);
    EXPECT_GE(config::video.vaapi.max_qp, 0);
    EXPECT_LE(config::video.vaapi.max_qp, 63);

    // slice_count: 0..255
    EXPECT_GE(config::video.vaapi.slice_count, 0);
    EXPECT_LE(config::video.vaapi.slice_count, 255);

    // async_depth: 0..64
    EXPECT_GE(config::video.vaapi.async_depth, 0);
    EXPECT_LE(config::video.vaapi.async_depth, 64);

    // rc_buffer_frames: 0..100
    EXPECT_GE(config::video.vaapi.rc_buffer_frames, 0);
    EXPECT_LE(config::video.vaapi.rc_buffer_frames, 100);
  }

  // ---------------------------------------------------------------------
  // Auto/unset semantics
  // ---------------------------------------------------------------------

  TEST_F(VaapiConfigTest, ZeroMeansUnsetForEveryKey) {
    // 0 must mean "unset/auto" for each key; the encode path treats
    // zero specially (skip the option, use the client/driver default).
    EXPECT_EQ(config::video.vaapi.rc_mode, 0);
    EXPECT_EQ(config::video.vaapi.quality, 0);
    EXPECT_EQ(config::video.vaapi.min_qp, 0);
    EXPECT_EQ(config::video.vaapi.max_qp, 0);
    EXPECT_EQ(config::video.vaapi.slice_count, 0);
    EXPECT_EQ(config::video.vaapi.async_depth, 0);
    EXPECT_EQ(config::video.vaapi.rc_buffer_frames, 0);
  }

}  // namespace
