/**
 * @file src/platform/linux/headless_compositor.cpp
 * @brief Headless Wayland compositor implementation for private game
 *        streaming via labwc.
 */

// standard includes
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

// platform includes
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// lib includes
#include <boost/algorithm/string/predicate.hpp>
#include <boost/process/v1.hpp>
#include <boost/process/v1/search_path.hpp>

// local includes
#include "headless_compositor.h"
#include "src/logging.h"

using namespace std::literals;
namespace bp = boost::process::v1;

namespace platf::headless {

  /**
   * @brief Discover the current user's runtime directory.
   * @return The path (e.g. "/run/user/1000") or empty on failure.
   */
  static std::string user_runtime_dir() {
    if (auto *xdg = std::getenv("XDG_RUNTIME_DIR")) {
      return {xdg};
    }
    auto *pw = getpwuid(getuid());
    if (pw && pw->pw_uid > 0) {
      return "/run/user/"s + std::to_string(pw->pw_uid);
    }
    return {};
  }

  /**
   * @brief List Wayland socket names inside the run directory,
   *        optionally filtering by a prefix.
   * @param run_dir The runtime directory (e.g. "/run/user/1000").
   * @param prefix Only return names beginning with this prefix (e.g. "wayland-").
   * @return A vector of basenames found.
   */
  static std::vector<std::string> list_wayland_sockets(const std::string &run_dir, const std::string &prefix) {
    std::vector<std::string> result;
    auto *dir = opendir(run_dir.c_str());
    if (!dir) return result;
    while (auto *ent = readdir(dir)) {
      std::string name {ent->d_name};
      if (boost::algorithm::starts_with(name, prefix)) {
        result.push_back(name);
      }
    }
    closedir(dir);
    return result;
  }

  bool is_kwin_running() {
    auto *xdg = std::getenv("XDG_CURRENT_DESKTOP");
    if (xdg) {
      std::string_view desktop {xdg};
      return desktop.find("KDE"sv) != std::string_view::npos;
    }
    auto *wayland = std::getenv("WAYLAND_DISPLAY");
    if (!wayland) return false;
    return bp::search_path("kwin_wayland").empty() ? false : true;
  }

  bool is_gamescope_running() {
    auto *xdg = std::getenv("XDG_CURRENT_DESKTOP");
    if (xdg && std::string_view {xdg} == "gamescope"sv) return true;
    auto *display = std::getenv("WAYLAND_DISPLAY");
    if (display && std::string_view {display}.find("gamescope"sv) != std::string_view::npos) return true;
    return !bp::search_path("gamescope").empty();
  }

  backend_e compositor_t::resolve_backend() const {
    if (_backend == backend_e::labwc) return backend_e::labwc;
    if (_backend == backend_e::krfb) return backend_e::krfb;
    if (_backend == backend_e::gamescope) return backend_e::gamescope;

    if (is_kwin_running() && !bp::search_path("krfb-virtualmonitor").empty()) {
      BOOST_LOG(info) << "headless_compositor: detected KWin, using krfb-virtualmonitor backend"sv;
      return backend_e::krfb;
    }
    if (is_gamescope_running()) {
      BOOST_LOG(info) << "headless_compositor: detected Gamescope, using nested gamescope backend"sv;
      return backend_e::gamescope;
    }
    BOOST_LOG(info) << "headless_compositor: using labwc backend"sv;
    return backend_e::labwc;
  }

  void compositor_t::set_backend(backend_e backend) {
    _backend = backend;
  }

  bool compositor_t::start(int width, int height, int refresh_hz, const std::string &game_cmd) {
    auto backend = resolve_backend();
    if (backend == backend_e::krfb) {
      _using_krfb = true;
      return start_krfb(width, height, refresh_hz);
    }
    if (backend == backend_e::gamescope) {
      _using_gamescope = true;
      return start_gamescope(width, height, refresh_hz, game_cmd);
    }
    return start_labwc(width, height, refresh_hz, game_cmd);
  }

