/**
 * @file src/audio_fx.h
 * @brief Pre-encoder audio effects for Sunshine/SolarFlare.
 *
 * Provides a small set of in-place, single-precision DSP utilities that run on
 * the captured audio buffer immediately before Opus encoding:
 *
 *   - Automatic Gain Control (AGC) with attack / hold / release
 *   - Voice Activity Detection (VAD) — RMS + hysteresis
 *   - Stream ducker — smooth attenuation when voice is active
 *   - Noise gate — kill mic-style noise floor
 *   - PreProcessor — composite orchestrator
 *
 * All processors operate on interleaved 32-bit float samples and are designed
 * to be called once per Opus frame (5–10 ms @ 48 kHz). They are allocation-
 * free after construction and safe to call from a single producer thread.
 *
 * The defaults are tuned for game-streaming audio: mixed voice + SFX + music,
 * 48 kHz stereo, 5 ms frames. They are intentionally conservative so a
 * misconfigured system never sounds worse than the unprocessed signal.
 *
 * These processors are OFF by default and must be enabled via the SolarFlare
 * configuration (see config::solarflare in src/config.h) so that the upstream
 * Sunshine behaviour is preserved when the new options are not set.
 */
#pragma once

#include <cstddef>

namespace audio::fx {

  /**
   * @brief Automatic Gain Control with attack / hold / release smoothing.
   *
   * Adjusts gain so the output RMS approaches @c target_rms_db, without the
   * audible "pumping" caused by instantaneous gain changes. The @c hold_ms
   * delay before release prevents the gain from being pulled back down on
   * brief pauses in speech.
   *
   * Thread model: single producer thread (the audio encode thread).
   * State is held by value; one instance per stream.
   */
  class AGC {
  public:
    /**
     * @brief AGC tuning parameters.
     *
     * Defaults are tuned for voice + game-SFX streaming.
     */
    struct config_t {
      /// Target output RMS level in dBFS (typical: -20 dBFS).
      float target_rms_db = -20.0f;
      /// Maximum positive gain in dB (e.g. 12 dB = 4x).
      float max_gain_db = 12.0f;
      /// Maximum attenuation in dB (e.g. -12 dB = 0.25x).
      float min_gain_db = -12.0f;
      /// Time-constant (ms) used while raising gain.
      float attack_ms = 10.0f;
      /// Hold time (ms) at peak gain before allowing release.
      float hold_ms = 200.0f;
      /// Time-constant (ms) used while reducing gain back toward unity.
      float release_ms = 100.0f;
      /// Sample rate (Hz). Must match the actual buffer sample rate.
      float sample_rate = 48000.0f;
    };

    /**
     * @brief Construct an AGC instance with default tuning.
     */
    AGC();

    /**
     * @brief Construct an AGC instance with custom tuning.
     * @param cfg Tuning parameters (see ::config_t).
     */
    explicit AGC(const config_t &cfg);

    /**
     * @brief Apply gain in-place to an interleaved float buffer.
     *
     * Operates per-frame so the smoothing time-constants work correctly. The
     * buffer is modified in place; channels are interleaved (LRLRLR...).
     *
     * @param samples Interleaved float samples (modified in place).
     * @param frame_count Number of frames (samples per channel).
     * @param channels Number of interleaved channels (typically 2).
     */
    void process(float *samples, std::size_t frame_count, int channels);

    /**
     * @brief Reset internal state (current gain, hold timer).
     *
     * Call when starting a new stream to avoid carrying over gain from the
     * previous session.
     */
    void reset();

    /**
     * @brief Current linear gain (read-only, for telemetry / tests).
     * @return Gain as linear multiplier (1.0 = unity).
     */
    float current_gain() const noexcept;

  private:
    config_t _cfg;
    float _current_gain_linear = 1.0f;
    float _hold_remaining_samples = 0.0f;
    float _frame_ms = 0.0f;
  };

  /**
   * @brief Voice Activity Detector based on RMS level with hysteresis.
   *
   * Outputs a binary "speech / silence" decision with a minimum-duration
   * requirement so very short transients do not trigger it.
   *
   * The detector does not modify the audio — it only reports the decision.
   * Use the result to drive a ::Ducker or to gate the Opus encoder.
   */
  class VAD {
  public:
    /**
     * @brief VAD tuning parameters.
     */
    struct config_t {
      /// RMS level (dBFS) above which input is considered speech.
      float threshold_db = -45.0f;
      /// Hysteresis (dB) below @c threshold_db before flipping back to silence.
      float hysteresis_db = 6.0f;
      /// Minimum duration (ms) above threshold to declare speech.
      float min_speech_ms = 100.0f;
      /// Minimum duration (ms) below threshold to declare silence again.
      float min_silence_ms = 200.0f;
      /// Sample rate (Hz). Must match the actual buffer sample rate.
      float sample_rate = 48000.0f;
    };

    /**
     * @brief Construct a VAD instance with default tuning.
     */
    VAD();

    /**
     * @brief Construct a VAD instance with custom tuning.
     * @param cfg Tuning parameters.
     */
    explicit VAD(const config_t &cfg);

