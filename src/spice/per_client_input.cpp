/**
 * @file src/spice/per_client_input.cpp
 */
#include "per_client_input.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <sstream>

namespace per_client_input {

  namespace {
    std::mutex g_mu;
    std::unordered_map<std::string, input_calibration_t> g_map;

    input_calibration_t defaults() {
      return input_calibration_t {};
    }
  }  // namespace

  input_calibration_t sanitize(input_calibration_t c) {
    c.mouse_dpi_ratio = std::clamp(c.mouse_dpi_ratio, 0.25, 4.0);
    c.scroll_divisor = std::clamp(c.scroll_divisor, 1, 16);
    c.deadzone_px = std::clamp(c.deadzone_px, 0, 16);
    return c;
  }

  adjusted_motion_t apply_motion(const input_calibration_t &c, double dx, double dy) {
    adjusted_motion_t out;
    auto cal = sanitize(c);
    if (std::abs(dx) < cal.deadzone_px) dx = 0;
    if (std::abs(dy) < cal.deadzone_px) dy = 0;
    dx *= cal.mouse_dpi_ratio;
    dy *= cal.mouse_dpi_ratio;
    if (cal.invert_pointer_x) dx = -dx;
    if (cal.invert_pointer_y) dy = -dy;
    if (cal.swap_buttons) std::swap(dx, dy);
    out.dx = dx;
    out.dy = dy;
    return out;
  }

  int apply_wheel(const input_calibration_t &c, int wheel_delta) {
    auto cal = sanitize(c);
    if (cal.scroll_divisor <= 1) return wheel_delta;
    if (wheel_delta == 0) return 0;
    int q = wheel_delta / cal.scroll_divisor;
    int r = wheel_delta % cal.scroll_divisor;
    return q + (r != 0 ? (q >= 0 ? 1 : -1) : 0);  // round toward zero
  }

  void set_calibration(const std::string &uuid, const input_calibration_t &c) {
    if (uuid.empty()) return;
    std::lock_guard<std::mutex> lk(g_mu);
    g_map[uuid] = sanitize(c);
  }

  void clear_calibration(const std::string &uuid) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_map.erase(uuid);
  }

  input_calibration_t get_calibration(const std::string &uuid) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_map.find(uuid);
    if (it == g_map.end()) return defaults();
    return it->second;
  }

  bool has_calibration(const std::string &uuid) {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_map.find(uuid) != g_map.end();
  }

  std::unordered_map<std::string, input_calibration_t> all_calibrations() {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_map;
  }

  std::string describe(const input_calibration_t &c) {
    auto v = sanitize(c);
    std::ostringstream os;
    os << "DPI x" << v.mouse_dpi_ratio
       << ", scroll /" << v.scroll_divisor
       << ", deadzone " << v.deadzone_px << "px";
    if (v.swap_buttons) os << ", buttons swapped";
    if (v.invert_pointer_x) os << ", invertX";
    if (v.invert_pointer_y) os << ", invertY";
    return os.str();
  }

}  // namespace per_client_input
