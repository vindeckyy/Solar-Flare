/**
 * @file src/platform/linux/headless_compositor.h
 * @brief Declarations for the headless compositor used for private game
 *        streaming. Supports labwc (wlroots) for general Wayland,
 *        niri (Smithay) for niri-managed sessions, and
 *        krfb-virtualmonitor for KDE/KWin sessions.
 */
#pragma once

// standard includes
#include <string>

namespace platf::headless {

  /**
   * @brief Backend selection for the headless display.
   */
  enum class backend_e {
    auto_detect,  ///< Detect compositor and pick best backend
    labwc,        ///< Force labwc wlroots headless compositor
    krfb,         ///< Force krfb-virtualmonitor (KDE only)
    gamescope,    ///< Force nested Gamescope headless (Steam Deck / game mode)
    niri,         ///< Force niri (Smithay-based Wayland compositor, attach to existing session)
  };

  /**
   * @brief Detect whether Gamescope is the active compositor.
   * @return true if Gamescope is running.
   */
  bool is_gamescope_running();

  /**
   * @brief Detect whether niri is the active Wayland compositor.
   * @return true if niri is running.
   */
  bool is_niri_running();

  /**
   * @brief Manages a headless display backend.
   *
   * Auto-detects KWin for krfb-virtualmonitor support, falling back to a
   * nested labwc compositor for non-KDE environments.
   */
  class compositor_t {
  public:
    compositor_t() = default;

    /**
     * @brief Set the preferred backend before calling start().
     * @param backend The backend to use.
     */
    void set_backend(backend_e backend);

    /**
     * @brief Start the headless display.
     *
     * @param width Virtual output width in pixels.
     * @param height Virtual output height in pixels.
     * @param refresh_hz Virtual output refresh rate.
     * @param game_cmd The command to launch (labwc only, krfb ignores this).
     * @return true on success.
     */
    bool start(int width, int height, int refresh_hz, const std::string &game_cmd);

    /**
     * @brief Stop the headless display.
     */
    void stop();

    /**
     * @brief Get the compositor PID, or -1 if not running.
     */
    int pid() const;

    /**
     * @brief Get the Wayland socket path (labwc only, empty for krfb).
     */
    const std::string &wayland_socket() const;

    /**
     * @brief Get the discovered X11 display (labwc only, empty for krfb).
     */
    const std::string &x11_display() const;

    /**
     * @brief Get the virtual output name created by this compositor.
     *
     * For krfb this is the monitor name (e.g. "Virtual-1").
     * For labwc this is "HEADLESS-1".
     */
    const std::string &output_name() const;

    /**
     * @brief Wrap a command string with the compositor's environment.
     *
     * Labwc: prepends WAYLAND_DISPLAY= and DISPLAY=.
     * Krfb: returns game_cmd unchanged (runs on host Wayland session).
     */
    std::string wrap_cmd(const std::string &game_cmd);

    /**
     * @brief Determine which backend would be used without starting it.
     *
     * If @ref set_backend has been called with an explicit non-auto value,
     * that value is returned. Otherwise the choice is auto-detected from
     * the running session (KDE -> krfb, gamescope -> nested gamescope,
     * else -> labwc).
     *
     * Public so callers can log / display the selected backend before
     * committing to @ref start(), and so unit tests can exercise the
     * dispatch logic without spawning subprocesses.
     */
    backend_e resolve_backend() const;

  private:
    bool start_labwc(int width, int height, int refresh_hz, const std::string &game_cmd);
    bool start_krfb(int width, int height, int refresh_hz);
    bool start_gamescope(int width, int height, int refresh_hz, const std::string &game_cmd);
    bool start_niri(int width, int height, int refresh_hz);
    void stop_labwc();
    void stop_krfb();
    void stop_gamescope();
    void stop_niri();

    backend_e _backend = backend_e::auto_detect;
    bool _using_krfb = false;
    bool _using_gamescope = false;
    bool _using_niri = false;
    int _pid = -1;
    std::string _wayland_socket;
    std::string _x11_display;
    std::string _output_name;
  };

}  // namespace platf::headless
