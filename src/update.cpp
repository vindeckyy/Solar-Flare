// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/update.cpp
 * @brief SolarFlare Linux self-update engine.
 */

// standard includes
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

// lib includes
#include <boost/algorithm/string.hpp>
#include <boost/process/v1.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

// local includes
#include "crypto.h"
#include "file_handler.h"
#include "httpcommon.h"
#include "logging.h"
#include "platform/common.h"
#include "rtsp.h"
#include "update.h"

using namespace std::literals;

namespace fs = std::filesystem;
namespace bp = boost::process::v1;

namespace update {
  namespace {

    constexpr auto UPDATE_REPO = "vindeckyy/Solar-Flare"sv;
    constexpr auto TARBALL_NAME = "solarflare-linux-x86_64.tar.gz"sv;
    constexpr auto SUMS_NAME = "SHA256SUMS"sv;
#ifdef SUNSHINE_UPDATE_HELPER_PATH
    constexpr auto HELPER_PATH = SUNSHINE_UPDATE_HELPER_PATH;
#else
    constexpr auto HELPER_PATH = "/usr/local/libexec/solarflare-update-apply";
#endif
    constexpr auto DOWNLOAD_TIMEOUT_S = 600L;

    struct state_t {
      std::mutex mutex;
      status_t status;
      std::atomic<bool> worker_running {false};
      std::atomic<bool> apply_when_idle {false};
      fs::path staging_root;
      fs::path staging_payload;  ///< Extracted solarflare/ directory.
      std::string expected_tarball_sha256;
    };

    state_t g;

    /**
     * @brief Append a command/action line to the UI terminal log.
     * @param line Log line.
     */
    void append_log(std::string line) {
      std::lock_guard lock(g.mutex);
      g.status.log.push_back(std::move(line));
      if (g.status.log.size() > 400) {
        g.status.log.erase(g.status.log.begin(), g.status.log.begin() + 100);
      }
    }

    /**
     * @brief Update phase, message, and optional percent under the status lock.
     * @param phase New phase.
     * @param message Status message.
     * @param percent Progress percent, or -1.
     */
    void set_phase(phase_e phase, std::string message, int percent = -1) {
      std::lock_guard lock(g.mutex);
      g.status.phase = phase;
      g.status.message = std::move(message);
      g.status.percent = percent;
      g.status.busy = phase != phase_e::idle && phase != phase_e::ready &&
                      phase != phase_e::error && phase != phase_e::unsupported;
      g.status.can_apply = (phase == phase_e::ready);
    }

    /**
     * @brief Strip a leading `v` and any `-suffix` from a version tag.
     * @param version Raw version string.
     * @return Numeric dotted prefix suitable for comparison.
     */
    std::string normalize_version(std::string_view version) {
      std::string v {version};
      if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) {
        v.erase(0, 1);
      }
      if (const auto dash = v.find('-'); dash != std::string::npos) {
        v.erase(dash);
      }
      return v;
    }

    /**
     * @brief SHA-256 hex digest of a file on disk.
     * @param path File path.
     * @return Lowercase hex digest, or nullopt on failure.
     */
    std::optional<std::string> file_sha256(const fs::path &path) {
      crypto::md_ctx_t ctx {EVP_MD_CTX_create()};
      if (!ctx) {
        return std::nullopt;
      }
      if (!EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr)) {
        return std::nullopt;
      }

      std::ifstream file(path, std::ios::binary);
      if (!file) {
        return std::nullopt;
      }

      std::array<char, 1 << 16> buf {};
      while (file) {
        file.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        if (file.gcount() > 0 && !EVP_DigestUpdate(ctx.get(), buf.data(), static_cast<size_t>(file.gcount()))) {
          return std::nullopt;
        }
      }

      unsigned char digest[EVP_MAX_MD_SIZE];
      unsigned int digest_len = 0;
      if (!EVP_DigestFinal_ex(ctx.get(), digest, &digest_len)) {
        return std::nullopt;
      }

