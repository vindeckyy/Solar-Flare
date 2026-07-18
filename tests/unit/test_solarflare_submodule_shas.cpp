// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_solarflare_submodule_shas.cpp
 * @brief Regression guard for SolarFlare's expected submodule pointers.
 */
#include "../tests_common.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

namespace {

  /**
   * @brief Walk up from CWD to find the dir containing .gitmodules.
   *
   * @return Absolute-or-relative source root path, or empty string if
   *         not found within 10 hops.
   */
  std::string find_source_root() {
    std::string path = ".";
    for (int i = 0; i < 10; ++i) {
      std::error_code ec;
      if (std::filesystem::exists(path + "/.gitmodules", ec)) {
        return path;
      }
      // Append ".." once we've moved past "." so we walk up from there.
      // ponytail: handles "." → ".." → "../.." instead of the npos bailout.
      if (path == ".") {
        path = "..";
        continue;
      }
      path += "/..";
    }
    return "";
  }

  // Run 'git submodule status' and return the output.
  std::string run_git_submodule_status(const std::string &source_root) {
    std::unique_ptr<FILE, int (*)(FILE *)> pipe(
      popen(("cd " + source_root + " && git submodule status").c_str(), "r"),
      pclose
    );
    if (!pipe) {
      return "";
    }
    std::string out;
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe.get())) {
      out += buf;
    }
    return out;
  }

  /**
   * @brief Identifies a submodule and the commit prefix expected on disk.
   */
  struct SubmodulePointer {
    const char *name;  ///< Human-readable submodule name.
    const char *path;  ///< Path relative to the source root.
    const char *expected_sha_prefix;  ///< Prefix of the pinned commit SHA.
  };

  constexpr std::array<SubmodulePointer, 3> kSubmodules = {{
    {"lizardbyte-common", "third-party/lizardbyte-common", "06cd442"},
    {"moonlight-common-c", "third-party/moonlight-common-c", "82e2514"},
    {"nvapi", "third-party/nvapi", "cd6918f"},
  }};

}  // namespace

TEST(SolarflareSubmoduleShas, OnDiskShasMatchExpectedPins) {
  const std::string source_root = find_source_root();
  EXPECT_FALSE(source_root.empty())
    << "Could not find source root (.gitmodules) by walking up.";

  const std::string status = run_git_submodule_status(source_root);
  EXPECT_FALSE(status.empty())
    << "'git submodule status' returned no output.";

  for (const auto &sm : kSubmodules) {
    const std::string needle = " " + std::string(sm.path) + " ";
    const size_t path_pos = status.find(needle);
    if (path_pos == std::string::npos) {
      const std::string alt_needle = "-" + std::string(sm.path) + " ";
      EXPECT_NE(status.find(alt_needle), std::string::npos)
        << "Could not find " << sm.name << " in 'git submodule status':\n"
        << status;
      continue;
    }
    size_t line_start = status.rfind('\n', path_pos);
    line_start = (line_start == std::string::npos) ? 0 : line_start + 1;
    std::string line = status.substr(line_start, path_pos - line_start);
    // 'git submodule status' pads the SHA column for the '-' marker
    // used on uninitialised submodules, so the line begins with a
    // single space (e.g. " cd6918f..."). Strip leading whitespace
    // before extracting the SHA.
    const size_t first_non_ws = line.find_first_not_of(" \t");
    if (first_non_ws != std::string::npos) {
      line = line.substr(first_non_ws);
    }
    EXPECT_GE(line.size(), 40u) << "Line too short for " << sm.name << ": '" << line << "'";
    const std::string sha = line.substr(0, 40);
    EXPECT_EQ(sha.substr(0, 7), std::string(sm.expected_sha_prefix))
      << "Submodule " << sm.name << " SHA '" << sha
      << "' does not start with expected prefix '"
      << sm.expected_sha_prefix << "'. Synchronize the checkout with "
                                   "the submodule pointer recorded by this commit.";
  }
}
