/**
 * @file src/nvenc/nvenc_config.h
 * @brief Declarations for NVENC encoder configuration.
 */
#pragma once

namespace nvenc {

  enum class nvenc_two_pass {
    disabled,  ///< Single pass, the fastest and no extra vram
    quarter_resolution,  ///< Larger motion vectors being caught, faster and uses less extra vram
    full_resolution,  ///< Better overall statistics, slower and uses more extra vram
  };

  enum class nvenc_split_frame_encoding {
    disabled,  ///< Disable
    driver_decides,  ///< Let driver decide
    force_enabled,  ///< Force-enable
  };

  /**
   * @brief NVENC encoder configuration.
   */
  struct nvenc_config {
    // Quality preset from 1 to 7, higher is slower
    int quality_preset = 1;

    // Use optional preliminary pass for better motion vectors, bitrate distribution and stricter VBV(HRD), uses CUDA cores
    nvenc_two_pass two_pass = nvenc_two_pass::quarter_resolution;

    // Percentage increase of VBV/HRD from the default single frame, allows low-latency variable bitrate
    int vbv_percentage_increase = 0;

    // Improves fades compression, uses CUDA cores
    bool weighted_prediction = false;

    // Allocate more bitrate to flat regions since they're visually more perceptible, uses CUDA cores
    bool adaptive_quantization = false;

    // Don't use QP below certain value, limits peak image quality to save bitrate
    bool enable_min_qp = false;

    // Min QP value for H.264 when enable_min_qp is selected
    unsigned min_qp_h264 = 19;

    // Min QP value for HEVC when enable_min_qp is selected
    unsigned min_qp_hevc = 23;

    // Min QP value for AV1 when enable_min_qp is selected
    unsigned min_qp_av1 = 23;

    // Use CAVLC entropy coding in H.264 instead of CABAC, not relevant and here for historical reasons
    bool h264_cavlc = false;

    // Add filler data to encoded frames to stay at target bitrate, mainly for testing
    bool insert_filler_data = false;

    // Enable split-frame encoding if the gpu has multiple NVENC hardware clusters
    nvenc_split_frame_encoding split_frame_encoding = nvenc_split_frame_encoding::driver_decides;

    // Rate-control lookahead, in frames (0 = disabled, 1-250)
    // Helps rate control anticipate motion; +15-30% bitrate variance reduction
    // at the cost of ~lookahead frames of pipeline latency. Maps to
    // NV_ENC_RC_PARAMS::enableLookahead + lookAheadDepth.
    int rc_lookahead = 0;

    // Number of encode surfaces (1-32, -1 = driver default)
    // More surfaces = better encoder pipelining at the cost of GPU memory.
    // Maps to NV_ENC_INITIALIZE_PARAMS::numEncodeSurfaces.
    int surfaces = -1;

    // B-frames between P-frames (0-4)
    // Higher = better compression at the cost of pipeline latency.
    // 0 = zero-reorder-delay (required for sub-frame streaming latency).
    int bframes = 0;

    // tune=zerolatency: auto-disable lookahead, B-frames, multi-pass.
    // Designed for interactive streaming where every ms of pipeline
    // latency shows up in the input-to-photon loop.
    bool zerolatency = false;

    // AQ strength when adaptive quantization is enabled (1-15).
    // 1 = subtle, 15 = aggressive bit redistribution. Default 8.
    int aq_strength = 8;

    // Temporal AQ: redistribute bits across frames instead of within a
    // frame. Pairs with spatial_aq for full 2D AQ.
    bool temporal_aq = false;
  };

}  // namespace nvenc