    /**
     * @brief Observe a frame and update the speech / silence decision.
     *
     * @param samples Interleaved float samples (read-only).
     * @param frame_count Number of frames (samples per channel).
     * @param channels Number of interleaved channels.
     * @return true if the detector currently classifies the input as speech.
     */
    bool process(const float *samples, std::size_t frame_count, int channels);

    /**
     * @brief Reset internal timers / state.
     */
    void reset();

    /**
     * @brief Current decision (read-only).
     * @return true if speech is currently active.
     */
    bool is_speech() const noexcept;

  private:
    config_t _cfg;
    bool _is_speech = false;
    float _speech_samples = 0.0f;
    float _silence_samples = 0.0f;
    float _frame_ms = 0.0f;
  };

  /**
   * @brief Stream ducker: smoothly attenuates the signal when voice is active.
   *
   * Useful when the captured audio mixes voice-chat (e.g. Discord) with
   * game SFX / music: when someone speaks, the game audio is briefly ducked
   * so the voice is intelligible, then restored during pauses.
   *
   * The ducker takes its voice-activity cue from a ::VAD (or any other
   * source); it does not look at the audio itself.
   */
  class Ducker {
  public:
    /**
     * @brief Ducker tuning parameters.
     */
    struct config_t {
      /// How much to duck (dB) when speech is active (negative = quieter).
      float target_attenuation_db = -12.0f;
      /// Time-constant (ms) for ducking (reducing gain).
      float attack_ms = 50.0f;
      /// Time-constant (ms) for releasing back to unity.
      float release_ms = 500.0f;
      /// Sample rate (Hz). Must match the actual buffer sample rate.
      float sample_rate = 48000.0f;
    };

    /**
     * @brief Construct a Ducker instance with default tuning.
     */
    Ducker();

    /**
     * @brief Construct a Ducker instance with custom tuning.
     * @param cfg Tuning parameters.
     */
    explicit Ducker(const config_t &cfg);

    /**
     * @brief Apply ducking in-place.
     *
     * The decision to duck is taken from @c voice_active (typically from a
     * ::VAD). When voice_active is true, the ducker ramps toward
     * @c target_attenuation_db; when false, it ramps back toward unity.
     *
     * @param samples Interleaved float samples (modified in place).
     * @param frame_count Number of frames.
     * @param channels Number of interleaved channels.
     * @param voice_active true if voice is currently detected.
     */
    void process(float *samples, std::size_t frame_count, int channels, bool voice_active);

    /**
     * @brief Reset internal state.
     */
    void reset();

    /**
     * @brief Current gain in dB (read-only).
     * @return Current attenuation in dB (0 = unity, negative = ducked).
     */
    float current_gain_db() const noexcept;

  private:
    config_t _cfg;
    float _current_gain_db = 0.0f;
    float _frame_ms = 0.0f;
  };

  /**
   * @brief Composite pre-encoder audio processor.
   *
   * Chains (in order):
   *
   *   capture -> [AGC] -> [VAD observes] -> [Noise gate] -> [Ducker] -> Opus
   *
   * Each stage can be enabled or disabled independently via the config. By
   * default all stages are enabled; pass a config with the relevant @c enable_*
   * flag set to false to bypass a stage.
   *
   * The PreProcessor is allocation-free after construction and lock-free for
   * single-producer use (which is exactly the Sunshine audio-encode thread).
   */
  class PreProcessor {
  public:
    /**
     * @brief PreProcessor configuration.
     */
    struct config_t {
      /// Apply gain control. Default: true.
      bool enable_agc = true;
      /// Run VAD and drive the ducker with its decision. Default: true.
      bool enable_vad = true;
      /// Apply ducking when voice is active. Default: true.
      bool enable_ducking = true;
      /// Apply noise gate (suppress signal below a threshold). Default: true.
      bool enable_noise_gate = true;
      /// Noise-gate threshold (dBFS). Signal below this is attenuated.
      float noise_gate_threshold_db = -55.0f;
      /// AGC tuning (used only when @c enable_agc is true).
      AGC::config_t agc {};
      /// VAD tuning (used only when @c enable_vad is true).
      VAD::config_t vad {};
      /// Ducker tuning (used only when @c enable_ducking is true).
      Ducker::config_t ducker {};
    };

    /**
     * @brief Construct a PreProcessor with default configuration.
     */
    PreProcessor();

    /**
     * @brief Construct a PreProcessor with a combined configuration.
     * @param cfg Combined configuration.
     */
    explicit PreProcessor(const config_t &cfg);

    /**
     * @brief Process a frame of interleaved audio in-place.
     *
     * @param samples Interleaved float samples (modified in place).
     * @param frame_count Number of frames (samples per channel).
     * @param channels Number of interleaved channels.
     * @return true if voice is currently active (only meaningful when
     *         @c enable_vad is true; false otherwise).
     */
    bool process(float *samples, std::size_t frame_count, int channels);

    /**
     * @brief Reset all sub-processor state.
     */
    void reset();

  private:
    config_t _cfg;
    AGC _agc;
    VAD _vad;
    Ducker _ducker;
    float _noise_gate_gain = 1.0f;
  };

}  // namespace audio::fx
