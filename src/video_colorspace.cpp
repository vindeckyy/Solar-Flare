// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/video_colorspace.cpp
 * @brief Definitions for colorspace functions.
 */
// this include
#include "video_colorspace.h"

// local includes
#include "logging.h"
#include "video.h"

extern "C" {
#include <libswscale/swscale.h>
}

namespace video {

  /**
   * @brief Check whether a colorspace is HDR.
   * @param colorspace Colorspace descriptor to test.
   * @return True when the colorspace is BT.2020 with PQ (HDR), false otherwise.
   */
  bool colorspace_is_hdr(const sunshine_colorspace_t &colorspace) {
    return colorspace.colorspace == colorspace_e::bt2020;
  }

  /**
   * @brief Derive a negotiated sunshine colorspace from client config and display capability.
   *
   * Validates the client-supplied encoderCscMode and dynamicRange, clamps
   * out-of-range values with a warning, and forces BT.2020+PQ when HDR is
   * requested and the display advertises HDR support. An invalid BT.2020 SDR
   * + 8-bit combination is downgraded to Rec.709 so the encoder does not
   * receive an unsupported color combination.
   *
   * @param config Client video config (encoderCscMode, dynamicRange).
   * @param hdr_display True when the captured display reports HDR metadata.
   * @return Validated sunshine_colorspace_t ready for encoder setup.
   */
  sunshine_colorspace_t colorspace_from_client_config(const config_t &config, bool hdr_display) {
    sunshine_colorspace_t colorspace;

    /* See video::config_t declaration for details */

    if (config.dynamicRange > 0 && hdr_display) {
      // Rec. 2020 with ST 2084 perceptual quantizer
      colorspace.colorspace = colorspace_e::bt2020;
    } else {
      switch (config.encoderCscMode >> 1) {
        case 0:
          // Rec. 601
          colorspace.colorspace = colorspace_e::rec601;
          break;

        case 1:
          // Rec. 709
          colorspace.colorspace = colorspace_e::rec709;
          break;

        case 2:
          // Rec. 2020
          colorspace.colorspace = colorspace_e::bt2020sdr;
          break;

        default:
          BOOST_LOG(error) << "Unknown video colorspace in csc, falling back to Rec. 709";
          colorspace.colorspace = colorspace_e::rec709;
          break;
      }
    }

    colorspace.full_range = (config.encoderCscMode & 0x1);

    switch (config.dynamicRange) {
      case 0:
        colorspace.bit_depth = 8;
        break;

      case 1:
        colorspace.bit_depth = 10;
        break;

      default:
        BOOST_LOG(error) << "Unknown dynamicRange value, falling back to 10-bit color depth";
        colorspace.bit_depth = 10;
        break;
    }

    if (colorspace.colorspace == colorspace_e::bt2020sdr && colorspace.bit_depth != 10) {
      BOOST_LOG(error) << "BT.2020 SDR colorspace expects 10-bit color depth, falling back to Rec. 709";
      colorspace.colorspace = colorspace_e::rec709;
    }

    return colorspace;
  }

