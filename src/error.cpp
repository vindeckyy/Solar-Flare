/**
 * @file src/error.cpp
 * @brief Implementation of the singleton error_counters_t.
 */
#include "error.h"

namespace sunshine {
  error_counters_t &counters() {
    static error_counters_t instance;
    return instance;
  }
}  // namespace sunshine
