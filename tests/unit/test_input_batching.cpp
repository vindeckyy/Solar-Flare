// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_input_batching.cpp
 * @brief Tests for latency-sensitive input packet batching.
 */

extern "C" {
#include <moonlight-common-c/src/Input.h>
}

#include "../tests_common.h"
#include "src/utility.h"

#include <limits>

namespace input {
  enum class batch_result_e;

  batch_result_e batch(PNV_REL_MOUSE_MOVE_PACKET dest, PNV_REL_MOUSE_MOVE_PACKET src);
  batch_result_e batch(PNV_SCROLL_PACKET dest, PNV_SCROLL_PACKET src);
  batch_result_e batch(PSS_HSCROLL_PACKET dest, PSS_HSCROLL_PACKET src);
}  // namespace input

namespace {
  constexpr int BATCHED = 0;
  constexpr int TERMINATE_BATCH = 2;
}  // namespace

TEST(InputBatching, RelativeMouseAddsPositiveAndNegativeDeltas) {
  NV_REL_MOUSE_MOVE_PACKET destination {};
  NV_REL_MOUSE_MOVE_PACKET source {};
  destination.deltaX = util::endian::big<short>(120);
  destination.deltaY = util::endian::big<short>(-80);
  source.deltaX = util::endian::big<short>(25);
  source.deltaY = util::endian::big<short>(30);

  const auto result = input::batch(&destination, &source);

  EXPECT_EQ(static_cast<int>(result), BATCHED);
  EXPECT_EQ(util::endian::big(destination.deltaX), 145);
  EXPECT_EQ(util::endian::big(destination.deltaY), -50);
}

TEST(InputBatching, RelativeMouseOverflowTerminatesWithoutMutation) {
  NV_REL_MOUSE_MOVE_PACKET destination {};
  NV_REL_MOUSE_MOVE_PACKET source {};
  destination.deltaX = util::endian::big(std::numeric_limits<short>::max());
  destination.deltaY = util::endian::big<short>(4);
  source.deltaX = util::endian::big<short>(1);
  source.deltaY = util::endian::big<short>(5);

  const auto result = input::batch(&destination, &source);

  EXPECT_EQ(static_cast<int>(result), TERMINATE_BATCH);
  EXPECT_EQ(util::endian::big(destination.deltaX), std::numeric_limits<short>::max());
  EXPECT_EQ(util::endian::big(destination.deltaY), 4);
}

TEST(InputBatching, RelativeMouseYOverflowTerminatesWithoutMutation) {
  NV_REL_MOUSE_MOVE_PACKET destination {};
  NV_REL_MOUSE_MOVE_PACKET source {};
  destination.deltaX = util::endian::big<short>(1);
  destination.deltaY = util::endian::big(std::numeric_limits<short>::max());
  source.deltaX = util::endian::big<short>(1);
  source.deltaY = util::endian::big<short>(1);

  const auto result = input::batch(&destination, &source);

  EXPECT_EQ(static_cast<int>(result), TERMINATE_BATCH);
  EXPECT_EQ(util::endian::big(destination.deltaX), 1);
  EXPECT_EQ(util::endian::big(destination.deltaY), std::numeric_limits<short>::max());
}

TEST(InputBatching, VerticalScrollAddsDeltaAndMirrorsProtocolFields) {
  NV_SCROLL_PACKET destination {};
  NV_SCROLL_PACKET source {};
  destination.scrollAmt1 = util::endian::big<short>(-120);
  destination.scrollAmt2 = destination.scrollAmt1;
  source.scrollAmt1 = util::endian::big<short>(30);
  source.scrollAmt2 = source.scrollAmt1;

  const auto result = input::batch(&destination, &source);

  EXPECT_EQ(static_cast<int>(result), BATCHED);
  EXPECT_EQ(util::endian::big(destination.scrollAmt1), -90);
  EXPECT_EQ(util::endian::big(destination.scrollAmt2), -90);
}

TEST(InputBatching, VerticalScrollOverflowTerminatesWithoutMutation) {
  NV_SCROLL_PACKET destination {};
  NV_SCROLL_PACKET source {};
  destination.scrollAmt1 = util::endian::big(std::numeric_limits<short>::max());
  destination.scrollAmt2 = destination.scrollAmt1;
  source.scrollAmt1 = util::endian::big<short>(1);
  source.scrollAmt2 = source.scrollAmt1;

  const auto result = input::batch(&destination, &source);

  EXPECT_EQ(static_cast<int>(result), TERMINATE_BATCH);
  EXPECT_EQ(util::endian::big(destination.scrollAmt1), std::numeric_limits<short>::max());
}

TEST(InputBatching, HorizontalScrollAddsDelta) {
  SS_HSCROLL_PACKET destination {};
  SS_HSCROLL_PACKET source {};
  destination.scrollAmount = util::endian::big<short>(-50);
  source.scrollAmount = util::endian::big<short>(20);

  const auto result = input::batch(&destination, &source);

  EXPECT_EQ(static_cast<int>(result), BATCHED);
  EXPECT_EQ(util::endian::big(destination.scrollAmount), -30);
}

TEST(InputBatching, HorizontalScrollOverflowTerminatesWithoutMutation) {
  SS_HSCROLL_PACKET destination {};
  SS_HSCROLL_PACKET source {};
  destination.scrollAmount = util::endian::big(std::numeric_limits<short>::min());
  source.scrollAmount = util::endian::big<short>(-1);

  const auto result = input::batch(&destination, &source);

  EXPECT_EQ(static_cast<int>(result), TERMINATE_BATCH);
  EXPECT_EQ(util::endian::big(destination.scrollAmount), std::numeric_limits<short>::min());
}
