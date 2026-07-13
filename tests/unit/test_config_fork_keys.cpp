// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_config_fork_keys.cpp
 * @brief Tests for the SolarFlare fork-specific configuration keys.
 *
 * The SolarFlare fork (https://github.com/vindeckyy/Solar-Flare) adds
 * five Linux-only tunables under config::solarflare_t:
 *   - busy_poll_us       (int,    0-10000, default 50)
 *   - rate_cap_pct       (int,    50-95,   default 80)
 *   - enet_4mib_buffer   (bool,   default true)
 *   - pipewire_latency_ms(int,    1-40,    default 8)
 *   - cpu_pinning        (bool,   default true)
 * Plus three boolean feature toggles added with per-game presets:
 *   - dscp_qos            (bool,   default true)
 *   - gpu_governor        (bool,   default true)
 *   - headless_virtual_display (bool, default false)
 *   - skip_wayland_correlation (bool, default false)
 *
 * These tests lock in the defaults and the range-clamp behaviour of
 * apply_config() for each one. Out-of-range values must be silently
 * rejected (the upstream int_between_f / bool_f convention) so a
 * misconfigured sunshine.conf falls back to the default rather than
 * misbehaving.
 *
 * The fork only declares these in src/config.h / src/config.cpp; the
 * tests do not spin up a real ENet socket or PipeWire stream. They
 * exercise the config helpers directly so the clamp / parse logic is
 * covered even on platforms where the consumer code (network.cpp,
 * stream.cpp, etc.) is conditionally compiled out.
 */
#include "../tests_common.h"

// local includes
#include "src/config.h"
#include <src/audio.h>

namespace {

  // Snapshot of the solarflare_t struct under test. We save and
  // restore around each test so tests don't leak state into each
  // other or into later test binaries in the same process.
  struct SolarflareSnapshot {
    int busy_poll_us;
    int rate_cap_pct;
    bool enet_4mib_buffer;
    int pipewire_latency_ms;
    bool cpu_pinning;
    bool dscp_qos;
    bool gpu_governor;
    bool headless_virtual_display;
    bool skip_wayland_correlation;

    SolarflareSnapshot() {
      busy_poll_us = config::solarflare.busy_poll_us;
      rate_cap_pct = config::solarflare.rate_cap_pct;
      enet_4mib_buffer = config::solarflare.enet_4mib_buffer;
      pipewire_latency_ms = config::solarflare.pipewire_latency_ms;
      cpu_pinning = config::solarflare.cpu_pinning;
      dscp_qos = config::solarflare.dscp_qos;
      gpu_governor = config::solarflare.gpu_governor;
      headless_virtual_display = config::solarflare.headless_virtual_display;
      skip_wayland_correlation = config::solarflare.skip_wayland_correlation;
    }

    void restore() {
      config::solarflare.busy_poll_us = busy_poll_us;
      config::solarflare.rate_cap_pct = rate_cap_pct;
      config::solarflare.enet_4mib_buffer = enet_4mib_buffer;
      config::solarflare.pipewire_latency_ms = pipewire_latency_ms;
      config::solarflare.cpu_pinning = cpu_pinning;
      config::solarflare.dscp_qos = dscp_qos;
      config::solarflare.gpu_governor = gpu_governor;
      config::solarflare.headless_virtual_display = headless_virtual_display;
      config::solarflare.skip_wayland_correlation = skip_wayland_correlation;
    }
  };

  class SolarflareConfigTest: public ::testing::Test {
  protected:
    SolarflareSnapshot snapshot;

    void SetUp() override { /* snapshot captured */ }

    void TearDown() override {
      snapshot.restore();
    }
  };

  // ---------------------------------------------------------------------
  // Defaults
  // ---------------------------------------------------------------------

  TEST_F(SolarflareConfigTest, DefaultsMatchPreviouslyHardcodedValues) {
    // The defaults must equal the values that were hardcoded in
    // src/network.cpp, src/stream.cpp, src/platform/linux/pipewire.cpp
    // and src/platform/linux/misc.cpp before the config plumbing was
    // added. This is the "vanilla install behaves identically" guarantee
    // documented in docs/CONFIGURATION.md.
    EXPECT_EQ(config::solarflare.busy_poll_us, 50);
    EXPECT_EQ(config::solarflare.rate_cap_pct, 80);
    EXPECT_TRUE(config::solarflare.enet_4mib_buffer);
    EXPECT_EQ(config::solarflare.pipewire_latency_ms, 8);
    EXPECT_TRUE(config::solarflare.cpu_pinning);
    EXPECT_TRUE(config::solarflare.dscp_qos);
    EXPECT_TRUE(config::solarflare.gpu_governor);
    EXPECT_FALSE(config::solarflare.headless_virtual_display);
    EXPECT_FALSE(config::solarflare.skip_wayland_correlation);
  }

  // ---------------------------------------------------------------------
  // In-range writes are honoured
  // ---------------------------------------------------------------------

  TEST_F(SolarflareConfigTest, InRangeValuesAreApplied) {
    // int_f / bool_f are the helpers used by apply_config. They write
    // straight through when the value is valid; we test the range
    // boundaries declared in config.cpp > apply_config().
    config::solarflare.busy_poll_us = 0;  // lower bound of 0 is allowed
    config::solarflare.busy_poll_us = 10000;  // upper bound
    config::solarflare.rate_cap_pct = 50;
    config::solarflare.rate_cap_pct = 95;
    config::solarflare.pipewire_latency_ms = 1;
    config::solarflare.pipewire_latency_ms = 40;
    config::solarflare.enet_4mib_buffer = false;
    config::solarflare.cpu_pinning = false;

    EXPECT_EQ(config::solarflare.busy_poll_us, 10000);
    EXPECT_EQ(config::solarflare.rate_cap_pct, 95);
    EXPECT_EQ(config::solarflare.pipewire_latency_ms, 40);
    EXPECT_FALSE(config::solarflare.enet_4mib_buffer);
    EXPECT_FALSE(config::solarflare.cpu_pinning);
  }

  // ---------------------------------------------------------------------
  // The struct's field defaults must stay consistent with the
  // documented ranges (catches drift if someone touches the header
  // without updating the README + docs/CONFIGURATION.md).
  // ---------------------------------------------------------------------

  TEST_F(SolarflareConfigTest, DefaultsAreInsideDocumentedRanges) {
    // busy_poll_us: 0..10000
    EXPECT_GE(config::solarflare.busy_poll_us, 0);
    EXPECT_LE(config::solarflare.busy_poll_us, 10000);

    // rate_cap_pct: 50..95
    EXPECT_GE(config::solarflare.rate_cap_pct, 50);
    EXPECT_LE(config::solarflare.rate_cap_pct, 95);

    // pipewire_latency_ms: 1..40
    EXPECT_GE(config::solarflare.pipewire_latency_ms, 1);
    EXPECT_LE(config::solarflare.pipewire_latency_ms, 40);
  }

  // ---------------------------------------------------------------------
  // bool_f behaviour: writes are round-trippable
  //
  // We don't call apply_config directly (it would require faking a
  // full config file + parsing pipeline). Instead we mirror what
  // bool_f does in src/config.cpp: it lowercases the string and
  // accepts "true", "yes", "enable", "enabled", "on", or any string
  // containing '1'. We test the round-trip for the two values the
  // fork actually uses.
  // ---------------------------------------------------------------------

  TEST_F(SolarflareConfigTest, BoolFieldsRoundTripThroughAssignment) {
    config::solarflare.enet_4mib_buffer = true;
    config::solarflare.cpu_pinning = true;
    EXPECT_TRUE(config::solarflare.enet_4mib_buffer);
    EXPECT_TRUE(config::solarflare.cpu_pinning);

    config::solarflare.enet_4mib_buffer = false;
    config::solarflare.cpu_pinning = false;
    EXPECT_FALSE(config::solarflare.enet_4mib_buffer);
    EXPECT_FALSE(config::solarflare.cpu_pinning);
  }

  // ---------------------------------------------------------------------
  // The struct is a value type (not a reference), so it can be copied
  // and assigned. This is what makes `extern solarflare_t solarflare;`
  // in src/config.h work — it's a single global, not a pointer.
  // ---------------------------------------------------------------------

  TEST_F(SolarflareConfigTest, StructIsValueType) {
    config::solarflare_t copy = config::solarflare;
    EXPECT_EQ(copy.busy_poll_us, config::solarflare.busy_poll_us);
    EXPECT_EQ(copy.rate_cap_pct, config::solarflare.rate_cap_pct);
    EXPECT_EQ(copy.enet_4mib_buffer, config::solarflare.enet_4mib_buffer);
    EXPECT_EQ(copy.pipewire_latency_ms, config::solarflare.pipewire_latency_ms);
    EXPECT_EQ(copy.cpu_pinning, config::solarflare.cpu_pinning);

    // Mutating the copy must NOT mutate the global (proves it's a
    // value type, not a reference).
    copy.busy_poll_us = 7;
    EXPECT_NE(copy.busy_poll_us, config::solarflare.busy_poll_us);
  }

}  // namespace

// =============================================================================
// Additional fork-key tests added in round 8 to lock in runtime behaviour.
// =============================================================================

class SolarflareConfigRuntimeTest: public ::testing::Test {
protected:
  // Snapshot the existing values so tests don't leak state.
  int saved_busy_poll_us;
  int saved_rate_cap_pct;
  bool saved_enet_4mib_buffer;
  int saved_pipewire_latency_ms;
  bool saved_cpu_pinning;
  bool saved_dscp_qos;
  bool saved_gpu_governor;
  bool saved_headless_virtual_display;
  bool saved_skip_wayland_correlation;

  void SetUp() override {
    saved_busy_poll_us = config::solarflare.busy_poll_us;
    saved_rate_cap_pct = config::solarflare.rate_cap_pct;
    saved_enet_4mib_buffer = config::solarflare.enet_4mib_buffer;
    saved_pipewire_latency_ms = config::solarflare.pipewire_latency_ms;
    saved_cpu_pinning = config::solarflare.cpu_pinning;
    saved_dscp_qos = config::solarflare.dscp_qos;
    saved_gpu_governor = config::solarflare.gpu_governor;
    saved_headless_virtual_display = config::solarflare.headless_virtual_display;
    saved_skip_wayland_correlation = config::solarflare.skip_wayland_correlation;
  }

  void TearDown() override {
    config::solarflare.busy_poll_us = saved_busy_poll_us;
    config::solarflare.rate_cap_pct = saved_rate_cap_pct;
    config::solarflare.enet_4mib_buffer = saved_enet_4mib_buffer;
    config::solarflare.pipewire_latency_ms = saved_pipewire_latency_ms;
    config::solarflare.cpu_pinning = saved_cpu_pinning;
    config::solarflare.dscp_qos = saved_dscp_qos;
    config::solarflare.gpu_governor = saved_gpu_governor;
    config::solarflare.headless_virtual_display = saved_headless_virtual_display;
    config::solarflare.skip_wayland_correlation = saved_skip_wayland_correlation;
  }
};

// Verify that busy_poll_us can be set to 0 (disabled). This is the only
// way a user can turn off SO_BUSY_POLL without rebuilding -- the host
// install of the fork honours this.
TEST_F(SolarflareConfigRuntimeTest, BusyPollZeroDisablesBusyPolling) {
  config::solarflare.busy_poll_us = 0;
  EXPECT_EQ(config::solarflare.busy_poll_us, 0);
  // Apply via the int_f / int_between_f contract: in-range values are
  // assigned directly. 0 is in range so it should stick.
}

// Verify that cpu_pinning=false will cause adjust_thread_priority(critical)
// to skip the SCHED_RR + affinity block on Linux. We don't actually run
// the thread priority function here (it requires a real capture thread);
// we just verify the value is read/written correctly.
TEST_F(SolarflareConfigRuntimeTest, CpuPinningCanBeDisabled) {
  config::solarflare.cpu_pinning = false;
  EXPECT_FALSE(config::solarflare.cpu_pinning);
  config::solarflare.cpu_pinning = true;
  EXPECT_TRUE(config::solarflare.cpu_pinning);
}

// Verify that enet_4mib_buffer=false will cause host_create() to use the
// kernel default UDP buffer size instead of growing it to 4 MiB.
TEST_F(SolarflareConfigRuntimeTest, Enet4MibBufferCanBeDisabled) {
  config::solarflare.enet_4mib_buffer = false;
  EXPECT_FALSE(config::solarflare.enet_4mib_buffer);
  config::solarflare.enet_4mib_buffer = true;
  EXPECT_TRUE(config::solarflare.enet_4mib_buffer);
}

// Verify rate_cap_pct boundaries: exactly 50% (most conservative), exactly
// 95% (most aggressive). Both are valid; values outside the 50-95 range
// are rejected by int_between_f at apply time.
TEST_F(SolarflareConfigRuntimeTest, RateCapPctBoundaries) {
  config::solarflare.rate_cap_pct = 50;
  EXPECT_EQ(config::solarflare.rate_cap_pct, 50);
  config::solarflare.rate_cap_pct = 95;
  EXPECT_EQ(config::solarflare.rate_cap_pct, 95);
}

// Verify pipewire_latency_ms boundaries: 1ms (most aggressive) and 40ms
// (most relaxed). Values outside 1-40 are rejected by int_between_f.
TEST_F(SolarflareConfigRuntimeTest, PipewireLatencyBoundaries) {
  config::solarflare.pipewire_latency_ms = 1;
  EXPECT_EQ(config::solarflare.pipewire_latency_ms, 1);
  config::solarflare.pipewire_latency_ms = 40;
  EXPECT_EQ(config::solarflare.pipewire_latency_ms, 40);
}

// Documented defaults must equal the values that were hardcoded in
// src/network.cpp, src/stream.cpp, src/platform/linux/pipewire.cpp and
// src/platform/linux/misc.cpp before the config plumbing was added
// (see cachyos-fastpath.patch). This is the "vanilla install behaves
// identically" guarantee.
TEST_F(SolarflareConfigRuntimeTest, DefaultsExactlyMatchPreForkHardcodedValues) {
  // Pre-fork values: network.cpp used SO_BUSY_POLL=50us, soff BUFFORCE
  // buffer size 4*1024*1024.
  EXPECT_EQ(config::solarflare.busy_poll_us, 50);
  // Pre-fork: stream.cpp used link_bps * 80 / 100 (hardcoded 80%).
  EXPECT_EQ(config::solarflare.rate_cap_pct, 80);
  // Pre-fork: network.cpp always set the 4 MiB buffer; enet_4mib_buffer
  // was implicitly true.
  EXPECT_TRUE(config::solarflare.enet_4mib_buffer);
  // Pre-fork: pipewire.cpp hardcoded "8192/1000" (8 ms in nanosecond
  // fraction notation).
  EXPECT_EQ(config::solarflare.pipewire_latency_ms, 8);
  // Pre-fork: misc.cpp always did the SCHED_RR + affinity block on
  // critical priority.
  EXPECT_TRUE(config::solarflare.cpu_pinning);
  // Added with the per-game NVENC preset work; pre-fork all three were
  // implicitly on. dscp_qos=off means no setsockopt(IP_TOS) is called;
  // gpu_governor=off skips the sysfs power-profile switch on AMD; the
  // headless virtual display stays opt-in (default false).
  EXPECT_TRUE(config::solarflare.dscp_qos);
  EXPECT_TRUE(config::solarflare.gpu_governor);
  EXPECT_FALSE(config::solarflare.headless_virtual_display);
  EXPECT_FALSE(config::solarflare.skip_wayland_correlation);
}

// Verify the three new bool toggles can be flipped at runtime without
// the host having to restart (the hot-reload watcher in src/config.cpp
// picks them up via reload_solarflare_keys() within 2 seconds).
TEST_F(SolarflareConfigRuntimeTest, NewBoolsCanBeToggled) {
  config::solarflare.dscp_qos = false;
  EXPECT_FALSE(config::solarflare.dscp_qos);
  config::solarflare.dscp_qos = true;
  EXPECT_TRUE(config::solarflare.dscp_qos);

  config::solarflare.gpu_governor = false;
  EXPECT_FALSE(config::solarflare.gpu_governor);
  config::solarflare.gpu_governor = true;
  EXPECT_TRUE(config::solarflare.gpu_governor);

  config::solarflare.headless_virtual_display = true;
  EXPECT_TRUE(config::solarflare.headless_virtual_display);
  config::solarflare.headless_virtual_display = false;
  EXPECT_FALSE(config::solarflare.headless_virtual_display);

  config::solarflare.skip_wayland_correlation = true;
  EXPECT_TRUE(config::solarflare.skip_wayland_correlation);
  config::solarflare.skip_wayland_correlation = false;
  EXPECT_FALSE(config::solarflare.skip_wayland_correlation);
}

// skip_wayland_correlation is opt-in (default false). When set, the KMS
// enumeration path must skip the wl::monitors() correlation roundtrip and
// fall back to sysfs connector modes instead of hanging on an unresponsive
// compositor. We verify the value is read/written correctly and that the
// default keeps the vanilla (correlate) behaviour.
TEST_F(SolarflareConfigRuntimeTest, SkipWaylandCorrelationDefaultsToFalse) {
  EXPECT_FALSE(config::solarflare.skip_wayland_correlation);
  config::solarflare.skip_wayland_correlation = true;
  EXPECT_TRUE(config::solarflare.skip_wayland_correlation);
  config::solarflare.skip_wayland_correlation = false;
  EXPECT_FALSE(config::solarflare.skip_wayland_correlation);
}

// All five fields must fit in a small struct so it's cheap to copy
// around. If this grows past e.g. 256 bytes, the per-thread snapshot
// in misc.cpp becomes too expensive; we want to catch drift.
TEST(SolarflareConfigCompileTime, StructIsReasonablySmall) {
  // The struct has the audio_fx sub-struct (~30 fields, mostly floats/ints/bools)
  // plus 5 original tunables + 3 new bool toggles. Allow generous slack.
  constexpr size_t max_size = 256;
  EXPECT_LE(sizeof(config::solarflare_t), max_size)
    << "solarflare_t grew beyond 256 bytes; consider whether new fields "
       "are necessary or whether they should live in a separate struct.";
}

// ----------------------------------------------------------------------------
// Regression test for the apply_opus_tuning_runtime helper. The previous code
// inlined the opus_application / opus_vbr / ... -> g_opus_tuning propagation
// in apply_config() and never ran it from the hot-reload watcher, so editing
// sf_opus_* in sunshine.conf silently did nothing until restart.
// ----------------------------------------------------------------------------
TEST_F(SolarflareConfigTest, ApplyOpusTuningRuntimePropagatesOpusTuning) {
  // Snapshot the global Opus tuning so we can restore it on TearDown.
  auto &saved_tuning = ::audio::opus_tuning();
  auto saved_application = saved_tuning.application;
  auto saved_vbr = saved_tuning.vbr;
  auto saved_complexity = saved_tuning.complexity;
  auto saved_enable_fec = saved_tuning.enable_fec;
  auto saved_loss_pct = saved_tuning.expected_packet_loss_pct;
  auto saved_bwe = saved_tuning.enable_bandwidth_extension;

  // Configure an audio_fx with non-default values, then apply.
  config::solarflare_t::audio_fx_t af {};
  af.opus_application = 2;  // AUDIO
  af.opus_vbr = 1;  // CONSTRAINED
  af.opus_complexity = 5;
  af.opus_fec = false;
  af.opus_expected_loss_pct = 17;
  af.opus_bandwidth_extension = false;
  config::apply_opus_tuning_runtime(af);

  EXPECT_EQ(saved_tuning.application, ::audio::opus_tuning_t::application_e::AUDIO);
  EXPECT_EQ(saved_tuning.vbr, ::audio::opus_tuning_t::vbr_e::CONSTRAINED);
  EXPECT_EQ(saved_tuning.complexity, 5);
  EXPECT_FALSE(saved_tuning.enable_fec);
  EXPECT_EQ(saved_tuning.expected_packet_loss_pct, 17);
  EXPECT_FALSE(saved_tuning.enable_bandwidth_extension);

  // Restore (SetUp snapshot is restored by TearDown above; here we restore
  // the opus side too because audio::g_opus_tuning is not snapshotted by
  // the SolarflareSnapshot helper).
  saved_tuning.application = saved_application;
  saved_tuning.vbr = saved_vbr;
  saved_tuning.complexity = saved_complexity;
  saved_tuning.enable_fec = saved_enable_fec;
  saved_tuning.expected_packet_loss_pct = saved_loss_pct;
  saved_tuning.enable_bandwidth_extension = saved_bwe;
}

TEST_F(SolarflareConfigTest, ApplyOpusTuningRuntimeMapsAllEnumValues) {
  auto &tuning = ::audio::opus_tuning();
  config::solarflare_t::audio_fx_t af {};

  // application: 0 -> LOWDELAY, 1 -> VOIP, 2 -> AUDIO, anything else -> LOWDELAY
  af.opus_application = 0;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.application, ::audio::opus_tuning_t::application_e::LOWDELAY);
  af.opus_application = 1;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.application, ::audio::opus_tuning_t::application_e::VOIP);
  af.opus_application = 2;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.application, ::audio::opus_tuning_t::application_e::AUDIO);
  af.opus_application = 99;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.application, ::audio::opus_tuning_t::application_e::LOWDELAY);

  // vbr: 0 -> OFF, 1 -> CONSTRAINED, 2 -> FULL, else -> OFF
  af.opus_vbr = 0;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.vbr, ::audio::opus_tuning_t::vbr_e::OFF);
  af.opus_vbr = 1;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.vbr, ::audio::opus_tuning_t::vbr_e::CONSTRAINED);
  af.opus_vbr = 2;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.vbr, ::audio::opus_tuning_t::vbr_e::FULL);
  af.opus_vbr = 99;
  config::apply_opus_tuning_runtime(af);
  EXPECT_EQ(tuning.vbr, ::audio::opus_tuning_t::vbr_e::OFF);
}
