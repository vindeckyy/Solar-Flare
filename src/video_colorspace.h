// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/video_colorspace.h
 * @brief Declarations for colorspace functions.
 */
#pragma once

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace video {

  enum class colorspace_e {
    rec601,  ///< Rec. 601
    rec709,  ///< Rec. 709
    bt2020sdr,  ///< Rec. 2020 SDR
    bt2020,  ///< Rec. 2020 HDR
  };

  /**
   * @brief Negotiated color description for a stream.
   */
  struct sunshine_colorspace_t {
    colorspace_e colorspace;  ///< Logical colorspace (Rec.601 / Rec.709 / BT.2020 SDR / BT.2020 HDR).
    bool full_range;  ///< True for JPEG (full) range, false for MPEG (limited).
    unsigned bit_depth;  ///< Output bit depth (8 or 10). Other values are clamped to 8.
  };

  /**
   * @brief Check whether a colorspace is HDR (BT.2020 + ST2084).
   * @param colorspace Colorspace to test.
   * @return True for BT.2020 HDR, false otherwise.
   */
  bool colorspace_is_hdr(const sunshine_colorspace_t &colorspace);

  // Declared in video.h
  struct config_t;

  /**
   * @brief Derive a Sunshine colorspace from a client config and display HDR capability.
   * @param config Client video config (encoderCscMode, dynamicRange).
   * @param hdr_display True when the captured display advertises HDR metadata.
   * @return Validated Sunshine colorspace.
   */
  sunshine_colorspace_t colorspace_from_client_config(const config_t &config, bool hdr_display);

  /**
   * @brief FFmpeg/SWS color description.
   */
  struct avcodec_colorspace_t {
    AVColorPrimaries primaries;  ///< FFmpeg color primaries.
    AVColorTransferCharacteristic transfer_function;  ///< FFmpeg transfer characteristic.
    AVColorSpace matrix;  ///< FFmpeg colorspace matrix.
    AVColorRange range;  ///< FFmpeg color range.
    int software_format;  ///< SWS colorspace identifier (SWS_CS_*).
  };

  /**
   * @brief Convert a Sunshine colorspace to FFmpeg/SWS parameters.
   * @param sunshine_colorspace Validated Sunshine colorspace.
   * @return FFmpeg color description; unknown inputs fall back to Rec.709.
   */
  avcodec_colorspace_t avcodec_colorspace_from_sunshine_colorspace(const sunshine_colorspace_t &sunshine_colorspace);

  struct alignas(16) color_t {
    float color_vec_y[4];
    float color_vec_u[4];
    float color_vec_v[4];
    float range_y[2];
    float range_uv[2];
  };

  /**
   * @brief Get static RGB->YUV color conversion matrix.
   *        This matrix expects RGB input in UNORM (0.0 to 1.0) range and doesn't perform any
   *        gamut mapping or gamma correction.
   * @param colorspace Targeted YUV colorspace.
   * @param unorm_output Whether the matrix should produce output in UNORM or UINT range.
   * @return `const color_t*` that contains RGB->YUV transformation vectors.
   *         Components `range_y` and `range_uv` are there for backwards compatibility
   *         and can be ignored in the computation.
   */
  const color_t *color_vectors_from_colorspace(const sunshine_colorspace_t &colorspace, bool unorm_output);
}  // namespace video
