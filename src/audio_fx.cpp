/**
 * @file src/audio_fx.cpp
 * @brief Implementation of pre-encoder audio effects.
 *
 * All DSP is done in single precision (32-bit float). The algorithms are
 * intentionally simple, well-documented, and easy to verify against test
 * vectors — these are not exotic processors, just clean implementations of
 * the textbook AGC / VAD / ducker.
 *
 * Time-constants are expressed in milliseconds and converted to per-sample
 * coefficients at construction time using an exponential moving-average
 * approximation:
 *
 *   alpha = 1 - exp(-frame_ms / tau_ms)
 *
 * This is the standard one-pole IIR smoothing used in digital audio. It is
 * stable at any sample rate and asymptotically approaches the target.
 */
#include "audio_fx.h"

#include <algorithm>
#include <cmath>

namespace audio::fx {

  namespace detail {
    /**
     * @brief Convert milliseconds to a per-sample smoothing coefficient.
     *
     * @param tau_ms Time-constant in milliseconds (e.g. 10 ms attack).
     * @param frame_ms Frame duration in milliseconds (e.g. 5 ms).
     * @return Coefficient in [0, 1] suitable for one-pole IIR smoothing.
     */
    inline float ms_to_alpha(float tau_ms, float frame_ms) noexcept {
      if (tau_ms <= 0.0f || frame_ms <= 0.0f) {
        return 1.0f;
      }
      // 1 - exp(-frame_ms / tau_ms)
      return 1.0f - std::exp(-frame_ms / tau_ms);
    }

    /**
     * @brief Compute RMS level (dBFS) of an interleaved buffer.
     *
     * Returns -200 dBFS for silence (effectively -inf) so callers can use it
     * directly in comparisons.
     *
     * @param samples Interleaved float samples (read-only).
     * @param frame_count Number of frames (samples per channel).
     * @param channels Number of interleaved channels.
     * @return RMS level in dBFS.
     */
    inline float rms_db(const float *samples, std::size_t frame_count, int channels) noexcept {
      if (!samples || frame_count == 0 || channels <= 0) {
        return -200.0f;
      }
      double acc = 0.0;
      const std::size_t total = frame_count * static_cast<std::size_t>(channels);
      for (std::size_t i = 0; i < total; ++i) {
        const double v = static_cast<double>(samples[i]);
        acc += v * v;
      }
      const double mean = acc / static_cast<double>(total);
      if (mean <= 0.0) {
        return -200.0f;
      }
      return static_cast<float>(10.0 * std::log10(mean));
    }

    /**
     * @brief Convert a dB value to a linear gain multiplier.
     * @param db Value in dB.
     * @return Linear gain (1.0 = unity).
     */
    inline float db_to_linear(float db) noexcept {
      return std::pow(10.0f, db / 20.0f);
    }

    /**
     * @brief Convert a linear gain to dB.
     * @param linear Linear gain (1.0 = unity).
     * @return Value in dB.
     */
    inline float linear_to_db(float linear) noexcept {
      if (linear <= 0.0f) {
        return -200.0f;
      }
      return 20.0f * std::log10(linear);
    }
  }  // namespace detail

  // ---------------------------------------------------------------------
  // AGC
  // ---------------------------------------------------------------------

  AGC::AGC(): AGC(config_t {}) {}

  AGC::AGC(const config_t &cfg):
      _cfg(cfg) {
    if (_cfg.sample_rate <= 0.0f) {
      _cfg.sample_rate = 48000.0f;
    }
    // Pre-compute frame_ms for a 5 ms default frame; will be re-derived from
    // the actual frame size passed to process(). Stored here only for
    // initial alpha so a single-frame call does not see a 0-ms alpha.
    _frame_ms = 5.0f;
  }

