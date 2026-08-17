// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/stat_trackers.cpp
 * @brief Definitions for streaming statistic tracking.
 */
// local includes
#include "stat_trackers.h"

namespace stat_trackers {

  /**
   * @brief Format helper for one digit after decimal.
   * @return Boost format "%1$.1f".
   */
  boost::format one_digit_after_decimal() {
    return boost::format("%1$.1f");
  }

  /**
   * @brief Format helper for two digits after decimal.
   * @return Boost format "%1$.2f".
   */
  boost::format two_digits_after_decimal() {
    return boost::format("%1$.2f");
  }

}  // namespace stat_trackers
