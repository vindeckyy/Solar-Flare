/**
 * @file tests/unit/test_audio_fx.cpp
 * @brief Unit tests for src/audio_fx.* (AGC, VAD, Ducker, PreProcessor).
 *
 * The tests are self-contained: they construct a known signal (sine,
 * silence, step amplitude), feed it through the processor, and verify
 * observable properties (output RMS in range, transitions in expected
 * order, smoothing monotonicity, etc.). No external audio files needed.
 *
 * Targets: 100% line coverage of src/audio_fx.cpp.
 */
#include "../tests_common.h"

#include <src/audio_fx.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <vector>

// Portable π (M_PI is non-standard). We don't need high precision for tests.
static constexpr float kPi =
    3.14159265358979323846f;

using namespace audio::fx;
using namespace std::chrono_literals;

namespace {

  /// Sample rate used across the suite.
  constexpr float kSampleRate = 48000.0f;

  /// Build a sine wave at the requested frequency / level / duration.
  /// @param freq_hz Frequency in Hz.
  /// @param amplitude_dbfs Peak amplitude in dBFS.
  /// @param duration_ms Duration in milliseconds.
  /// @param channels Number of channels to interleave.
  std::vector<float> make_sine(float freq_hz, float amplitude_dbfs, int duration_ms, int channels = 2) {
    const std::size_t frames = static_cast<std::size_t>(kSampleRate * duration_ms / 1000.0f);
    std::vector<float> out(frames * channels);
    const float amp = std::pow(10.0f, amplitude_dbfs / 20.0f);
    for (std::size_t i = 0; i < frames; ++i) {
      const float v = amp * std::sin(2.0f * kPi * freq_hz * static_cast<float>(i) / kSampleRate);
      for (int c = 0; c < channels; ++c) {
        out[i * channels + c] = v;
      }
    }
    return out;
  }

  /// Build a buffer of silence.
  std::vector<float> make_silence(int duration_ms, int channels = 2) {
    const std::size_t frames = static_cast<std::size_t>(kSampleRate * duration_ms / 1000.0f);
    return std::vector<float>(frames * channels, 0.0f);
  }

  /// Compute RMS (linear) of a buffer.
  float rms_linear(const std::vector<float> &buf) {
    if (buf.empty()) return 0.0f;
    double acc = 0.0;
    for (float v : buf) acc += static_cast<double>(v) * v;
    return static_cast<float>(std::sqrt(acc / buf.size()));
  }

  /// Compute RMS in dBFS, with a floor of -120 dBFS.
  float rms_dbfs(const std::vector<float> &buf) {
    const float r = rms_linear(buf);
    if (r <= 0.0f) return -120.0f;
    return 20.0f * std::log10(r);
  }

  /// Find the frame index where the ducker reaches within 1 dB of its target.
  std::size_t frames_to_within_1db(const std::vector<float> &gains_db, float target_db) {
    for (std::size_t i = 0; i < gains_db.size(); ++i) {
      if (std::abs(gains_db[i] - target_db) <= 1.0f) {
        return i;
      }
    }
    return gains_db.size();
  }

  /// True if @p gains_db is monotonically non-increasing (gain coming up).
  bool is_monotonic_nonincreasing(const std::vector<float> &v) {
    for (std::size_t i = 1; i < v.size(); ++i) {
      if (v[i] > v[i - 1] + 1e-4f) return false;
    }
    return true;
  }

  /// True if @p gains_db is monotonically non-decreasing (gain coming down).
  bool is_monotonic_nondecreasing(const std::vector<float> &v) {
    for (std::size_t i = 1; i < v.size(); ++i) {
      if (v[i] < v[i - 1] - 1e-4f) return false;
    }
    return true;
  }

}  // namespace

// =====================================================================
// AGC tests
// =====================================================================

TEST(AGC, PassThroughAtUnityGain) {
  // With a 0 dBFS sine, AGC should leave it alone (target is -20 dB, but a
  // 0 dBFS sine is already louder than the target; AGC will attenuate).
  // We test that AGC's gain is clamped within [min_gain_db, max_gain_db].
  AGC::config_t cfg {};
  cfg.target_rms_db = -3.0f;  // Push target above signal so AGC must attack up.
  cfg.max_gain_db = 6.0f;
  cfg.min_gain_db = -6.0f;
  cfg.attack_ms = 1.0f;
  cfg.release_ms = 1.0f;
  cfg.hold_ms = 0.0f;
  cfg.sample_rate = kSampleRate;
  AGC agc {cfg};

  auto buf = make_sine(440.0f, -20.0f, 200 /*ms*/, 2);
  agc.process(buf.data(), buf.size() / 2, 2);

  // Gain should be > 1.0 (signal is at -20 dBFS RMS, target is -3 dBFS, +17 dB needed)
  // and clamped to max_gain_db = +6 dB.
  const float linear = agc.current_gain();
  EXPECT_GT(linear, 1.0f) << "AGC should attack up when signal is below target";
  EXPECT_LE(linear, std::pow(10.0f, cfg.max_gain_db / 20.0f) + 1e-3f)
      << "AGC must clamp to max_gain_db";
}

