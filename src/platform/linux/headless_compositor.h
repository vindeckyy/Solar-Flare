/**
 * @file src/platform/linux/headless_compositor.h
 * @brief Declarations for the headless Wayland compositor used for
 *        private game streaming.
 */
#pragma once

// standard includes
#include <string>

namespace platf::headless {

  /**
   * @brief Manages a headless nested Wayland compositor (labwc).
   *
   * Spawns labwc as a child process with WLR_BACKENDS=headless so that
   * games run inside a private compositor session rather than the user's
   * desktop. The class discovers the Wayland socket, polls for the
   * headless output, and detects XWayland on launch.
   */
  class compositor_t {
  public:
    compositor_t() = default;

    /**
     * @brief Start the headless compositor.
     *
     * Finds labwc and wlr-randr in PATH, forks a child to run labwc with
     * the headless backend, discovers the new WAYLAND_DISPLAY socket, and
     * polls for the HEADLESS-1 output.
     *
     * @param width Virtual output width in pixels.
     * @param height Virtual output height in pixels.
     * @param refresh_hz Virtual output refresh rate.
     * @param game_cmd The command to launch after compositor is ready,
     *                 with WAYLAND_DISPLAY and DISPLAY already set.
     * @return true on success.
     */
    bool start(int width, int height, int refresh_hz, const std::string &game_cmd);

    /**
     * @brief Stop the compositor by terminating the process group.
     *
     * Sends SIGTERM, waits up to 3 seconds, then sends SIGKILL.
     */
    void stop();

    /**
     * @brief Get the compositor PID.
     * @return The PID of the labwc process, or -1 if not running.
     */
    int pid() const;

    /**
     * @brief Get the discovered WAYLAND_DISPLAY path.
     * @return The absolute Wayland socket path (e.g. "wayland-1").
     */
    const std::string &wayland_socket() const;

    /**
     * @brief Get the discovered X11 display number.
     * @return The X11 display number string (e.g. ":1"), empty if none.
     */
    const std::string &x11_display() const;

    /**
     * @brief Wrap a command string with the compositor's environment.
     *
     * Prepends WAYLAND_DISPLAY= and DISPLAY= to @p game_cmd so the
     * launched game uses the private compositor session.
     *
     * @param game_cmd The raw command to wrap.
     * @return The wrapped command string.
     */
    std::string wrap_cmd(const std::string &game_cmd);

  private:
    int _pid = -1;
    std::string _wayland_socket;
    std::string _x11_display;
  };

}  // namespace platf::headless
