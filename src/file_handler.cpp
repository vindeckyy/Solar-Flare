/**
 * @file file_handler.cpp
 * @brief Definitions for file handling functions.
 */

// standard includes
#include <filesystem>
#include <fstream>

// local includes
#include "file_handler.h"
#include "logging.h"

namespace file_handler {
  std::string get_parent_directory(const std::string &path) {
    // remove any trailing path separators
    std::string trimmed_path = path;
    while (!trimmed_path.empty() && trimmed_path.back() == '/') {
      trimmed_path.pop_back();
    }

    std::filesystem::path p(trimmed_path);
    return p.parent_path().string();
  }

  bool make_directory(const std::string &path) {
    // first, check if the directory already exists
    if (std::filesystem::exists(path)) {
      return true;
    }

    return std::filesystem::create_directories(path);
  }

  std::string read_file(const char *path) {
    if (!std::filesystem::exists(path)) {
      BOOST_LOG(debug) << "Missing file: " << path;
      return {};
    }

    std::ifstream in(path);
    return std::string {(std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()};
  }

  int write_file(const char *path, const std::string_view &contents) {
    // Ensure the parent directory exists. The on-disk config is written by
    // callers like saveConfig() that assume the destination is reachable;
    // without an mkdir-p here, a fresh appdata directory surfaces as a
    // silent failure (ofstream::open returns null and the caller can't tell
    // the user "save didn't take" because the API contract ignored the
    // return value).
    if (const auto parent = get_parent_directory(path); !parent.empty() && parent != "." && parent != "/") {
      std::error_code ec;
      std::filesystem::create_directories(parent, ec);
      // create_directories sets ec on failure but also returns false; we
      // don't propagate the error here -- is_open() below is the source of
      // truth for whether the file is writable.
    }

    std::ofstream out(path);

    if (!out.is_open()) {
      return -1;
    }

    out << contents;

    return 0;
  }
}  // namespace file_handler
