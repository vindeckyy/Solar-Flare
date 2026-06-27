/**
 * @file tests/unit/test_solarflare_submodule_shas.cpp
 * @brief Simple regression guard for the round-6 submodule-pointer
 *        cherry-picks (9cdf44ea, c2a74487, 5dcf3f08).
 */
#include "../tests_common.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

namespace {

  // Walk up the directory tree to find the source root (the dir
  // containing .gitmodules). Starts from "." (test CWD is
  // build/tests/).
  std::string find_source_root() {
    std::string path = ".";
    for (int i = 0; i < 10; ++i) {
      std::error_code ec;
      if (std::filesystem::exists(path + "/.gitmodules", ec)) {
        return path;
      }
      if (path == ".") { path = ".."; continue; }
      auto slash = path.find_last_of('/');
      if (slash == std::string::npos || slash == 0) return "";
      path = path.substr(0, slash);
    }
    return "";
  }

  // Run 'git submodule status' and return the output.
  std::string run_git_submodule_status(const std::string &source_root) {
    std::unique_ptr<FILE, int (*)(FILE *)> pipe(
      popen(("cd " + source_root + " && git submodule status").c_str(), "r"),
      pclose);
    if (!pipe) return "";
    std::string out;
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe.get())) out += buf;
    return out;
  }

  struct SubmodulePointer {
    const char *name;
    const char *path;
    const char *round6_sha_prefix;
  };
  constexpr std::array<SubmodulePointer, 3> kSubmodules = {{
    {"lizardbyte-common",   "third-party/lizardbyte-common",   "8d7dcc9"},
    {"moonlight-common-c",  "third-party/moonlight-common-c",  "47b4d33"},
    {"nvapi",               "third-party/nvapi",               "cd6918f"},
  }};

}  // namespace

TEST(SolarflareSubmoduleShas, OnDiskShasAreRound6Bumped) {
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
    const std::string line = status.substr(line_start, path_pos - line_start);
    EXPECT_GE(line.size(), 40u) << "Line too short for " << sm.name << ": '" << line << "'";
    const std::string sha = line.substr(0, 40);
    EXPECT_EQ(sha.substr(0, 7), std::string(sm.round6_sha_prefix))
      << "Submodule " << sm.name << " SHA '" << sha
      << "' does not start with expected round-6 prefix '"
      << sm.round6_sha_prefix << "'. The pointer has been reverted. "
         "Re-apply the round-6 cherry-pick.";
  }
}