  bool compositor_t::start_krfb(int width, int height, int refresh_hz) {
    auto krfb_path = bp::search_path("krfb-virtualmonitor");
    if (krfb_path.empty()) {
      BOOST_LOG(error) << "headless_compositor: krfb-virtualmonitor not found in PATH"sv;
      return false;
    }

    _output_name = "SolarFlare-Headless";

    std::error_code ec;
    std::string krfb_out;
    int krfb_exit = 0;
    {
      bp::ipstream pipe_stream;
      bp::child proc(krfb_path,
        "--name", _output_name,
        "--width", std::to_string(width),
        "--height", std::to_string(height),
        "--refresh", std::to_string(refresh_hz),
        bp::std_out > pipe_stream, bp::std_err > pipe_stream, ec);
      if (ec) {
        BOOST_LOG(error) << "headless_compositor: krfb-virtualmonitor failed to start: "sv << ec.message();
        return false;
      }
      proc.wait(ec);
      krfb_exit = proc.exit_code();
      if (ec) {
        BOOST_LOG(error) << "headless_compositor: krfb-virtualmonitor exited with error"sv;
        return false;
      }
      std::ostringstream oss;
      oss << pipe_stream.rdbuf();
      krfb_out = oss.str();
    }

    if (krfb_exit != 0) {
      BOOST_LOG(error) << "headless_compositor: krfb-virtualmonitor exited with status "sv << krfb_exit;
      if (!krfb_out.empty()) {
        BOOST_LOG(error) << "headless_compositor: krfb output:\n"sv << krfb_out;
      }
      return false;
    }
    if (!krfb_out.empty()) {
      BOOST_LOG(debug) << "headless_compositor: krfb-virtualmonitor output:\n"sv << krfb_out;
    }

    BOOST_LOG(info) << "headless_compositor: krfb virtual output \""sv << _output_name
                    << "\" created at "sv << width << 'x' << height << '@' << refresh_hz;
    return true;
  }

  bool compositor_t::start_gamescope(int width, int height, int refresh_hz, const std::string &game_cmd) {
    auto gamescope_path = bp::search_path("gamescope");
    if (gamescope_path.empty()) {
      BOOST_LOG(error) << "headless_compositor: gamescope not found in PATH"sv;
      return false;
    }

    _output_name = "HEADLESS-1";

    auto run_dir = user_runtime_dir();
    if (run_dir.empty()) {
      BOOST_LOG(error) << "headless_compositor: cannot determine user runtime directory"sv;
      return false;
    }

    auto before = list_wayland_sockets(run_dir, std::string("wayland-"));

    _pid = fork();
    if (_pid < 0) {
      BOOST_LOG(error) << "headless_compositor: fork failed"sv;
      return false;
    }

    if (_pid == 0) {
      setsid();

      auto max_fd = sysconf(_SC_OPEN_MAX);
      if (max_fd < 0) max_fd = 1024;
      for (int fd = 3; fd < max_fd; ++fd) close(fd);

      int nullfd = open("/dev/null", O_RDWR);
      if (nullfd >= 0) {
        dup2(nullfd, STDIN_FILENO);
        dup2(nullfd, STDOUT_FILENO);
        dup2(nullfd, STDERR_FILENO);
        if (nullfd > STDERR_FILENO) close(nullfd);
      }

      // Gamescope headless mode creates a virtual output backed by EGL.
      // Nested mode: Gamescope runs inside the existing Wayland session.
      setenv("WLR_NO_HARDWARE_CURSORS", "1", 1);
      unsetenv("DISPLAY");

      // Use nested mode: gamescope runs as a Wayland client in the current session,
      // creating a virtual output for the game.
      execl(gamescope_path.c_str(), "gamescope",
        "--headless",
        "--prefer-vk-device",  // Prefer Vulkan for rendering
        "-W", std::to_string(width).c_str(),
        "-H", std::to_string(height).c_str(),
        "-r", std::to_string(refresh_hz).c_str(),
        "--", "sh", "-c", game_cmd.c_str(),
        nullptr);

      _exit(127);
    }

    // Discover the new Wayland socket.
    std::string discovered;
    for (int attempt = 0; attempt < 50; ++attempt) {
      std::this_thread::sleep_for(100ms);
      auto after = list_wayland_sockets(run_dir, std::string("wayland-"));
      for (auto &s : after) {
        if (std::find(before.begin(), before.end(), s) == before.end()) {
          discovered = std::move(s);
          break;
        }
      }
      if (!discovered.empty()) break;
    }

    if (discovered.empty()) {
      BOOST_LOG(error) << "headless_compositor: could not discover gamescope Wayland socket"sv;
      stop();
      return false;
    }

    _wayland_socket = run_dir + "/" + discovered;
    BOOST_LOG(info) << "headless_compositor: gamescope WAYLAND_DISPLAY="sv << _wayland_socket;
    return true;
  }

