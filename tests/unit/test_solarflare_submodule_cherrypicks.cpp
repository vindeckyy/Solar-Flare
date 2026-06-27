/**
 * @file tests/unit/test_solarflare_submodule_cherrypicks.cpp
 * @brief Regression guard for the round-6 submodule-pointer
 *        cherry-picks (9cdf44ea, c2a74487, 5dcf3f08).
 */
#include "../tests_common.h"

#include "src/file_handler.h"

#include <cctype>
#include <string>

namespace {

  std::string read_file(const std::string &path) {
    return file_handler::read_file(path.c_str());
  }

  bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
  }

  bool is_lower_hex_40(const std::string &s) {
    if (s.size() < 40) return false;
    for (int i = 0; i < 40; ++i) {
      char c = s[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
  }

  struct SubmodulePointer {
    const char *name;
    const char *path;
    const char *round6_sha_prefix;
  };

  constexpr SubmodulePointer kSubmodules[] = {
    {"lizardbyte-common",   "third-party/lizardbyte-common",   "8d7dcc9"},
    {"moonlight-common-c",  "third-party/moonlight-common-c",  "47b4d33"},
    {"nvapi",               "third-party/nvapi",               "cd6918f"},
  };

  // Follow the .git file's gitdir: line to find the submodule's
  // HEAD file. The HEAD file contains a 40-char lowercase hex SHA
  // (possibly with no trailing newline) when the submodule is on
  // a detached HEAD.
  std::string read_submodule_head_sha(const std::string &path) {
    const std::string dot_git = path + "/.git";
    const std::string dot_git_contents = read_file(dot_git);
    if (dot_git_contents.empty()) return "";
    const std::string marker = "gitdir: ";
    const size_t pos = dot_git_contents.find(marker);
    if (pos == std::string::npos) return "";
    size_t eol = dot_git_contents.find('\n', pos);
    if (eol == std::string::npos) eol = dot_git_contents.size();
    std::string gitdir = dot_git_contents.substr(pos + marker.size(),
                                                  eol - pos - marker.size());
    size_t slash = path.find_last_of('/');
    const std::string parent = (slash == std::string::npos) ? "." : path.substr(0, slash);
    const std::string head_path = parent + "/" + gitdir + "/HEAD";
    const std::string head = read_file(head_path);
    if (head.empty()) return "";
    if (is_lower_hex_40(head)) return head.substr(0, 40);
    return "";
  }

}  // namespace

TEST(SolarflareSubmoduleCherryPicks, OnDiskShasAreRound6Bumped) {
  for (const auto &sm : kSubmodules) {
    const std::string sha = read_submodule_head_sha(sm.path);
    EXPECT_FALSE(sha.empty())
      << "Could not read HEAD SHA for submodule " << sm.name
      << " (path=" << sm.path << "). The submodule is either not "
         "checked out or the .git file is malformed. Run 'git "
         "submodule update --init --recursive'.";
    if (!sha.empty()) {
      EXPECT_TRUE(contains(sha, sm.round6_sha_prefix))
        << "Submodule " << sm.name
        << " is at SHA '" << sha << "', which does not start "
           "with the expected round-6 prefix '"
        << sm.round6_sha_prefix
        << "'. The submodule pointer has been reverted. Re-apply "
           "the round-6 cherry-pick.";
    }
  }
}

TEST(SolarflareSubmoduleCherryPicks, SubmoduleDirectoriesPresent) {
  for (const auto &sm : kSubmodules) {
    // The submodule directory must exist on disk.
    struct stat sb;
    EXPECT_EQ(stat(sm.path, &sb), 0)
      << "Submodule directory " << sm.path
      << " does not exist on disk. Run 'git submodule update "
         "--init --recursive' to check it out.";
  }
}
