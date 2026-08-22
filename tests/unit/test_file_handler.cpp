// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_file_handler.cpp
 * @brief Test src/file_handler.*.
 */
#include "../tests_common.h"

#include <format>
#include <src/file_handler.h>

struct FileHandlerParentDirectoryTest: testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(FileHandlerParentDirectoryTest, Run) {
  auto [input, expected] = GetParam();
  EXPECT_EQ(file_handler::get_parent_directory(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
  FileHandlerTests,
  FileHandlerParentDirectoryTest,
  testing::Values(
    std::make_tuple("/path/to/file.txt", "/path/to"),
    std::make_tuple("/path/to/directory", "/path/to"),
    std::make_tuple("/path/to/directory/", "/path/to")
  )
);

struct FileHandlerMakeDirectoryTest: testing::TestWithParam<std::tuple<std::string, bool, bool>> {};

TEST_P(FileHandlerMakeDirectoryTest, Run) {
  auto [input, expected, remove] = GetParam();
  const std::string test_dir = platf::appdata().string() + "/tests/path/";
  input = test_dir + input;

  EXPECT_EQ(file_handler::make_directory(input), expected);
  EXPECT_TRUE(std::filesystem::exists(input));

  // remove test directory
  if (remove) {
    std::filesystem::remove_all(test_dir);
    EXPECT_FALSE(std::filesystem::exists(test_dir));
  }
}

INSTANTIATE_TEST_SUITE_P(
  FileHandlerTests,
  FileHandlerMakeDirectoryTest,
  testing::Values(
    std::make_tuple("dir_123", true, false),
    std::make_tuple("dir_123", true, true),
    std::make_tuple("dir_123/abc", true, false),
    std::make_tuple("dir_123/abc", true, true)
  )
);

struct FileHandlerTests: testing::TestWithParam<std::tuple<int, std::string>> {};

INSTANTIATE_TEST_SUITE_P(
  TestFiles,
  FileHandlerTests,
  testing::Values(
    std::make_tuple(0, ""),  // empty file
    std::make_tuple(1, "a"),  // single character
    std::make_tuple(2, "Mr. Blue Sky - Electric Light Orchestra"),  // single line
    std::make_tuple(3, R"(
Morning! Today's forecast calls for blue skies
The sun is shining in the sky
There ain't a cloud in sight
It's stopped raining
Everybody's in the play
And don't you know, it's a beautiful new day
Hey, hey, hey!
Running down the avenue
See how the sun shines brightly in the city
All the streets where once was pity
Mr. Blue Sky is living here today!
Hey, hey, hey!
    )")  // multi-line
  )
);

TEST_P(FileHandlerTests, WriteFileTest) {
  auto [fileNum, content] = GetParam();
  const std::string fileName = std::format("write_file_test_{}.txt", fileNum);
  EXPECT_EQ(file_handler::write_file(fileName.c_str(), content), 0);
}

TEST_P(FileHandlerTests, ReadFileTest) {
  auto [fileNum, content] = GetParam();
  const std::string fileName = std::format("write_file_test_{}.txt", fileNum);
  EXPECT_EQ(file_handler::read_file(fileName.c_str()), content);
}

TEST(FileHandlerTests, ReadMissingFileTest) {
  // read missing file
  EXPECT_EQ(file_handler::read_file("non-existing-file.txt"), "");
}

// Test: write_file creates missing parent directories (SolarFlare fork-fix).
// Regression guard for the user-visible "config save not working" symptom:
// saveConfig used to drop write_file's return code, so a missing parent dir
// surfaced as a silent no-op. Combined with the empty-payload wipe fix in
// saveConfig, write_file now mkdir-p's the parent so the write succeeds.
TEST(FileHandlerTests, WriteFile_CreatesMissingParentDirectories) {
  const std::string nested_dir = platf::appdata().string() + "/tests/write_file_mkdir/dir_a/dir_b";
  // ensure the test starts clean even if a prior run left directories behind
  std::error_code ec;
  std::filesystem::remove_all(platf::appdata().string() + "/tests/write_file_mkdir", ec);

  const std::string path = nested_dir + "/leaf.conf";
  EXPECT_FALSE(std::filesystem::exists(nested_dir));
  EXPECT_EQ(file_handler::write_file(path.c_str(), "leaf contents"), 0);
  EXPECT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(file_handler::read_file(path.c_str()), "leaf contents");

  std::filesystem::remove_all(platf::appdata().string() + "/tests/write_file_mkdir", ec);
}

// Test: write_file returns -1 when the target directory is read-only /
// unwritable. This is the disk-side failure mode that saveConfig used to
// mask by ignoring the return value; now saveConfig surfaces it as a 500
// JSON body so the caller knows the file did not get persisted.
TEST(FileHandlerTests, WriteFile_FailsOnUnwritableDirectory) {
  // Use a directory we know exists but cannot write to as a regular user.
  // On Linux, /proc is reliably non-writable by unprivileged processes.
  // Skip the test on Windows where /proc semantics differ.
#ifdef _WIN32
  GTEST_SKIP() << "Read-only path semantics differ on Windows; covered by platform tests.";
#else
  const std::string bad_path = "/proc/self/cmdline/should_fail_to_write.txt";
  const auto rc = file_handler::write_file(bad_path.c_str(), "ignored");
  EXPECT_EQ(rc, -1) << "write_file should fail when the parent directory is not writable";
#endif
}

/**
 * @brief Tests for file_handler::is_safe_path canonicalization guard.
 *
 * Covers empty/missing/error paths via weakly_canonical + filesystem_error
 * handling and verifies traversal / symlink-escape are blocked.
 */
TEST(FileHandlerIsSafePathTest, EmptyPathRejected) {
  // Arrange
  const std::string root = platf::appdata().string() + "/tests/is_safe_path/root_empty";
  std::error_code ec;
  std::filesystem::create_directories(root, ec);
  const std::string empty_path = "";

  // Act
  const bool result = file_handler::is_safe_path(empty_path, root);

  // Assert
  EXPECT_FALSE(result) << "empty path must be rejected";
  std::filesystem::remove_all(platf::appdata().string() + "/tests/is_safe_path", ec);
}

TEST(FileHandlerIsSafePathTest, EmptyRootRejected) {
  // Arrange
  const std::string valid_path = platf::appdata().string() + "/tests/is_safe_path/root_empty2/file.txt";
  std::error_code ec;
  std::filesystem::create_directories(platf::appdata().string() + "/tests/is_safe_path/root_empty2", ec);
  std::filesystem::create_directories(valid_path, ec);
  // create a dummy file so path exists
  file_handler::write_file(valid_path.c_str(), "data");

  // Act
  const bool result = file_handler::is_safe_path(valid_path, "");

  // Assert
  EXPECT_FALSE(result) << "empty root must be rejected";
  std::filesystem::remove_all(platf::appdata().string() + "/tests/is_safe_path", ec);
}

TEST(FileHandlerIsSafePathTest, TraversalRejected) {
  // Arrange
  const std::string root = platf::appdata().string() + "/tests/is_safe_path/traversal_root";
  std::error_code ec;
  std::filesystem::create_directories(root, ec);
  const std::string traversal_path = root + "/../../etc/passwd";
  // Also test explicit ".." inside root that escapes
  const std::string traversal2 = root + "/subdir/../../etc/passwd";
  std::filesystem::create_directories(root + "/subdir", ec);

  // Act
  const bool result1 = file_handler::is_safe_path(traversal_path, root);
  const bool result2 = file_handler::is_safe_path(traversal2, root);
  const bool result3 = file_handler::is_safe_path("../../etc/passwd", root);

  // Assert
  EXPECT_FALSE(result1) << "path with .. escaping root must be rejected";
  EXPECT_FALSE(result2) << "nested .. escaping root must be rejected";
  EXPECT_FALSE(result3) << "relative traversal must be rejected";
  std::filesystem::remove_all(platf::appdata().string() + "/tests/is_safe_path", ec);
}

TEST(FileHandlerIsSafePathTest, SymlinkEscapeRejected) {
  // Arrange
  const std::string base = platf::appdata().string() + "/tests/is_safe_path/symlink";
  const std::string root = base + "/root";
  const std::string outside = base + "/outside";
  std::error_code ec;
  std::filesystem::remove_all(base, ec);
  std::filesystem::create_directories(root, ec);
  std::filesystem::create_directories(outside, ec);
  const std::string outside_file = outside + "/secret.txt";
  file_handler::write_file(outside_file.c_str(), "secret");
  const std::string link_path = root + "/link_to_outside";
#ifdef _WIN32
  GTEST_SKIP() << "Symlink test skipped on Windows without privilege";
#else
  // Create symlink inside root pointing to outside directory
  std::filesystem::remove(link_path, ec);
  std::filesystem::create_symlink(outside, link_path, ec);
  if (ec) {
    GTEST_SKIP() << "Failed to create symlink, skipping: " << ec.message();
  }
  const std::string escaped = link_path + "/secret.txt";

  // Act
  const bool result = file_handler::is_safe_path(escaped, root);

  // Assert
  EXPECT_FALSE(result) << "symlink escape must be rejected via weakly_canonical";
  std::filesystem::remove_all(base, ec);
#endif
}

TEST(FileHandlerIsSafePathTest, ValidPathAccepted) {
  // Arrange
  const std::string root = platf::appdata().string() + "/tests/is_safe_path/valid_root";
  const std::string subdir = root + "/subdir";
  const std::string file = subdir + "/file.txt";
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
  std::filesystem::create_directories(subdir, ec);
  ASSERT_EQ(file_handler::write_file(file.c_str(), "hello"), 0);
  ASSERT_TRUE(std::filesystem::exists(file));

  // Act
  const bool result_file = file_handler::is_safe_path(file, root);
  const bool result_subdir = file_handler::is_safe_path(subdir, root);
  const bool result_root = file_handler::is_safe_path(root, root);

  // Assert
  EXPECT_TRUE(result_file) << "file inside root must be accepted";
  EXPECT_TRUE(result_subdir) << "subdir inside root must be accepted";
  EXPECT_TRUE(result_root) << "root itself must be accepted";
  std::filesystem::remove_all(platf::appdata().string() + "/tests/is_safe_path", ec);
}

TEST(FileHandlerIsSafePathTest, MissingFileHandled) {
  // Arrange
  const std::string root = platf::appdata().string() + "/tests/is_safe_path/missing_root";
  std::error_code ec;
  std::filesystem::create_directories(root, ec);
  const std::string missing = root + "/does_not_exist_xyz_123.txt";
  const std::string missing_nested = root + "/no_dir/file.txt";
  // Ensure they really do not exist
  std::filesystem::remove(missing, ec);
  std::filesystem::remove_all(root + "/no_dir", ec);

  // Act
  const bool result_missing = file_handler::is_safe_path(missing, root);
  const bool result_nested = file_handler::is_safe_path(missing_nested, root);
  const bool result_no_root = file_handler::is_safe_path(root + "/file.txt", root + "/nonexistent_root_xyz");

  // Assert
  EXPECT_FALSE(result_missing) << "missing file must be handled as unsafe (false) via exists/weakly_canonical";
  EXPECT_FALSE(result_nested) << "missing nested file must be handled as unsafe";
  EXPECT_FALSE(result_no_root) << "non-existent root must be handled as unsafe";
  std::filesystem::remove_all(platf::appdata().string() + "/tests/is_safe_path", ec);
}

TEST(FileHandlerIsSafePathTest, SimilarPrefixNotAccepted) {
  // Arrange - ensure /tmp/root vs /tmp/root2 boundary check
  const std::string base = platf::appdata().string() + "/tests/is_safe_path/prefix";
  const std::string root = base + "/root";
  const std::string sibling = base + "/root2";
  std::error_code ec;
  std::filesystem::remove_all(base, ec);
  std::filesystem::create_directories(root, ec);
  std::filesystem::create_directories(sibling, ec);
  const std::string sibling_file = sibling + "/file.txt";
  file_handler::write_file(sibling_file.c_str(), "data");

  // Act
  const bool result = file_handler::is_safe_path(sibling_file, root);

  // Assert
  EXPECT_FALSE(result) << "sibling prefix must not be accepted as inside root";
  std::filesystem::remove_all(base, ec);
}