  bool compositor_t::start_labwc(int width, int height, int refresh_hz, const std::string &game_cmd) {
    if (width <= 0 || height <= 0) {
      BOOST_LOG(error) << "headless_compositor: invalid dimensions "sv << width << 'x' << height;
      return false;
    }

    auto labwc_path = bp::search_path("labwc");
    if (labwc_path.empty()) {
      BOOST_LOG(error) << "headless_compositor: labwc not found in PATH"sv;
      return false;
    }

    auto wlr_randr_path = bp::search_path("wlr-randr");
    if (wlr_randr_path.empty()) {
      BOOST_LOG(error) << "headless_compositor: wlr-randr not found in PATH"sv;
      return false;
    }

    auto run_dir = user_runtime_dir();
    if (run_dir.empty()) {
      BOOST_LOG(error) << "headless_compositor: cannot determine user runtime directory"sv;
      return false;
    }

    BOOST_LOG(info) << "headless_compositor: labwc="sv << labwc_path << " wlr-randr="sv << wlr_randr_path
                    << " run_dir="sv << run_dir;

    // Snapshot existing Wayland sockets so we can detect the new one.
    auto before = list_wayland_sockets(run_dir, std::string("wayland-"));

    _pid = fork();
    if (_pid < 0) {
      BOOST_LOG(error) << "headless_compositor: fork failed"sv;
      return false;
    }

    if (_pid == 0) {
      // Child: launch labwc in its own session.
      setsid();

      // Close all open file descriptors except stdin/stdout/stderr.
      auto max_fd = sysconf(_SC_OPEN_MAX);
      if (max_fd < 0) max_fd = 1024;
      for (int fd = 3; fd < max_fd; ++fd) {
        close(fd);
      }

      // Redirect child stdio to /dev/null.
      int nullfd = open("/dev/null", O_RDWR);
      if (nullfd >= 0) {
        dup2(nullfd, STDIN_FILENO);
        dup2(nullfd, STDOUT_FILENO);
        dup2(nullfd, STDERR_FILENO);
        if (nullfd > STDERR_FILENO) close(nullfd);
      }

      // Headless compositor environment.
      setenv("WLR_BACKENDS", "headless", 1);
      setenv("WLR_RENDERER", "gles2", 1);
      setenv("WLR_HEADLESS_OUTPUTS", "1", 1);
      setenv("WLR_NO_HARDWARE_CURSORS", "1", 1);

      unsetenv("DISPLAY");
      unsetenv("WAYLAND_DISPLAY");

      // game_cmd is intentionally NOT exec'd by labwc itself: Sunshine
      // launches the game process separately via run_command() (see
      // process.cpp) so Sunshine retains ownership of the PID and the
      // teardown ordering. labwc has no --command flag of its own; the
      // game is started after labwc's WAYLAND_DISPLAY is discovered and
      // injected into _env. The parameter is kept in the signature for
      // parity with start_gamescope() / wrap_cmd().
      (void) game_cmd;

      execl(labwc_path.c_str(), "labwc", nullptr);

      // execl only returns on error.
      _exit(127);
    }

    // Parent: discover the new Wayland socket.
    // Poll up to 50 times × 100 ms = 5 seconds.
    std::string discovered_socket;
    for (int attempt = 0; attempt < 50; ++attempt) {
      std::this_thread::sleep_for(100ms);

      auto after = list_wayland_sockets(run_dir, std::string("wayland-"));
      for (auto &s : after) {
        if (std::find(before.begin(), before.end(), s) == before.end()) {
          discovered_socket = std::move(s);
          break;
        }
      }
      if (!discovered_socket.empty()) break;
    }

    if (discovered_socket.empty()) {
      BOOST_LOG(error) << "headless_compositor: could not discover new WAYLAND_DISPLAY socket"sv;
      stop();
      return false;
    }

    _wayland_socket = run_dir + "/" + discovered_socket;
    BOOST_LOG(info) << "headless_compositor: discovered WAYLAND_DISPLAY="sv << _wayland_socket;

    // Poll for the HEADLESS-1 output to be ready using wlr-randr.
    bool output_ready = false;
    for (int attempt = 0; attempt < 50; ++attempt) {
      std::this_thread::sleep_for(100ms);

      std::error_code ec;
      std::string wlr_out;
      {
        bp::ipstream pipe_stream;
        bp::environment env {};
        env["WAYLAND_DISPLAY"] = _wayland_socket;
        bp::child proc(wlr_randr_path, bp::std_out > pipe_stream, env, ec);
        if (!ec) {
          proc.wait(ec);
          if (!ec) {
            std::ostringstream oss;
            oss << pipe_stream.rdbuf();
            wlr_out = oss.str();
          }
        }
      }

      if (wlr_out.find("HEADLESS-1"sv) != std::string::npos) {
        BOOST_LOG(info) << "headless_compositor: HEADLESS-1 output detected"sv;
        output_ready = true;
        break;
      }
    }

    if (!output_ready) {
      BOOST_LOG(error) << "headless_compositor: HEADLESS-1 output not detected in time"sv;
      stop();
      return false;
    }

    // Discover XWayland display via /tmp/.X11-unix/ detection.
    auto before_x11 = list_wayland_sockets(std::string("/tmp/.X11-unix"), std::string("X"));

    // Wait briefly for XWayland to start.
    for (int attempt = 0; attempt < 20; ++attempt) {
      std::this_thread::sleep_for(100ms);
      auto after_x11 = list_wayland_sockets(std::string("/tmp/.X11-unix"), std::string("X"));
      for (auto &x : after_x11) {
        if (std::find(before_x11.begin(), before_x11.end(), x) == before_x11.end()) {
          _x11_display = ":" + x.substr(1);
          BOOST_LOG(info) << "headless_compositor: discovered XWayland display="sv << _x11_display;
          return true;
        }
      }
    }

    // XWayland may not be available; that's okay.
    BOOST_LOG(info) << "headless_compositor: XWayland not detected, headless compositor is ready"sv;
    return true;
  }

