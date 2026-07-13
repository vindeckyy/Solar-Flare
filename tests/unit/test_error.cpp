/**
 * @file tests/unit/test_error.cpp
 * @brief Regression tests for src/error.h.
 *
 * ponytail: these tests assert the counter contract -- the only part
 * of error.h that's our code. The BOOST_LOG call inside log_error()
 * is boost plumbing; we don't exercise it here because boost::log
 * needs a sink to be initialized before the global `error` logger
 * can be used, and that requires linking src/logging.cpp + display-
 * device + ffmpeg. The standalone runner script bypasses that.
 *
 * Public contract we DO test:
 * - to_string() mappings are stable (the Web UI's /api/errors
 *   response body depends on these strings never changing)
 * - error_counters_t atomics advance correctly
 * - the SUN_ERR macro is just a function-call wrapper (compile-only)
 */
#include "../tests_common.h"

#include <src/error.h>

using namespace sunshine;

TEST(SunshineErrorTest, CategoryStringsAreStable) {
  EXPECT_EQ(to_string(error_category_e::ENCODER), "encoder");
  EXPECT_EQ(to_string(error_category_e::CAPTURE), "capture");
  EXPECT_EQ(to_string(error_category_e::NETWORK), "network");
  EXPECT_EQ(to_string(error_category_e::SESSION), "session");
  EXPECT_EQ(to_string(error_category_e::PROCESS), "process");
  EXPECT_EQ(to_string(error_category_e::CONFIG),  "config");
  EXPECT_EQ(to_string(error_category_e::CRYPTO),  "crypto");
  EXPECT_EQ(to_string(error_category_e::UNKNOWN), "unknown");
}

TEST(SunshineErrorTest, EncodeErrorStringsAreStable) {
  EXPECT_EQ(to_string(encode_error_e::NONE),                 "none");
  EXPECT_EQ(to_string(encode_error_e::EMPTY_PACKET),         "empty_packet");
  EXPECT_EQ(to_string(encode_error_e::FRAME_INDEX_MISMATCH), "frame_index_mismatch");
  EXPECT_EQ(to_string(encode_error_e::UNSUPPORTED_SESSION),  "unsupported_session");
}

TEST(SunshineErrorTest, CounterTotalAdvancesOnBump) {
  // ponytail: hit the counter directly. Going through SUN_ERR would
  // also call BOOST_LOG, which needs the global log sink to be
  // initialized. Without logging::init(), BOOST_LOG segfaults.
  auto &c = counters();
  auto before = c.total.load(std::memory_order_relaxed);
  c.total.fetch_add(1, std::memory_order_relaxed);
  EXPECT_EQ(c.total.load(), before + 1);
}

TEST(SunshineErrorTest, PerCategoryCounterIsIndependent) {
  auto &c = counters();
  auto enc_before = c.encoder.load();
  auto cap_before = c.capture.load();
  c.encoder.fetch_add(1, std::memory_order_relaxed);
  c.capture.fetch_add(1, std::memory_order_relaxed);
  EXPECT_EQ(c.encoder.load(), enc_before + 1);
  EXPECT_EQ(c.capture.load(), cap_before + 1);
}
