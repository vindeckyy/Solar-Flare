// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_x11grab.cpp
 * @brief Tests X11 output selection.
 */
#include "../tests_common.h"

#ifdef SUNSHINE_BUILD_X11
  #include <src/platform/linux/x11grab.h>

TEST(X11OutputSelectionTest, RequiresConnectedOutputWithAnActiveCrtc) {
  EXPECT_TRUE(platf::x11_output_is_active(true, true));
  EXPECT_FALSE(platf::x11_output_is_active(false, true));
  EXPECT_FALSE(platf::x11_output_is_active(true, false));
}
#endif