  void compositor_t::stop() {
    // Dispatch teardown by the active backend flag, not by string-matching
    // the output name. Previously this routed by checking
    // `_output_name != "HEADLESS-1"`, which silently broke if gamescope
    // ever changed its default output name or if a future backend reused
    // the literal "HEADLESS-1".
    if (_using_krfb) {
      stop_krfb();
    } else if (_using_gamescope) {
      stop_gamescope();
    } else {
      stop_labwc();
    }
  }

  void compositor_t::stop_krfb() {
    if (_output_name.empty()) return;

    auto krfb_path = bp::search_path("krfb-virtualmonitor");
    if (krfb_path.empty()) {
      _output_name.clear();
      return;
    }

    BOOST_LOG(info) << "headless_compositor: removing krfb virtual output \""sv << _output_name << '"';
    std::error_code ec;
    bp::child proc(krfb_path, "--remove", _output_name, ec);
    if (!ec) proc.wait(ec);
    _output_name.clear();
  }

  void compositor_t::stop_gamescope() {
    if (_pid <= 0) return;
    BOOST_LOG(info) << "headless_compositor: stopping gamescope process "sv << _pid;
    kill(-_pid, SIGTERM);
    for (int i = 0; i < 30; ++i) {
      std::this_thread::sleep_for(100ms);
      int status = 0;
      pid_t r = waitpid(_pid, &status, WNOHANG);
      if (r > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
          BOOST_LOG(warning) << "headless_compositor: gamescope exited with status "sv << WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
          BOOST_LOG(warning) << "headless_compositor: gamescope killed by signal "sv << WTERMSIG(status);
        }
        _pid = -1;
        return;
      }
    }
    kill(-_pid, SIGKILL);
    int status = 0;
    waitpid(_pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      BOOST_LOG(warning) << "headless_compositor: gamescope exited with status "sv << WEXITSTATUS(status) << " after SIGKILL";
    } else if (WIFSIGNALED(status)) {
      BOOST_LOG(warning) << "headless_compositor: gamescope killed by signal "sv << WTERMSIG(status);
    }
    _pid = -1;
  }

  void compositor_t::stop_labwc() {
    if (_pid <= 0) return;

    BOOST_LOG(info) << "headless_compositor: stopping process group "sv << _pid;

    // Send SIGTERM to the entire process group.
    kill(-_pid, SIGTERM);

    // Wait up to 3 seconds for graceful shutdown.
    for (int i = 0; i < 30; ++i) {
      std::this_thread::sleep_for(100ms);
      int status = 0;
      pid_t r = waitpid(_pid, &status, WNOHANG);
      if (r > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
          BOOST_LOG(warning) << "headless_compositor: labwc exited with status "sv << WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
          BOOST_LOG(warning) << "headless_compositor: labwc killed by signal "sv << WTERMSIG(status);
        } else {
          BOOST_LOG(info) << "headless_compositor: compositor exited gracefully"sv;
        }
        _pid = -1;
        return;
      }
    }

    // Force kill.
    BOOST_LOG(info) << "headless_compositor: sending SIGKILL"sv;
    kill(-_pid, SIGKILL);
    int status = 0;
    waitpid(_pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      BOOST_LOG(warning) << "headless_compositor: labwc exited with status "sv << WEXITSTATUS(status) << " after SIGKILL";
    } else if (WIFSIGNALED(status)) {
      BOOST_LOG(warning) << "headless_compositor: labwc killed by signal "sv << WTERMSIG(status);
    }
    _pid = -1;
  }

  int compositor_t::pid() const {
    return _pid;
  }

  const std::string &compositor_t::wayland_socket() const {
    return _wayland_socket;
  }

  const std::string &compositor_t::x11_display() const {
    return _x11_display;
  }

  std::string compositor_t::wrap_cmd(const std::string &game_cmd) {
    if (_using_krfb) return game_cmd;
    if (!_wayland_socket.empty()) {
      auto wrapped = "WAYLAND_DISPLAY="s + _wayland_socket + " ";
      if (!_x11_display.empty()) {
        wrapped += "DISPLAY="s + _x11_display + " ";
      }
      return wrapped + game_cmd;
    }
    return game_cmd;
  }

  const std::string &compositor_t::output_name() const {
    if (!_output_name.empty()) return _output_name;
    static const std::string headless1 {"HEADLESS-1"};
    return headless1;
  }

}  // namespace platf::headless
