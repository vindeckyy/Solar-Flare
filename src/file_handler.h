// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file file_handler.h
 * @brief Declarations for file handling functions.
 */
#pragma once

// standard includes
#include <string>

/**
 * @brief Responsible for file handling functions.
 */
namespace file_handler {
  /**
   * @brief Get the parent directory of a file or directory.
   * @param path The path of the file or directory.
   * @return The parent directory.
   * @examples
   * std::string parent_dir = get_parent_directory("path/to/file");
   * @examples_end
   */
  std::string get_parent_directory(const std::string &path);

  /**
   * @brief Make a directory.
   * @param path The path of the directory.
   * @return `true` on success, `false` on failure.
   * @examples
   * bool dir_created = make_directory("path/to/directory");
   * @examples_end
   */
  bool make_directory(const std::string &path);

  /**
   * @brief Read a file to string.
   * @param path The path of the file.
   * @return The contents of the file.
   * @examples
   * std::string contents = read_file("path/to/file");
   * @examples_end
   */
  std::string read_file(const char *path);

  /**
   * @brief Writes a file, ensuring the parent directory exists.
   * @param path The path of the file.
   * @param contents The contents to write.
   * @return ``0`` on success, ``-1`` on failure to open or write the file.
   * @note The parent directory of @p path is created (recursively) if it
   *       does not already exist, so callers do not need to pre-create it.
   *       The function still returns ``-1`` if the file itself cannot be
   *       opened (insufficient permissions, read-only mount, etc.).
   * @examples
   * int write_status = write_file("path/to/file", "file contents");
   * @examples_end
   */
  int write_file(const char *path, const std::string_view &contents);

  /**
   * @brief Check whether a requested path is safely contained within a root directory.
   *
   * Canonicalizes both @p path and @p root via @c std::filesystem::weakly_canonical
   * and verifies the canonical requested path starts with the canonical root.
   * This blocks directory-traversal (``..``) and symlink-escape attacks.
   *
   * @param path Requested file path to validate (absolute or relative).
   * @param root Allowed root directory; the canonical @p path must be inside it.
   * @return ``true`` if the resolved @p path is within @p root, ``false`` otherwise.
   * @note Returns ``false`` for empty @p path or empty @p root, and for any
   *       filesystem error (missing component, permission denied, etc.) where
   *       @c weakly_canonical throws @c std::filesystem::filesystem_error.
   * @examples
   * bool ok = is_safe_path("/srv/assets/../etc/passwd", "/srv/assets");
   * // ok == false
   * @examples_end
   */
  bool is_safe_path(const std::string &path, const std::string &root);
}  // namespace file_handler