  /**
   * @brief Convert a Sunshine colorspace to FFmpeg/SWS color parameters.
   *
   * Handles all known colorspace variants and clamps invalid bit depths or
   * unknown enum values to a safe Rec.709 fallback so the caller never
   * receives uninitialized AVColor* fields. BT.2020 variants require 10-bit
   * depth and emit a warning when the caller violates that expectation.
   *
   * @param sunshine_colorspace Validated Sunshine colorspace descriptor.
   * @return FFmpeg color description suitable for AVCodecContext and sws.
   */
  avcodec_colorspace_t avcodec_colorspace_from_sunshine_colorspace(const sunshine_colorspace_t &sunshine_colorspace) {
    avcodec_colorspace_t avcodec_colorspace {};

    // Defensive: treat an out-of-range bit depth as 8-bit and warn, so
    // subsequent asserts do not fire in release builds.
    unsigned bit_depth = sunshine_colorspace.bit_depth;
    if (bit_depth != 8 && bit_depth != 10) {
      BOOST_LOG(warning) << "Invalid colorspace bit depth " << bit_depth << ", clamping to 8-bit";
      bit_depth = 8;
    }

    switch (sunshine_colorspace.colorspace) {
      case colorspace_e::rec601:
        // Rec. 601
        avcodec_colorspace.primaries = AVCOL_PRI_SMPTE170M;
        avcodec_colorspace.transfer_function = AVCOL_TRC_SMPTE170M;
        avcodec_colorspace.matrix = AVCOL_SPC_SMPTE170M;
        avcodec_colorspace.software_format = SWS_CS_SMPTE170M;
        break;

      case colorspace_e::rec709:
        // Rec. 709
        avcodec_colorspace.primaries = AVCOL_PRI_BT709;
        avcodec_colorspace.transfer_function = AVCOL_TRC_BT709;
        avcodec_colorspace.matrix = AVCOL_SPC_BT709;
        avcodec_colorspace.software_format = SWS_CS_ITU709;
        break;

      case colorspace_e::bt2020sdr:
        // Rec. 2020 (SDR, gamma 2.4)
        avcodec_colorspace.primaries = AVCOL_PRI_BT2020;
        if (bit_depth != 10) {
          BOOST_LOG(warning) << "BT.2020 SDR expects 10-bit depth, got " << bit_depth << " bit; using 10-bit transfer anyway";
        }
        avcodec_colorspace.transfer_function = AVCOL_TRC_BT2020_10;
        avcodec_colorspace.matrix = AVCOL_SPC_BT2020_NCL;
        avcodec_colorspace.software_format = SWS_CS_BT2020;
        break;

      case colorspace_e::bt2020:
        // Rec. 2020 with ST 2084 perceptual quantizer (HDR)
        avcodec_colorspace.primaries = AVCOL_PRI_BT2020;
        if (bit_depth != 10) {
          BOOST_LOG(warning) << "BT.2020 HDR expects 10-bit depth, got " << bit_depth << " bit; using PQ transfer anyway";
        }
        avcodec_colorspace.transfer_function = AVCOL_TRC_SMPTE2084;
        avcodec_colorspace.matrix = AVCOL_SPC_BT2020_NCL;
        avcodec_colorspace.software_format = SWS_CS_BT2020;
        break;

      default:
        BOOST_LOG(warning) << "Unknown colorspace enum " << static_cast<int>(sunshine_colorspace.colorspace)
                           << ", falling back to Rec. 709";
        avcodec_colorspace.primaries = AVCOL_PRI_BT709;
        avcodec_colorspace.transfer_function = AVCOL_TRC_BT709;
        avcodec_colorspace.matrix = AVCOL_SPC_BT709;
        avcodec_colorspace.software_format = SWS_CS_ITU709;
        break;
    }

    avcodec_colorspace.range = sunshine_colorspace.full_range ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

    return avcodec_colorspace;
  }

