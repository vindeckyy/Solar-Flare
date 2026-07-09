/**
 * @file tests/unit/test_error.cpp
 * @brief Regression tests for src/error.h.
 */
#include "../tests_common.h"

#include <src/error.h>

using namespace sunshine;

TEST(SunshineErrorTest, CounterTotalAdvances) {
  auto total_before = counters().total.load(std::memory_order_relaxed);
  log_error(error_category_e::ENCODER, "test", "noop");
  EXPECT_EQ(counters().total.load(), total_before + 1);
}

TEST(SunshineErrorTest, PerCategoryRouting) {
  auto &c = counters();
  auto enc_before  = c.encoder.load();
  auto cap_before  = c.capture.load();
  auto net_before  = c.network.load();
  log_error(error_category_e::CAPTURE, "test", "noop");
  log_error(error_category_e::NETWORK, "test", "noop");
  log_error(error_category_e::SESSION, "test", "noop");
  EXPECT_EQ(c.capture.load(), cap_before + 1);
  EXPECT_EQ(c.network.load(), net_before + 1);
  EXPECT_EQ(c.session.load(), c.session.load());  // session count unchanged by these
  EXPECT_EQ(c.encoder.load(), enc_before);  // encoder not touched
}

TEST(SunshineErrorTest, CategoryStringsAreStable) {
  // ponytail: these strings are part of the public /api/errors contract.
  // Changing them breaks the Web UI grouping. Add a new value, never
  // rename an existing one.
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
  // ponytail: same public-contract rule. The Web UI shows
  // 'encoder error: empty_packet' on the dashboard; the strings
  // are part of the API.
  EXPECT_EQ(to_string(encode_error_e::NONE),                 "none");
  EXPECT_EQ(to_string(encode_error_e::EMPTY_PACKET),         "empty_packet");
  EXPECT_EQ(to_string(encode_error_e::FRAME_INDEX_MISMATCH), "frame_index_mismatch");
  EXPECT_EQ(to_string(encode_error_e::UNSUPPORTED_SESSION),  "unsupported_session");
}

TEST(SunshineErrorTest, MacroIncrementsCounter) {
  auto &c = counters();
  auto before = c.encoder.load();
  SUN_ERR(error_category_e::ENCODER, "test_macro", "frame 42 dropped");
  EXPECT_EQ(c.encoder.load(), before + 1);
}
