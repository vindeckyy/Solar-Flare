// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/platform/linux/x11grab.h
 * @brief Declarations for x11 capture.
 */
#pragma once

// standard includes
#include <optional>

// local includes
#include "src/platform/common.h"
#include "src/utility.h"

// X11 Display
extern "C" struct _XDisplay;

namespace egl {
  class cursor_t;
}

namespace platf {
  /**
   * @brief Check whether an XRandR output can be selected for capture.
   *
   * @param connected Whether XRandR reports the output as connected.
   * @param has_crtc Whether the output has an active CRTC.
   * @return true when the output can produce a capture region.
   */
  bool x11_output_is_active(bool connected, bool has_crtc);
}  // namespace platf

namespace platf::x11 {
  struct cursor_ctx_raw_t;
  void freeCursorCtx(cursor_ctx_raw_t *ctx);
  void freeDisplay(_XDisplay *xdisplay);

  using cursor_ctx_t = util::safe_ptr<cursor_ctx_raw_t, freeCursorCtx>;
  using xdisplay_t = util::safe_ptr<_XDisplay, freeDisplay>;

  class cursor_t {
  public:
    static std::optional<cursor_t> make();

    void capture(egl::cursor_t &img);

    /**
     * Capture and blend the cursor into the image
     *
     * img <-- destination image
     * offsetX, offsetY <--- Top left corner of the virtual screen
     */
    void blend(img_t &img, int offsetX, int offsetY);

    cursor_ctx_t ctx;
  };

  xdisplay_t make_display();
}  // namespace platf::x11