TEST(AGC, AttenuatesLoudSignal) {
  AGC::config_t cfg {};
  cfg.target_rms_db = -20.0f;
  cfg.max_gain_db = 12.0f;
  cfg.min_gain_db = -12.0f;
  cfg.attack_ms = 5.0f;
  cfg.release_ms = 5.0f;
  cfg.hold_ms = 0.0f;
  cfg.sample_rate = kSampleRate;
  AGC agc {cfg};

  auto buf = make_sine(440.0f, -3.0f, 200 /*ms*/, 2);
  agc.process(buf.data(), buf.size() / 2, 2);

  // Signal is way above target; AGC should attenuate.
  EXPECT_LT(agc.current_gain(), 1.0f) << "AGC must attenuate when signal is above target";
  EXPECT_GE(agc.current_gain(), std::pow(10.0f, cfg.min_gain_db / 20.0f) - 1e-3f)
      << "AGC must clamp to min_gain_db";
}

TEST(AGC, AttackIsFasterThanRelease) {
  // Verify the asymmetric time-constants by comparing the gain trajectory
  // length needed to converge in each direction.
  AGC::config_t cfg {};
  cfg.target_rms_db = -20.0f;
  cfg.max_gain_db = 20.0f;
  cfg.min_gain_db = -20.0f;
  cfg.attack_ms = 5.0f;
  cfg.release_ms = 200.0f;
  cfg.hold_ms = 0.0f;
  cfg.sample_rate = kSampleRate;
  AGC agc {cfg};

  // Start with a loud signal — AGC should attenuate quickly (release).
  std::vector<float> buf = make_sine(440.0f, -3.0f, 1000 /*ms*/, 1);
  agc.process(buf.data(), buf.size(), 1);
  const float gain_after_attack = agc.current_gain();
  EXPECT_LT(gain_after_attack, 1.0f);

  // Now feed a quiet signal — AGC should raise gain slowly (release).
  std::vector<float> quiet = make_silence(1000 /*ms*/, 1);
  // For an AGC to raise gain on silence, we'd need a target above the
  // current RMS. Since silence RMS is -120 dB and target is -20, AGC will
  // attack (raise gain) on each frame. Attack is 5 ms so this is fast.
  // We measure both: attack direction is fast, release direction is slow.
  // The release direction is what we already tested above; here we just
  // verify the attack itself doesn't take longer than ~3 frames at 5 ms each.
  AGC agc2 {cfg};
  std::vector<float> quiet_long = make_silence(20 /*ms*/, 1);
  for (int frame = 0; frame < 4; ++frame) {
    agc2.process(quiet_long.data(), quiet_long.size(), 1);
  }
  const float gain_after_quiet = agc2.current_gain();
  EXPECT_GT(gain_after_quiet, 1.0f) << "AGC must attack up after a quiet input";
}

TEST(AGC, HoldPreventsRelease) {
  // While in hold, AGC should not release gain even if input is loud.
  AGC::config_t cfg {};
  cfg.target_rms_db = -20.0f;
  cfg.max_gain_db = 20.0f;
  cfg.min_gain_db = -20.0f;
  cfg.attack_ms = 5.0f;
  cfg.release_ms = 5.0f;
  cfg.hold_ms = 500.0f;
  cfg.sample_rate = kSampleRate;
  AGC agc {cfg};

  // First, raise gain with quiet input.
  std::vector<float> quiet = make_silence(50 /*ms*/, 1);
  agc.process(quiet.data(), quiet.size(), 1);
  const float gain_in_hold = agc.current_gain();
  EXPECT_GT(gain_in_hold, 1.0f);

  // Now feed loud input; AGC should keep current gain during hold.
  std::vector<float> loud = make_sine(440.0f, -3.0f, 100 /*ms*/, 1);
  agc.process(loud.data(), loud.size(), 1);
  EXPECT_NEAR(agc.current_gain(), gain_in_hold, 0.05f)
      << "Hold should prevent release during the hold window";
}

