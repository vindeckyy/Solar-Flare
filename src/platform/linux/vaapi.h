// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/platform/linux/vaapi.h
 * @brief Declarations for VA-API hardware accelerated capture.
 */
#pragma once

// standard includes
#include <cstdint>

// local includes
#include "misc.h"
#include "src/platform/common.h"

namespace egl {
  struct surface_descriptor_t;
}

namespace va {
  /**
   * @brief Whether VA-API encode should use a single-frame VBV buffer size.
   *
   * True when the user enables `strict_rc_buffer`, or for the historical
   * Intel GPU / AV1 automatic cases. Applies for both auto (`rc_mode == 0`)
   * and explicit rate-control modes; only automatic mode *selection*
   * remains gated on auto so an explicit user choice is not overwritten.
   *
   * @param strict_rc_buffer User preference from `config::video.vaapi.strict_rc_buffer`.
   * @param is_intel True when the VA vendor string identifies an Intel GPU.
   * @param is_av1 True when encoding AV1.
   * @return True if single-frame VBV sizing should be applied.
   */
  bool want_single_frame_vbv(bool strict_rc_buffer, bool is_intel, bool is_av1);

  /**
   * @brief Compute a VBV buffer size in bits for a number of frames of bitrate.
   *
   * Matches the historical VA-API sizing:
   * `bit_rate * framerate_den * frames / framerate_num`.
   *
   * @param bit_rate Target bitrate in bits per second.
   * @param framerate_num Framerate numerator (must be > 0 for a meaningful result).
   * @param framerate_den Framerate denominator.
   * @param frames Buffer length in frames (`1` for single-frame VBV).
   * @return Buffer size in bits.
   */
  std::int64_t rc_buffer_size_bits(std::int64_t bit_rate, int framerate_num, int framerate_den, int frames);

  /**
   * Width --> Width of the image
   * Height --> Height of the image
   * offset_x --> Horizontal offset of the image in the texture
   * offset_y --> Vertical offset of the image in the texture
   * file_t card --> The file descriptor of the render device used for encoding
   */
  std::unique_ptr<platf::avcodec_encode_device_t> make_avcodec_encode_device(int width, int height, bool vram);
  std::unique_ptr<platf::avcodec_encode_device_t> make_avcodec_encode_device(int width, int height, int offset_x, int offset_y, bool vram);
  std::unique_ptr<platf::avcodec_encode_device_t> make_avcodec_encode_device(int width, int height, file_t &&card, int offset_x, int offset_y, bool vram);

  // Ensure the render device pointed to by fd is capable of encoding h264 with the hevc_mode configured
  bool validate(int fd);
}  // namespace va