  void AGC::process(float *samples, std::size_t frame_count, int channels) {
    if (!samples || frame_count == 0 || channels <= 0) {
      return;
    }

    // Derive the frame duration from the actual call so per-frame smoothing
    // is correct regardless of how process() is invoked.
    const float frame_ms = (1000.0f * static_cast<float>(frame_count)) / _cfg.sample_rate;
    _frame_ms = frame_ms;

    const float attack_alpha = detail::ms_to_alpha(_cfg.attack_ms, frame_ms);
    const float release_alpha = detail::ms_to_alpha(_cfg.release_ms, frame_ms);

    const float rms = detail::rms_db(samples, frame_count, channels);
    // Required gain (linear) to bring observed RMS up to the target.
    const float needed_gain_db = _cfg.target_rms_db - rms;
    const float clamped_db = std::clamp(needed_gain_db, _cfg.min_gain_db, _cfg.max_gain_db);
    const float target_gain_linear = detail::db_to_linear(clamped_db);

    // Asymmetric smoothing:
    //   - target > current  -> attack (raise gain quickly toward target)
    //   - target < current  -> release (lower gain slowly, after hold)
    if (target_gain_linear > _current_gain_linear) {
      // We need more gain — attack quickly toward target. Reset the hold
      // timer so each new loud event re-arms the hold for `hold_ms`.
      _current_gain_linear += (target_gain_linear - _current_gain_linear) * attack_alpha;
      _hold_remaining_samples = (_cfg.hold_ms / 1000.0f) * _cfg.sample_rate;
    } else {
      // Target is lower (we want to back off) — wait out the hold first.
      const float hold_remaining_samples_f = _hold_remaining_samples;
      if (hold_remaining_samples_f <= 0.0f) {
        _current_gain_linear += (target_gain_linear - _current_gain_linear) * release_alpha;
      } else {
        // Hold is active — keep current gain, decrement the hold timer.
        const float consumed = static_cast<float>(frame_count);
        _hold_remaining_samples = std::max(0.0f, _hold_remaining_samples - consumed);
      }
    }

    // Guard against numerical drift that could push the gain out of bounds.
    const float gain = std::clamp(_current_gain_linear, 0.01f, 8.0f);
    _current_gain_linear = gain;

    const std::size_t total = frame_count * static_cast<std::size_t>(channels);
    for (std::size_t i = 0; i < total; ++i) {
      samples[i] *= gain;
    }
  }

  void AGC::reset() {
    _current_gain_linear = 1.0f;
    _hold_remaining_samples = 0.0f;
  }

  float AGC::current_gain() const noexcept {
    return _current_gain_linear;
  }

  // ---------------------------------------------------------------------
  // VAD
  // ---------------------------------------------------------------------

  VAD::VAD(): VAD(config_t {}) {}

  VAD::VAD(const config_t &cfg):
      _cfg(cfg) {
    if (_cfg.sample_rate <= 0.0f) {
      _cfg.sample_rate = 48000.0f;
    }
    _frame_ms = 5.0f;
  }

  bool VAD::process(const float *samples, std::size_t frame_count, int channels) {
    if (!samples || frame_count == 0 || channels <= 0) {
      return _is_speech;
    }

    const float frame_ms = (1000.0f * static_cast<float>(frame_count)) / _cfg.sample_rate;
    _frame_ms = frame_ms;

    const float rms = detail::rms_db(samples, frame_count, channels);
    const float enter_threshold = _cfg.threshold_db;
    const float exit_threshold = _cfg.threshold_db - _cfg.hysteresis_db;

    if (!_is_speech) {
      if (rms >= enter_threshold) {
        _speech_samples += frame_count;
        _silence_samples = 0.0f;
        if ((_speech_samples / _cfg.sample_rate) * 1000.0f >= _cfg.min_speech_ms) {
          _is_speech = true;
        }
      } else {
        _speech_samples = 0.0f;
      }
    } else {
      if (rms <= exit_threshold) {
        _silence_samples += frame_count;
        if ((_silence_samples / _cfg.sample_rate) * 1000.0f >= _cfg.min_silence_ms) {
          _is_speech = false;
          _speech_samples = 0.0f;
          _silence_samples = 0.0f;
        }
      } else {
        _silence_samples = 0.0f;
      }
    }

    return _is_speech;
  }

  void VAD::reset() {
    _is_speech = false;
    _speech_samples = 0.0f;
    _silence_samples = 0.0f;
  }

  bool VAD::is_speech() const noexcept {
    return _is_speech;
  }

  // ---------------------------------------------------------------------
  // Ducker
  // ---------------------------------------------------------------------

  Ducker::Ducker(): Ducker(config_t {}) {}