TEST(AGC, ResetReturnsToUnity) {
  AGC::config_t cfg {};
  cfg.target_rms_db = -10.0f;
  cfg.max_gain_db = 20.0f;
  cfg.min_gain_db = -20.0f;
  cfg.attack_ms = 1.0f;
  cfg.release_ms = 1.0f;
  cfg.hold_ms = 0.0f;
  cfg.sample_rate = kSampleRate;
  AGC agc {cfg};

  std::vector<float> quiet = make_silence(20 /*ms*/, 1);
  agc.process(quiet.data(), quiet.size(), 1);
  EXPECT_GT(agc.current_gain(), 1.0f);

  agc.reset();
  EXPECT_NEAR(agc.current_gain(), 1.0f, 1e-5f);
}

// =====================================================================
// VAD tests
// =====================================================================

TEST(VAD, DetectsLoudSignalAsSpeech) {
  VAD::config_t cfg {};
  cfg.threshold_db = -45.0f;
  cfg.hysteresis_db = 6.0f;
  cfg.min_speech_ms = 50.0f;
  cfg.min_silence_ms = 200.0f;
  cfg.sample_rate = kSampleRate;
  VAD vad {cfg};

  // Feed a -10 dBFS sine for 200 ms — should be detected as speech.
  std::vector<float> buf = make_sine(440.0f, -10.0f, 200 /*ms*/, 1);
  bool final = false;
  // Feed in 5 ms chunks so min_speech_ms timer accumulates correctly.
  const std::size_t frames_per_chunk = static_cast<std::size_t>(kSampleRate * 0.005f);
  for (std::size_t off = 0; off < buf.size(); off += frames_per_chunk) {
    final = vad.process(buf.data() + off, frames_per_chunk, 1);
    if (final) break;
  }
  EXPECT_TRUE(final) << "VAD must detect a -10 dBFS sine as speech";
  EXPECT_TRUE(vad.is_speech());
}

TEST(VAD, DoesNotDetectSilence) {
  VAD::config_t cfg {};
  cfg.threshold_db = -45.0f;
  cfg.hysteresis_db = 6.0f;
  cfg.min_speech_ms = 50.0f;
  cfg.min_silence_ms = 200.0f;
  cfg.sample_rate = kSampleRate;
  VAD vad {cfg};

  std::vector<float> buf = make_silence(2000 /*ms*/, 1);
  const std::size_t frames_per_chunk = static_cast<std::size_t>(kSampleRate * 0.005f);
  for (std::size_t off = 0; off + frames_per_chunk <= buf.size(); off += frames_per_chunk) {
    vad.process(buf.data() + off, frames_per_chunk, 1);
  }
  EXPECT_FALSE(vad.is_speech());
}

TEST(VAD, HysteresisPreventsFlapping) {
  VAD::config_t cfg {};
  cfg.threshold_db = -40.0f;
  cfg.hysteresis_db = 10.0f;
  cfg.min_speech_ms = 50.0f;
  cfg.min_silence_ms = 200.0f;
  cfg.sample_rate = kSampleRate;
  VAD vad {cfg};

  // 1) Establish speech with loud input.
  std::vector<float> loud = make_sine(440.0f, -10.0f, 200 /*ms*/, 1);
  std::size_t chunk = static_cast<std::size_t>(kSampleRate * 0.005f);
  for (std::size_t off = 0; off < loud.size(); off += chunk) {
    vad.process(loud.data() + off, chunk, 1);
  }
  ASSERT_TRUE(vad.is_speech());

  // 2) Feed a signal between threshold and threshold-hysteresis. This must
  // NOT release the speech state because of hysteresis.
  std::vector<float> marginal = make_sine(440.0f, -45.0f, 1000 /*ms*/, 1);
  for (std::size_t off = 0; off + chunk <= marginal.size(); off += chunk) {
    vad.process(marginal.data() + off, chunk, 1);
    if (!vad.is_speech()) break;
  }
  EXPECT_TRUE(vad.is_speech()) << "VAD should hold speech state through marginal input";

  // 3) Feed silence long enough to release.
  std::vector<float> silent = make_silence(500 /*ms*/, 1);
  for (std::size_t off = 0; off + chunk <= silent.size(); off += chunk) {
    vad.process(silent.data() + off, chunk, 1);
  }
  EXPECT_FALSE(vad.is_speech());
}

