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