  Ducker::Ducker(const config_t &cfg):
      _cfg(cfg) {
    if (_cfg.sample_rate <= 0.0f) {
      _cfg.sample_rate = 48000.0f;
    }
    _frame_ms = 5.0f;
  }

  void Ducker::process(float *samples, std::size_t frame_count, int channels, bool voice_active) {
    if (!samples || frame_count == 0 || channels <= 0) {
      return;
    }

    const float frame_ms = (1000.0f * static_cast<float>(frame_count)) / _cfg.sample_rate;
    _frame_ms = frame_ms;

    const float target_db = voice_active ? _cfg.target_attenuation_db : 0.0f;
    const float tau_ms = (target_db < _current_gain_db) ? _cfg.attack_ms : _cfg.release_ms;
    const float alpha = detail::ms_to_alpha(tau_ms, frame_ms);

    _current_gain_db += (target_db - _current_gain_db) * alpha;

    const float gain = detail::db_to_linear(_current_gain_db);
    const std::size_t total = frame_count * static_cast<std::size_t>(channels);
    for (std::size_t i = 0; i < total; ++i) {
      samples[i] *= gain;
    }
  }

  void Ducker::reset() {
    _current_gain_db = 0.0f;
  }

  float Ducker::current_gain_db() const noexcept {
    return _current_gain_db;
  }

  // ---------------------------------------------------------------------
  // PreProcessor (composite)
  // ---------------------------------------------------------------------

  PreProcessor::PreProcessor(): PreProcessor(config_t {}) {}

  PreProcessor::PreProcessor(const config_t &cfg):
      _cfg(cfg),
      _agc(cfg.agc),
      _vad(cfg.vad),
      _ducker(cfg.ducker) {
    // Ensure sub-processors share the same sample-rate assumption.
    if (_cfg.agc.sample_rate <= 0.0f) _cfg.agc.sample_rate = 48000.0f;
    if (_cfg.vad.sample_rate <= 0.0f) _cfg.vad.sample_rate = 48000.0f;
    if (_cfg.ducker.sample_rate <= 0.0f) _cfg.ducker.sample_rate = 48000.0f;
    _agc = AGC(_cfg.agc);
    _vad = VAD(_cfg.vad);
    _ducker = Ducker(_cfg.ducker);
  }

  bool PreProcessor::process(float *samples, std::size_t frame_count, int channels) {
    if (!samples || frame_count == 0 || channels <= 0) {
      return false;
    }

    // Stage 1: AGC (gain smoothing).
    if (_cfg.enable_agc) {
      _agc.process(samples, frame_count, channels);
    }

    // Stage 2: VAD observes the post-AGC signal so its threshold is relative
    // to the consistent post-AGC level.
    bool speech = false;
    if (_cfg.enable_vad) {
      speech = _vad.process(samples, frame_count, channels);
    }

    // Stage 3: Noise gate (after AGC so the gate sees normalized levels).
    if (_cfg.enable_noise_gate) {
      const float rms = detail::rms_db(samples, frame_count, channels);
      if (rms < _cfg.noise_gate_threshold_db) {
        // Below the threshold — exponentially decay the gate toward silence.
        const float gate_release = detail::ms_to_alpha(20.0f, (1000.0f * static_cast<float>(frame_count)) / 48000.0f);
        _noise_gate_gain += (0.0f - _noise_gate_gain) * gate_release;
      } else {
        // Above the threshold — open the gate quickly.
        const float gate_attack = detail::ms_to_alpha(5.0f, (1000.0f * static_cast<float>(frame_count)) / 48000.0f);
        _noise_gate_gain += (1.0f - _noise_gate_gain) * gate_attack;
      }
      const float g = _noise_gate_gain;
      const std::size_t total = frame_count * static_cast<std::size_t>(channels);
      for (std::size_t i = 0; i < total; ++i) {
        samples[i] *= g;
      }
    }

    // Stage 4: Ducker (acts on the now-normalized signal).
    if (_cfg.enable_ducking) {
      _ducker.process(samples, frame_count, channels, speech);
    }

    return speech;
  }

  void PreProcessor::reset() {
    _agc.reset();
    _vad.reset();
    _ducker.reset();
    _noise_gate_gain = 1.0f;
  }

}  // namespace audio::fx