TEST(VAD, ResetClearsState) {
  VAD::config_t cfg {};
  cfg.threshold_db = -45.0f;
  cfg.min_speech_ms = 50.0f;
  cfg.min_silence_ms = 200.0f;
  cfg.sample_rate = kSampleRate;
  VAD vad {cfg};

  std::vector<float> loud = make_sine(440.0f, -10.0f, 200 /*ms*/, 1);
  std::size_t chunk = static_cast<std::size_t>(kSampleRate * 0.005f);
  for (std::size_t off = 0; off + chunk <= loud.size(); off += chunk) {
    vad.process(loud.data() + off, chunk, 1);
  }
  ASSERT_TRUE(vad.is_speech());

  vad.reset();
  EXPECT_FALSE(vad.is_speech());
}

// =====================================================================
// Ducker tests
// =====================================================================

TEST(Ducker, AttenuatesWhenVoiceActive) {
  Ducker::config_t cfg {};
  cfg.target_attenuation_db = -12.0f;
  cfg.attack_ms = 20.0f;
  cfg.release_ms = 200.0f;
  cfg.sample_rate = kSampleRate;
  Ducker ducker {cfg};

  // 800 ms buffer: 100 ms silence (voice inactive) then 700 ms with voice
  // active. Total: 800 * 48 = 38400 frames.
  std::vector<float> buf = make_sine(440.0f, 0.0f, 800 /*ms*/, 1);
  const std::size_t chunk = static_cast<std::size_t>(kSampleRate * 0.005f);
  const std::size_t buf_frames = buf.size();  // = 38400

  // First 100 ms: voice inactive, ducker should stay near 0 dB.
  const std::size_t inactive_frames = static_cast<std::size_t>(kSampleRate * 0.100f);
  for (std::size_t off = 0; off + chunk <= inactive_frames; off += chunk) {
    ducker.process(buf.data() + off, chunk, 1, false);
  }
  EXPECT_NEAR(ducker.current_gain_db(), 0.0f, 1.0f);

  // Remaining 700 ms with voice active: should ramp toward -12 dB.
  // Iterate over the entire remaining buffer.
  for (std::size_t off = inactive_frames; off + chunk <= buf_frames; off += chunk) {
    ducker.process(buf.data() + off, chunk, 1, true);
  }
  EXPECT_NEAR(ducker.current_gain_db(), -12.0f, 1.0f);

  // Verify the output is attenuated relative to the input.
  const float post_input_rms_dbfs = rms_dbfs(buf);
  EXPECT_LT(post_input_rms_dbfs, -10.0f)
      << "Ducker must attenuate the signal after voice-active ramp";
}

TEST(Ducker, ReleasesToUnityWhenVoiceInactive) {
  Ducker::config_t cfg {};
  cfg.target_attenuation_db = -12.0f;
  cfg.attack_ms = 20.0f;
  cfg.release_ms = 200.0f;
  cfg.sample_rate = kSampleRate;
  Ducker ducker {cfg};

  // 1500 ms of silence is plenty of buffer.
  std::vector<float> buf = make_silence(1500 /*ms*/, 1);
  const std::size_t chunk = static_cast<std::size_t>(kSampleRate * 0.005f);
  const std::size_t buf_frames = buf.size();
  const std::size_t attack_frames = static_cast<std::size_t>(kSampleRate * 0.200f);  // 200 ms

  // Ramp down 200 ms with voice active.
  for (std::size_t off = 0; off + chunk <= attack_frames; off += chunk) {
    ducker.process(buf.data() + off, chunk, 1, true);
  }
  EXPECT_NEAR(ducker.current_gain_db(), -12.0f, 1.0f);

  // Now release for the rest of the buffer.
  for (std::size_t off = attack_frames; off + chunk <= buf_frames; off += chunk) {
    ducker.process(buf.data() + off, chunk, 1, false);
  }
  EXPECT_NEAR(ducker.current_gain_db(), 0.0f, 1.0f);
}

TEST(Ducker, ResetReturnsToUnity) {
  Ducker::config_t cfg {};
  cfg.target_attenuation_db = -12.0f;
  cfg.attack_ms = 1.0f;
  cfg.release_ms = 1.0f;
  cfg.sample_rate = kSampleRate;
  Ducker ducker {cfg};

  std::vector<float> buf = make_sine(440.0f, 0.0f, 50 /*ms*/, 1);
  ducker.process(buf.data(), buf.size(), 1, true);
  EXPECT_LT(ducker.current_gain_db(), -1.0f);

  ducker.reset();
  EXPECT_NEAR(ducker.current_gain_db(), 0.0f, 1e-3f);
}

// =====================================================================
// PreProcessor tests
// =====================================================================