      std::ostringstream ss;
      ss << std::hex << std::setfill('0');
      for (unsigned int i = 0; i < digest_len; ++i) {
        ss << std::setw(2) << static_cast<int>(digest[i]);
      }
      return ss.str();
    }

    /**
     * @brief Download a HTTPS URL to @p dest with redirects and a long timeout.
     * @param url Source URL.
     * @param dest Destination file.
     * @return true on success.
     */
    bool download_https(const std::string &url, const fs::path &dest) {
      append_log("curl --location --fail -o " + dest.string() + " " + url);

      if (const auto parent = dest.parent_path(); !parent.empty()) {
        std::error_code ec;
        fs::create_directories(parent, ec);
        if (ec) {
          return false;
        }
      }

      CURL *curl = curl_easy_init();
      if (!curl) {
        return false;
      }
      if (http::restrict_protocols_to_https(curl) != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
      }

      FILE *fp = fopen(dest.c_str(), "wb");
      if (!fp) {
        curl_easy_cleanup(curl);
        return false;
      }

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "SolarFlare-Updater/1.0");
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT_S);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
      curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

      const CURLcode result = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      fclose(fp);

      if (result != CURLE_OK) {
        std::error_code ec;
        fs::remove(dest, ec);
        BOOST_LOG(error) << "Update download failed ["sv << url << "]: "sv << curl_easy_strerror(result);
        return false;
      }
      return true;
    }

    /**
     * @brief Resolve ~/.cache/solarflare/update (or $XDG_CACHE_HOME).
     * @return Cache root directory.
     */
    fs::path cache_root() {
      if (const char *xdg = std::getenv("XDG_CACHE_HOME"); xdg && *xdg) {
        return fs::path(xdg) / "solarflare" / "update";
      }
      const char *home = std::getenv("HOME");
      if (!home || !*home) {
        return fs::temp_directory_path() / "solarflare-update";
      }
      return fs::path(home) / ".cache" / "solarflare" / "update";
    }

    /**
     * @brief Absolute path of the running sunshine binary.
     * @return Path, or empty on failure.
     */
    fs::path self_exe() {
#ifdef __linux__
      std::error_code ec;
      auto path = fs::read_symlink("/proc/self/exe", ec);
      if (ec) {
        return {};
      }
      return path;
#else
      return {};
#endif
    }

    /**
     * @brief True when the process can write @p path (file or parent dir).
     * @param path Target path.
     * @return true if writable without elevation.
     */
    bool path_writable(const fs::path &path) {
      std::error_code ec;
      if (fs::exists(path, ec)) {
        return ::access(path.c_str(), W_OK) == 0;
      }
      const auto parent = path.parent_path();
      if (parent.empty()) {
        return false;
      }
      return ::access(parent.c_str(), W_OK) == 0;
    }

    /**
     * @brief Render an argument vector for the updater log.
     *
     * @param command Executable followed by its arguments.
     * @return Quoted log representation that is never executed.
     */
    std::string command_log(const std::vector<std::string> &command) {
      std::ostringstream log;
      bool first = true;
      for (const auto &argument : command) {
        if (!first) {
          log << ' ';
        }
        log << std::quoted(argument);
        first = false;
      }
      return log.str();
    }

    /**
     * @brief Run an executable with explicit arguments and append it to the log.
     *
     * @param command Executable followed by its arguments.
     * @return Child exit code, or -1 when spawning or waiting fails.
     */
    int run_logged(const std::vector<std::string> &command) {
      if (command.empty()) {
        return -1;
      }

      append_log(command_log(command));
      std::vector<std::string> arguments {command.begin() + 1, command.end()};
      std::error_code error;
      bp::child child(command.front(), bp::args(arguments), error);
      if (error) {
        append_log("# unable to start " + command.front() + ": " + error.message());
        return -1;
      }
      child.wait(error);
      if (error) {
        append_log("# unable to wait for " + command.front() + ": " + error.message());
        return -1;
      }
      return child.exit_code();
    }

    /**
     * @brief Copy staging payload onto the live install paths.
     * @param payload Extracted solarflare/ directory.
     * @param binary_dest Live binary path.
     * @param assets_dest Live assets directory (SUNSHINE_ASSETS_DIR).
     * @return true on success.
     */
    bool install_payload(const fs::path &payload, const fs::path &binary_dest, const fs::path &assets_dest) {
      const fs::path staged_bin = payload / "sunshine";
      const fs::path staged_assets = payload / "assets";
      if (!fs::exists(staged_bin) || !fs::is_directory(staged_assets)) {
        set_phase(phase_e::error, "Staged release is missing sunshine or assets/", -1);
        return false;
      }

      std::error_code ec;
      const fs::path bin_tmp = binary_dest.string() + ".new";
      const fs::path bin_prev = binary_dest.string() + ".prev";
      fs::remove(bin_tmp, ec);
      fs::copy_file(staged_bin, bin_tmp, fs::copy_options::overwrite_existing, ec);
      if (ec) {
        set_phase(phase_e::error, "Failed to stage new binary: " + ec.message(), -1);
        return false;
      }
      fs::permissions(bin_tmp, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec, ec);

      if (fs::exists(binary_dest)) {
        fs::remove(bin_prev, ec);
        fs::rename(binary_dest, bin_prev, ec);
        if (ec) {
          set_phase(phase_e::error, "Failed to keep previous binary: " + ec.message(), -1);
          return false;
        }
      }
      fs::rename(bin_tmp, binary_dest, ec);
      if (ec) {
        // Best-effort rollback of the rename.
        fs::rename(bin_prev, binary_dest, ec);
        set_phase(phase_e::error, "Failed to install new binary: " + ec.message(), -1);
        return false;
      }

      // Install assets via a temp tree so cross-device renames can fall back to copy.
      const fs::path assets_tmp = assets_dest.string() + ".new";
      const fs::path assets_prev = assets_dest.string() + ".prev";
      fs::remove_all(assets_tmp, ec);
      fs::create_directories(assets_dest.parent_path(), ec);
      fs::copy(staged_assets, assets_tmp, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
      if (ec) {
        set_phase(phase_e::error, "Failed to stage assets: " + ec.message(), -1);
        return false;
      }
      if (fs::exists(assets_dest)) {
        fs::remove_all(assets_prev, ec);
        fs::rename(assets_dest, assets_prev, ec);
        if (ec) {
          fs::remove_all(assets_tmp, ec);
          set_phase(phase_e::error, "Failed to keep previous assets: " + ec.message(), -1);
          return false;
        }
      }
      fs::rename(assets_tmp, assets_dest, ec);
      if (ec) {
        fs::copy(assets_tmp, assets_dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        fs::remove_all(assets_tmp, ec);
        if (ec) {
          set_phase(phase_e::error, "Failed to install assets: " + ec.message(), -1);
          return false;
        }
      }

      const int cap_rc = run_logged({"setcap", "cap_sys_admin,cap_sys_nice+p", binary_dest.string()});
      if (cap_rc != 0) {
        BOOST_LOG(warning) << "setcap returned "sv << cap_rc << " for "sv << binary_dest;
        append_log("# warning: setcap exited " + std::to_string(cap_rc) + " (KMS may need a manual setcap)");
      }
      return true;
    }

    /**
     * @brief Apply using the polkit helper when the install is not user-writable.
     * @param payload Staging payload directory.
     * @param binary_dest Live binary.
     * @param assets_dest Live assets dir.
     * @return true on success.
     */
    bool apply_with_helper(const fs::path &payload, const fs::path &binary_dest, const fs::path &assets_dest) {
      if (!fs::exists(HELPER_PATH)) {
        set_phase(phase_e::error, "Update helper missing. Run ./scripts/linux-install.sh again.", -1);
        append_log("# missing "s + HELPER_PATH);
        return false;
      }

      const int rc = run_logged({"pkexec", HELPER_PATH, "--staging", payload.string(), "--binary", binary_dest.string(), "--assets", assets_dest.string()});
      if (rc != 0) {
        set_phase(phase_e::error, "Privileged apply failed (pkexec/helper exit " + std::to_string(rc) + ")", -1);
        return false;
      }
      return true;
    }

    /**
     * @brief Perform apply once sessions allow it.
     */
    void apply_now() {
      const auto binary_dest = self_exe();
      const fs::path assets_dest {SUNSHINE_ASSETS_DIR};
      fs::path payload;
      {
        std::lock_guard lock(g.mutex);
        payload = g.staging_payload;
      }

      if (binary_dest.empty() || payload.empty() || !fs::exists(payload)) {
        set_phase(phase_e::error, "No staged update payload is ready", -1);
        return;
      }

      set_phase(phase_e::applying, "Installing update", 90);
      bool ok = false;
      if (path_writable(binary_dest) && path_writable(assets_dest)) {
        append_log("# applying in-process (install paths are writable)");
        ok = install_payload(payload, binary_dest, assets_dest);
      } else {
        ok = apply_with_helper(payload, binary_dest, assets_dest);
      }

      if (!ok) {
        g.worker_running = false;
        return;
      }

      set_phase(phase_e::restarting, "Restarting SolarFlare", 100);
      append_log("# platf::restart()");
      g.worker_running = false;
      platf::restart();
    }

    /**
     * @brief Wait for streaming sessions to end, then apply.
     */
    void wait_idle_then_apply() {
      set_phase(phase_e::waiting_idle, "Waiting for the stream to end", -1);
      append_log("# waiting until rtsp_stream::session_count() == 0");
      while (g.apply_when_idle.load()) {
        if (rtsp_stream::session_count() == 0) {
          apply_now();
          return;
        }
        std::this_thread::sleep_for(1s);
      }
      set_phase(phase_e::ready, "Apply cancelled while waiting for idle", -1);
      g.worker_running = false;
    }

    /**
     * @brief Background download / verify / stage worker.
     */
    void download_worker() {
#ifndef __linux__
      set_phase(phase_e::unsupported, "Updates are only available on Linux", -1);
      g.worker_running = false;
      return;
#else
      try {
        set_phase(phase_e::checking, "Checking for the latest release", 5);
        const std::string api_url = "https://api.github.com/repos/" + std::string(UPDATE_REPO) + "/releases/latest";
        const fs::path work = cache_root() / "work";
        std::error_code ec;
        fs::remove_all(work, ec);
        fs::create_directories(work, ec);

        const fs::path release_json = work / "release.json";
        if (!download_https(api_url, release_json)) {
          set_phase(phase_e::error, "Failed to fetch latest release metadata", -1);
          g.worker_running = false;
          return;
        }

        const auto meta = nlohmann::json::parse(file_handler::read_file(release_json.c_str()), nullptr, false);
        if (meta.is_discarded() || !meta.contains("tag_name")) {
          set_phase(phase_e::error, "Latest release metadata was invalid", -1);
          g.worker_running = false;
          return;
        }

        const std::string tag = meta["tag_name"].get<std::string>();
        const std::string html_url = meta.value("html_url", "");
        {
          std::lock_guard lock(g.mutex);
          g.status.latest_tag = tag;
          g.status.html_url = html_url;
          g.status.outdated = compare_versions(PROJECT_VERSION, tag) < 0;
        }
        append_log("# latest tag " + tag);

        std::string tarball_url = "https://github.com/" + std::string(UPDATE_REPO) + "/releases/download/" + tag + "/" + std::string(TARBALL_NAME);
        std::string sums_url = "https://github.com/" + std::string(UPDATE_REPO) + "/releases/download/" + tag + "/" + std::string(SUMS_NAME);
        if (meta.contains("assets") && meta["assets"].is_array()) {
          for (const auto &asset : meta["assets"]) {
            const auto name = asset.value("name", "");
            const auto url = asset.value("browser_download_url", "");
            if (name == TARBALL_NAME && !url.empty()) {
              tarball_url = url;
            }
            if (name == SUMS_NAME && !url.empty()) {
              sums_url = url;
            }
          }
        }

        set_phase(phase_e::downloading, "Downloading SHA256SUMS", 15);
        const fs::path sums_path = work / std::string(SUMS_NAME);
        if (!download_https(sums_url, sums_path)) {
          set_phase(phase_e::error, "Failed to download SHA256SUMS", -1);
          g.worker_running = false;
          return;
        }

        set_phase(phase_e::downloading, "Downloading release tarball", 35);
        const fs::path tarball_path = work / std::string(TARBALL_NAME);
        if (!download_https(tarball_url, tarball_path)) {
          set_phase(phase_e::error, "Failed to download release tarball", -1);
          g.worker_running = false;
          return;
        }

        set_phase(phase_e::verifying, "Verifying SHA-256", 70);
        append_log("sha256sum -c SHA256SUMS");
        const auto sums = parse_sha256sums(file_handler::read_file(sums_path.c_str()));
        const auto expected_it = sums.find(std::string(TARBALL_NAME));
        if (expected_it == sums.end()) {
          set_phase(phase_e::error, "SHA256SUMS does not list the release tarball", -1);
          g.worker_running = false;
          return;
        }
        const auto actual = file_sha256(tarball_path);
        if (!actual || !boost::iequals(*actual, expected_it->second)) {
          set_phase(phase_e::error, "Tarball SHA-256 mismatch", -1);
          append_log("# expected " + expected_it->second);
          append_log("# actual   " + (actual ? *actual : std::string("<missing>")));
          g.worker_running = false;
          return;
        }

        const fs::path extract_dir = work / "extract";
        fs::create_directories(extract_dir, ec);
        if (run_logged({"tar", "-xzf", tarball_path.string(), "-C", extract_dir.string()}) != 0) {
          set_phase(phase_e::error, "Failed to extract release tarball", -1);
          g.worker_running = false;
          return;
        }

        const fs::path payload = extract_dir / "solarflare";
        if (!fs::exists(payload / "sunshine") || !fs::is_directory(payload / "assets")) {
          set_phase(phase_e::error, "Extracted archive is missing the solarflare/ payload", -1);
          g.worker_running = false;
          return;
        }

        {
          std::lock_guard lock(g.mutex);
          g.staging_root = work;
          g.staging_payload = payload;
          g.expected_tarball_sha256 = expected_it->second;
        }

        set_phase(phase_e::ready, "Update staged and verified", 100);
        append_log("# ready to apply " + tag);

        // One-click path: apply as soon as staging succeeds (waits out live streams).
        g.worker_running = false;
        if (const auto err = apply(true)) {
          append_log(std::string("# auto-apply deferred: ") + *err);
        }
        return;
      } catch (const std::exception &ex) {
        set_phase(phase_e::error, std::string("Update failed: ") + ex.what(), -1);
      }
      g.worker_running = false;
#endif
    }

  }  // namespace

  std::string to_string(phase_e phase) {
    switch (phase) {
      case phase_e::idle:
        return "idle";
      case phase_e::checking:
        return "checking";
      case phase_e::downloading:
        return "downloading";
      case phase_e::verifying:
        return "verifying";
      case phase_e::ready:
        return "ready";
      case phase_e::waiting_idle:
        return "waiting_idle";
      case phase_e::applying:
        return "applying";
      case phase_e::restarting:
        return "restarting";
      case phase_e::error:
        return "error";
      case phase_e::unsupported:
        return "unsupported";
    }
    return "idle";
  }

  nlohmann::json to_json(const status_t &status) {
    return {
      {"phase", to_string(status.phase)},
      {"percent", status.percent},
      {"message", status.message},
      {"latest_tag", status.latest_tag},
      {"html_url", status.html_url},
      {"log", status.log},
      {"outdated", status.outdated},
      {"can_apply", status.can_apply},
      {"busy", status.busy},
      {"helper_path", apply_helper_path()},
    };
  }

  status_t status() {
#ifndef __linux__
    status_t s;
    s.phase = phase_e::unsupported;
    s.message = "Updates are only available on Linux";
    return s;
#else
    std::lock_guard lock(g.mutex);
    auto copy = g.status;
    copy.busy = g.worker_running.load() || copy.busy;
    return copy;
#endif
  }

  std::optional<std::string> start() {
#ifndef __linux__
    return "Updates are only available on Linux";
#else
    if (g.worker_running.exchange(true)) {
      return "An update is already in progress";
    }
    {
      std::lock_guard lock(g.mutex);
      g.status = status_t {};
      g.status.phase = phase_e::checking;
      g.status.message = "Starting update";
      g.status.busy = true;
      g.status.log.clear();
    }
    append_log("# update::start()");
    std::thread(download_worker).detach();
    return std::nullopt;
#endif
  }

  std::optional<std::string> apply(bool when_idle) {
#ifndef __linux__
    return "Updates are only available on Linux";
#else
    {
      std::lock_guard lock(g.mutex);
      if (g.status.phase != phase_e::ready && g.status.phase != phase_e::waiting_idle) {
        return "No verified update is staged yet";
      }
    }
    if (g.worker_running.exchange(true)) {
      return "An update is already in progress";
    }

    if (rtsp_stream::session_count() > 0) {
      if (!when_idle) {
        g.worker_running = false;
        return "A stream is active. End it first, or use when_idle apply.";
      }
      g.apply_when_idle = true;
      append_log("# update::apply(when_idle=true)");
      std::thread(wait_idle_then_apply).detach();
      return std::nullopt;
    }

    append_log("# update::apply()");
    std::thread(apply_now).detach();
    return std::nullopt;
#endif
  }

  std::unordered_map<std::string, std::string> parse_sha256sums(std::string_view body) {
    std::unordered_map<std::string, std::string> out;
    std::string line;
    std::istringstream in {std::string {body}};
    while (std::getline(in, line)) {
      boost::algorithm::trim(line);
      if (line.empty() || line[0] == '#') {
        continue;
      }
      // digest, whitespace, optional '*', filename
      const auto first_space = line.find_first_of(" \t");
      if (first_space == std::string::npos) {
        continue;
      }
      auto digest = line.substr(0, first_space);
      auto name = boost::algorithm::trim_copy(line.substr(first_space + 1));
      if (!name.empty() && name[0] == '*') {
        name.erase(0, 1);
      }
      boost::algorithm::to_lower(digest);
      if (digest.size() == 64 && !name.empty()) {
        // Keep basename only so paths in SUMS still match the downloaded file.
        out[fs::path(name).filename().string()] = digest;
      }
    }
    return out;
  }

  int compare_versions(std::string_view lhs, std::string_view rhs) {
    const auto a = normalize_version(lhs);
    const auto b = normalize_version(rhs);
    std::vector<int> pa;
    std::vector<int> pb;
    auto parse = [](const std::string &s, std::vector<int> &parts) {
      std::stringstream ss(s);
      std::string item;
      while (std::getline(ss, item, '.')) {
        try {
          parts.push_back(std::stoi(item));
        } catch (...) {
          parts.clear();
          return;
        }
      }
    };
    parse(a, pa);
    parse(b, pb);
    if (pa.empty() || pb.empty()) {
      return 0;
    }
    const size_t n = std::max(pa.size(), pb.size());
    pa.resize(n, 0);
    pb.resize(n, 0);
    for (size_t i = 0; i < n; ++i) {
      if (pa[i] < pb[i]) {
        return -1;
      }
      if (pa[i] > pb[i]) {
        return 1;
      }
    }
    return 0;
  }

  std::string apply_helper_path() {
    return HELPER_PATH;
  }

}  // namespace update