  /**
   * @brief Get static RGB->YUV color conversion matrix for a colorspace.
   *
   * Returns a precomputed color_t for the requested Sunshine colorspace and
   * output range. Unknown colorspace enums are treated as Rec.709 with a
   * warning, and invalid bit depths (not 8 or 10) are clamped to 8-bit so
   * the caller never indexes out of bounds or produces nonsensical matrices.
   *
   * @param colorspace Targeted YUV colorspace.
   * @param unorm_output True for UNORM (0.0-1.0) output, false for UINT.
   * @return Pointer to a static color_t with conversion vectors.
   */
  const color_t *color_vectors_from_colorspace(const sunshine_colorspace_t &colorspace, bool unorm_output) {
    constexpr auto generate_color_vectors = [](const sunshine_colorspace_t &colorspace, bool unorm_output) -> color_t {
      // "Table 4 – Interpretation of matrix coefficients (MatrixCoefficients) value" section of ITU-T H.273
      double Kr;
      double Kb;
      switch (colorspace.colorspace) {
        case colorspace_e::rec601:
          Kr = 0.299;
          Kb = 0.114;
          break;
        case colorspace_e::rec709:
        default:
          Kr = 0.2126;
          Kb = 0.0722;
          break;
        case colorspace_e::bt2020:
        case colorspace_e::bt2020sdr:
          Kr = 0.2627;
          Kb = 0.0593;
          break;
      }
      double Kg = 1.0 - Kr - Kb;

      double y_mult;
      double y_add;
      double uv_mult;
      double uv_add;

      // "8.3 Matrix coefficients" section of ITU-T H.273
      if (colorspace.full_range) {
        y_mult = (1 << colorspace.bit_depth) - 1;
        y_add = 0;
        uv_mult = (1 << colorspace.bit_depth) - 1;
        uv_add = (1 << (colorspace.bit_depth - 1));
      } else {
        y_mult = (1 << (colorspace.bit_depth - 8)) * 219;
        y_add = (1 << (colorspace.bit_depth - 8)) * 16;
        uv_mult = (1 << (colorspace.bit_depth - 8)) * 224;
        uv_add = (1 << (colorspace.bit_depth - 8)) * 128;
      }

      if (unorm_output) {
        const double unorm_range = (1 << colorspace.bit_depth) - 1;
        y_mult /= unorm_range;
        y_add /= unorm_range;
        uv_mult /= unorm_range;
        uv_add /= unorm_range;
      } else {
        // For rounding
        y_add += 0.5f;
        uv_add += 0.5f;
      }

      color_t color_vectors;

      color_vectors.color_vec_y[0] = Kr * y_mult;
      color_vectors.color_vec_y[1] = Kg * y_mult;
      color_vectors.color_vec_y[2] = Kb * y_mult;
      color_vectors.color_vec_y[3] = y_add;

      color_vectors.color_vec_u[0] = -0.5 * Kr / (1.0 - Kb) * uv_mult;
      color_vectors.color_vec_u[1] = -0.5 * Kg / (1.0 - Kb) * uv_mult;
      color_vectors.color_vec_u[2] = 0.5 * uv_mult;
      color_vectors.color_vec_u[3] = uv_add;

      color_vectors.color_vec_v[0] = 0.5 * uv_mult;
      color_vectors.color_vec_v[1] = -0.5 * Kg / (1.0 - Kr) * uv_mult;
      color_vectors.color_vec_v[2] = -0.5 * Kb / (1.0 - Kr) * uv_mult;
      color_vectors.color_vec_v[3] = uv_add;

      // Unused
      color_vectors.range_y[0] = 1;
      color_vectors.range_y[1] = 0;
      color_vectors.range_uv[0] = 1;
      color_vectors.range_uv[1] = 0;

      return color_vectors;
    };

    static constexpr color_t colors[] = {
      generate_color_vectors({colorspace_e::rec601, false, 8}, false),
      generate_color_vectors({colorspace_e::rec601, true, 8}, false),
      generate_color_vectors({colorspace_e::rec601, false, 10}, false),
      generate_color_vectors({colorspace_e::rec601, true, 10}, false),
      generate_color_vectors({colorspace_e::rec709, false, 8}, false),
      generate_color_vectors({colorspace_e::rec709, true, 8}, false),
      generate_color_vectors({colorspace_e::rec709, false, 10}, false),
      generate_color_vectors({colorspace_e::rec709, true, 10}, false),
      generate_color_vectors({colorspace_e::bt2020, false, 8}, false),
      generate_color_vectors({colorspace_e::bt2020, true, 8}, false),
      generate_color_vectors({colorspace_e::bt2020, false, 10}, false),
      generate_color_vectors({colorspace_e::bt2020, true, 10}, false),

      generate_color_vectors({colorspace_e::rec601, false, 8}, true),
      generate_color_vectors({colorspace_e::rec601, true, 8}, true),
      generate_color_vectors({colorspace_e::rec601, false, 10}, true),
      generate_color_vectors({colorspace_e::rec601, true, 10}, true),
      generate_color_vectors({colorspace_e::rec709, false, 8}, true),
      generate_color_vectors({colorspace_e::rec709, true, 8}, true),
      generate_color_vectors({colorspace_e::rec709, false, 10}, true),
      generate_color_vectors({colorspace_e::rec709, true, 10}, true),
      generate_color_vectors({colorspace_e::bt2020, false, 8}, true),
      generate_color_vectors({colorspace_e::bt2020, true, 8}, true),
      generate_color_vectors({colorspace_e::bt2020, false, 10}, true),
      generate_color_vectors({colorspace_e::bt2020, true, 10}, true),
    };

    // Clamp invalid bit depths before indexing the precomputed table.
    unsigned effective_bit_depth = colorspace.bit_depth;
    if (effective_bit_depth != 8 && effective_bit_depth != 10) {
      BOOST_LOG(warning) << "Invalid bit depth " << effective_bit_depth << " for color_vectors, clamping to 8";
      effective_bit_depth = 8;
    }

    const color_t *result = nullptr;

    switch (colorspace.colorspace) {
      case colorspace_e::rec601:
        result = &colors[0];
        break;
      case colorspace_e::rec709:
        result = &colors[4];
        break;
      case colorspace_e::bt2020:
      case colorspace_e::bt2020sdr:
        result = &colors[8];
        break;
      default:
        BOOST_LOG(warning) << "Unknown colorspace enum " << static_cast<int>(colorspace.colorspace)
                           << " in color_vectors_from_colorspace, falling back to Rec.709";
        result = &colors[4];
        break;
    }

    if (effective_bit_depth == 10) {
      result += 2;
    }
    if (colorspace.full_range) {
      result += 1;
    }
    if (unorm_output) {
      result += 12;
    }

    return result;
  }
}  // namespace video