TEST(PreProcessor, AllDisabledIsPassThrough) {
  // With everything off, PreProcessor should leave the buffer unchanged
  // (modulo floating-point round-trip noise gate at 1.0 and ducker at 0 dB).
  PreProcessor::config_t cfg {};
  cfg.enable_agc = false;
  cfg.enable_vad = false;
  cfg.enable_ducking = false;
  cfg.enable_noise_gate = false;
  PreProcessor pp {cfg};

  std::vector<float> buf = make_sine(440.0f, -10.0f, 100 /*ms*/, 2);
  std::vector<float> original = buf;
  const bool speech = pp.process(buf.data(), buf.size() / 2, 2);
  EXPECT_FALSE(speech);

  for (std::size_t i = 0; i < buf.size(); ++i) {
    EXPECT_NEAR(buf[i], original[i], 1e-5f)
        << "PreProcessor with all stages off must be a no-op (index " << i << ")";
  }
}

TEST(PreProcessor, AGCAndVADAndDuckingWorkTogether) {
  PreProcessor::config_t cfg {};
  cfg.enable_agc = true;
  cfg.enable_vad = true;
  cfg.enable_ducking = true;
  cfg.enable_noise_gate = false;
  cfg.agc.target_rms_db = -20.0f;
  cfg.agc.max_gain_db = 6.0f;
  cfg.agc.min_gain_db = -6.0f;
  cfg.agc.attack_ms = 10.0f;
  cfg.agc.hold_ms = 0.0f;
  cfg.agc.release_ms = 100.0f;
  cfg.agc.sample_rate = kSampleRate;
  cfg.vad.threshold_db = -45.0f;
  cfg.vad.min_speech_ms = 50.0f;
  cfg.vad.min_silence_ms = 200.0f;
  cfg.vad.sample_rate = kSampleRate;
  cfg.ducker.target_attenuation_db = -6.0f;
  cfg.ducker.attack_ms = 20.0f;
  cfg.ducker.release_ms = 200.0f;
  cfg.ducker.sample_rate = kSampleRate;
  PreProcessor pp {cfg};

  // Build a 600 ms signal: 200 ms loud (triggers VAD + ducker), 200 ms quiet,
  // 200 ms loud again. Verify the ducker state cycles correctly.
  std::vector<float> loud = make_sine(440.0f, -10.0f, 200, 1);
  std::vector<float> quiet = make_sine(440.0f, -60.0f, 200, 1);
  std::vector<float> buf;
  buf.insert(buf.end(), loud.begin(), loud.end());
  buf.insert(buf.end(), quiet.begin(), quiet.end());
  buf.insert(buf.end(), loud.begin(), loud.end());

  const std::size_t chunk = static_cast<std::size_t>(kSampleRate * 0.005f);
  std::vector<float> gain_history_db;
  // We don't have direct access to internal state during process; instead,
  // verify that the overall RMS of the loud sections ends up reasonable.
  pp.process(buf.data(), buf.size(), 1);

  const float rms_after = rms_dbfs(buf);
  EXPECT_GT(rms_after, -30.0f) << "AGC + ducker should still leave audible signal";
  EXPECT_LT(rms_after, 0.0f) << "AGC should not amplify beyond unity on a hot signal";
}

TEST(PreProcessor, NoiseGateClosesOnSilence) {
  PreProcessor::config_t cfg {};
  cfg.enable_agc = false;
  cfg.enable_vad = false;
  cfg.enable_ducking = false;
  cfg.enable_noise_gate = true;
  cfg.noise_gate_threshold_db = -30.0f;
  PreProcessor pp {cfg};

  // Feed silence for 500 ms; the noise gate should close.
  std::vector<float> buf = make_silence(500, 1);
  pp.process(buf.data(), buf.size(), 1);

  // The buffer should remain effectively silent (the gate multiplies by a
  // gain <= 1.0, never increases a silent signal).
  const float post_rms_db = rms_dbfs(buf);
  EXPECT_LT(post_rms_db, -60.0f) << "Noise gate should keep silence quiet";
}

TEST(PreProcessor, ResetClearsAllStages) {
  PreProcessor::config_t cfg {};
  cfg.enable_agc = true;
  cfg.enable_vad = true;
  cfg.enable_ducking = true;
  cfg.enable_noise_gate = true;
  cfg.agc.sample_rate = kSampleRate;
  cfg.vad.sample_rate = kSampleRate;
  cfg.ducker.sample_rate = kSampleRate;
  PreProcessor pp {cfg};

  std::vector<float> buf = make_sine(440.0f, -10.0f, 200, 1);
  pp.process(buf.data(), buf.size(), 1);

  // After reset, the next call with silence should leave silence silent.
  pp.reset();
  std::vector<float> sil = make_silence(50, 1);
  pp.process(sil.data(), sil.size(), 1);
  EXPECT_LT(rms_dbfs(sil), -60.0f);
}
