// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_gpu_governor.cpp
 * @brief Unit tests for the AMD GPU performance-mode RAII governor.
 *
 * Uses an injectable fake DRM sysfs tree under a temporary directory so
 * apply/restore behaviour can be asserted without touching real hardware.
 */
#include "../tests_common.h"

// standard includes
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>

// local includes
#include "src/gpu_governor.h"

namespace fs = std::filesystem;

namespace {

  /**
   * @brief Create a writable fake DRM card level file under @p root.
   *
   * @param root Fake `/sys/class/drm` root.
   * @param card Zero-based card index.
   * @param initial Initial file contents (may be empty).
   * @return Path to the created level node.
   */
  fs::path make_card_level(const fs::path &root, int card, std::string_view initial = "auto") {
    const auto device_dir = root / ("card" + std::to_string(card)) / "device";
    fs::create_directories(device_dir);
    const auto path = device_dir / "power_dpm_force_performance_level";
    std::ofstream out(path);
    out << initial;
    return path;
  }

  /**
   * @brief Read the entire contents of a text file.
   *
   * @param path File to read.
   * @return File contents, or empty string when the file cannot be opened.
   */
  std::string read_file(const fs::path &path) {
    std::ifstream in(path);
    if (!in) {
      return {};
    }
    return std::string {
      std::istreambuf_iterator<char>(in),
      std::istreambuf_iterator<char>()
    };
  }

  class GpuGovernorTest: public ::testing::Test {
  protected:
    void SetUp() override {
      root_ = fs::temp_directory_path() / ("sf_gpu_gov_" + std::to_string(::getpid()) + "_" +
                                           std::to_string(reinterpret_cast<uintptr_t>(this)));
      fs::create_directories(root_);
    }

    void TearDown() override {
      std::error_code ec;
      fs::remove_all(root_, ec);
    }

    fs::path root_;  ///< Injectable fake DRM sysfs root for the test.
  };

}  // namespace

TEST_F(GpuGovernorTest, LevelPathJoinsCardAndNode) {
  EXPECT_EQ(
    video::gpu_governor_level_path("/tmp/drm", 2),
    "/tmp/drm/card2/device/power_dpm_force_performance_level"
  );
}

TEST_F(GpuGovernorTest, DisabledGuardDoesNotTouchSysfs) {
  const auto card0 = make_card_level(root_, 0, "auto");

  {
    video::gpu_governor_guard_t guard(false, root_.string());
    EXPECT_FALSE(guard.active());
    EXPECT_EQ(read_file(card0), "auto");
  }

  EXPECT_EQ(read_file(card0), "auto");
}

TEST_F(GpuGovernorTest, EnabledGuardAppliesPerformanceAndRestoresAuto) {
  const auto card0 = make_card_level(root_, 0, "auto");
  const auto card1 = make_card_level(root_, 1, "low");

  {
    video::gpu_governor_guard_t guard(true, root_.string());
#ifdef __linux__
    EXPECT_TRUE(guard.active());
    EXPECT_EQ(read_file(card0), "performance");
    EXPECT_EQ(read_file(card1), "performance");
#else
    EXPECT_FALSE(guard.active());
    EXPECT_EQ(read_file(card0), "auto");
    EXPECT_EQ(read_file(card1), "low");
#endif
  }

#ifdef __linux__
  EXPECT_EQ(read_file(card0), "auto");
  EXPECT_EQ(read_file(card1), "auto");
#else
  EXPECT_EQ(read_file(card0), "auto");
  EXPECT_EQ(read_file(card1), "low");
#endif
}

TEST_F(GpuGovernorTest, MissingCardsAreSkippedSilently) {
  // Only card0 exists; cards 1..3 are absent. Writes must not throw.
  const auto card0 = make_card_level(root_, 0, "auto");

  EXPECT_NO_THROW({
    video::gpu_governor_guard_t guard(true, root_.string());
#ifdef __linux__
    EXPECT_EQ(read_file(card0), "performance");
#endif
  });

#ifdef __linux__
  EXPECT_EQ(read_file(card0), "auto");
#endif
}

TEST_F(GpuGovernorTest, MoveTransfersRestoreOwnership) {
  const auto card0 = make_card_level(root_, 0, "auto");

  video::gpu_governor_guard_t first(true, root_.string());
#ifdef __linux__
  ASSERT_TRUE(first.active());
  EXPECT_EQ(read_file(card0), "performance");
#endif

  video::gpu_governor_guard_t second(std::move(first));
  EXPECT_FALSE(first.active());
#ifdef __linux__
  EXPECT_TRUE(second.active());
  EXPECT_EQ(read_file(card0), "performance");
#endif

  // Destroying the moved-from guard must not restore early.
  {
    video::gpu_governor_guard_t sunk(std::move(first));
    EXPECT_FALSE(sunk.active());
  }
#ifdef __linux__
  EXPECT_EQ(read_file(card0), "performance");
#endif

  // Drop the owner; restore happens here.
  {
    video::gpu_governor_guard_t owner(std::move(second));
#ifdef __linux__
    EXPECT_TRUE(owner.active());
#endif
  }
#ifdef __linux__
  EXPECT_EQ(read_file(card0), "auto");
#endif
}

TEST_F(GpuGovernorTest, ContextStyleUniquePtrRestoresOnReset) {
  const auto card0 = make_card_level(root_, 0, "auto");

  auto governor = std::make_unique<video::gpu_governor_guard_t>(true, root_.string());
#ifdef __linux__
  ASSERT_TRUE(governor->active());
  EXPECT_EQ(read_file(card0), "performance");
#endif

  // Mirrors capture context teardown: dropping the unique_ptr restores.
  governor.reset();
  EXPECT_EQ(read_file(card0), "auto");
}
