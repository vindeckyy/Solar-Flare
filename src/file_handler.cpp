// SPDX-License-Identifier: GPL-3.0-only

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

using namespace std::literals;

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

  /**
   * @brief Ensure a directory exists, creating it recursively if needed.
   * @param path Directory path.
   * @return true if the directory exists or was created, false on error.
   */
  bool make_directory(const std::string &path) {
    if (path.empty()) {
      BOOST_LOG(warning) << "make_directory: empty path"sv;
      return false;
    }
    // first, check if the directory already exists
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
      if (ec) {
        BOOST_LOG(warning) << "make_directory: exists check failed for ["sv << path << "]: "sv << ec.message();
        return false;
      }
      return true;
    }

    if (!std::filesystem::create_directories(path, ec)) {
      if (ec) {
        BOOST_LOG(warning) << "make_directory: create_directories ["sv << path << "] failed: "sv << ec.message();
        return false;
      }
      // create_directories returns false when the directory already exists
      // (raced). Treat as success if it now exists.
      return std::filesystem::exists(path, ec);
    }
    return true;
  }

  /**
   * @brief Read a file to string.
   * @param path Path to the file.
   * @return File contents, or empty on missing/unreadable.
   */
  std::string read_file(const char *path) {
    if (!path || !*path) {
      BOOST_LOG(warning) << "read_file: empty path"sv;
      return {};
    }
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
      if (ec) {
        BOOST_LOG(warning) << "read_file: exists check failed for ["sv << path << "]: "sv << ec.message();
      } else {
        BOOST_LOG(debug) << "Missing file: " << path;
      }
      return {};
    }
    if (ec) {
      BOOST_LOG(warning) << "read_file: stat failed for ["sv << path << "]: "sv << ec.message();
      return {};
    }

    std::ifstream in(path);
    if (!in) {
      BOOST_LOG(warning) << "read_file: failed to open ["sv << path << "]: "sv << std::strerror(errno);
      return {};
    }
    return std::string {(std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()};
  }

  /**
   * @brief Write contents to a file, creating parent directories.
   * @param path Destination file path.
   * @param contents Data to write.
   * @return 0 on success, -1 on failure to open or write.
   */
  int write_file(const char *path, const std::string_view &contents) {
    if (!path || !*path) {
      BOOST_LOG(warning) << "write_file: empty path"sv;
      return -1;
    }
    // Ensure the parent directory exists. The on-disk config is written by
    // callers like saveConfig() that assume the destination is reachable;
    // without an mkdir-p here, a fresh appdata directory surfaces as a
    // silent failure (ofstream::open returns null and the caller can't tell
    // the user "save didn't take" because the API contract ignored the
    // return value).
    if (const auto parent = get_parent_directory(path); !parent.empty() && parent != "." && parent != "/") {
      std::error_code ec;
      std::filesystem::create_directories(parent, ec);
      if (ec) {
        BOOST_LOG(warning) << "write_file: create_directories ["sv << parent << "] failed: "sv << ec.message();
        return -1;
      }
    }

    std::ofstream out(path);
    if (!out.is_open()) {
      BOOST_LOG(warning) << "write_file: failed to open ["sv << path << "] for writing: "sv << std::strerror(errno);
      return -1;
    }

    out << contents;
    if (!out) {
      BOOST_LOG(warning) << "write_file: failed to write ["sv << path << "]: "sv << std::strerror(errno);
      return -1;
    }

    return 0;
  }
}  // namespace file_handler
