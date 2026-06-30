/**
 * @file tests/unit/test_per_client_input.cpp
 * @brief Tests for the per_client_input fork module.
 */
#include "../tests_common.h"
#include "src/spice/per_client_input.h"

using namespace per_client_input;

TEST(PerClientInputTest, NoCalibrationIsIdentity) {
  per_client_input::input_calibration_t c;
  auto m = per_client_input::apply_motion(c, 10.0, 5.0);
  EXPECT_DOUBLE_EQ(m.dx, 10.0);
  EXPECT_DOUBLE_EQ(m.dy, 5.0);
}

TEST(PerClientInputTest, DpiRatioScalesMotion) {
  input_calibration_t c;
  c.mouse_dpi_ratio = 2.0;
  auto m = apply_motion(c, 10.0, 5.0);
  EXPECT_DOUBLE_EQ(m.dx, 20.0);
  EXPECT_DOUBLE_EQ(m.dy, 10.0);
}

TEST(PerClientInputTest, WheelDivisorRoundtripsZero) {
  EXPECT_EQ(apply_wheel({}, 0), 0);
}

TEST(PerClientInputTest, WheelDivisorHalves) {
  input_calibration_t c;
  c.scroll_divisor = 2;
  EXPECT_EQ(apply_wheel(c, 10), 5);
  EXPECT_EQ(apply_wheel(c, -10), -5);
}

TEST(PerClientInputTest, InvertAxes) {
  input_calibration_t c;
  c.invert_pointer_x = true;
  c.invert_pointer_y = true;
  auto m = apply_motion(c, 3.0, -4.0);
  EXPECT_DOUBLE_EQ(m.dx, -3.0);
  EXPECT_DOUBLE_EQ(m.dy, 4.0);
}

TEST(PerClientInputTest, SanitizeClampsDpiRatioToValidRange) {
  input_calibration_t c;
  c.mouse_dpi_ratio = 99.0;
  auto c2 = sanitize(c);
  EXPECT_GE(c2.mouse_dpi_ratio, 0.25);
  EXPECT_LE(c2.mouse_dpi_ratio, 4.0);
}

TEST(PerClientInputTest, CalibrationRegistryRoundTrip) {
  set_calibration("uuid-1", {});
  EXPECT_TRUE(has_calibration("uuid-1"));
  auto c = get_calibration("uuid-1");
  EXPECT_DOUBLE_EQ(c.mouse_dpi_ratio, 1.0);
  clear_calibration("uuid-1");
  EXPECT_FALSE(has_calibration("uuid-1"));
}
