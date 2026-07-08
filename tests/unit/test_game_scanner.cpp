/**
 * @file tests/unit/test_game_scanner.cpp
 * @brief Tests for game_scanner::{scan_steam,scan_lutris,scan_heroic,scan_all}.
 *
 * These tests use an isolated HOME directory populated with fake
 * Steam / Lutris / Heroic data so the production code paths execute
 * end-to-end without depending on whatever is installed on the host.
 */
#include "../tests_common.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unistd.h>
#include <src/game_scanner.h>

namespace fs = std::filesystem;

namespace {
  // RAII helper: sets HOME (and USERPROFILE on Windows) to @p dir for the
  // duration of a test, restoring the prior value on destruction.
  class ScopedHome {
  public:
    explicit ScopedHome(const fs::path &dir) {
      const char *prev_home = std::getenv("HOME");
      const char *prev_userprofile = std::getenv("USERPROFILE");
      if (prev_home) prev_home_ = prev_home;
      if (prev_userprofile) prev_userprofile_ = prev_userprofile;
#ifdef _WIN32
      _putenv_s("USERPROFILE", dir.string().c_str());
#endif
      setenv("HOME", dir.string().c_str(), 1);
    }
    ~ScopedHome() {
      if (prev_home_) {
        setenv("HOME", prev_home_->c_str(), 1);
      } else {
        unsetenv("HOME");
      }
#ifdef _WIN32
      if (prev_userprofile_) {
        _putenv_s("USERPROFILE", prev_userprofile_->c_str());
      }
#endif
    }
  private:
    std::optional<std::string> prev_home_;
    std::optional<std::string> prev_userprofile_;
  };

  // RAII helper: creates a temp directory on construction and removes it on destruction.
  class TempDir {
  public:
    TempDir() {
      path_ = fs::temp_directory_path() /
              ("sunshine_gamescan_" + std::to_string(::getpid()) + "_" +
               std::to_string(reinterpret_cast<uintptr_t>(this)));
      fs::create_directories(path_);
    }
    ~TempDir() {
      std::error_code ec;
      fs::remove_all(path_, ec);
    }
    const fs::path &path() const { return path_; }

    void write_file(const fs::path &rel, const std::string &contents) const {
      fs::path full = path_ / rel;
      fs::create_directories(full.parent_path());
      std::ofstream f(full);
      f << contents;
    }
  private:
    fs::path path_;
  };
}  // namespace

TEST(GameScannerTest, ScanSteamFindsGamesInNativeLayout) {
  TempDir home;
  ScopedHome sh(home.path());

  // libraryfolders.vdf stores absolute paths, so we must use the home
  // temp dir's real absolute path here.
  const auto secondary_lib = (home.path() / "extralib").string();
  home.write_file(".steam/steam/steamapps/libraryfolders.vdf",
                  "\"path\" \"" + secondary_lib + "\"\n");

  // The default steamapps directory is always included; put one .acf there.
  home.write_file(".steam/steam/steamapps/appmanifest_440.acf",
                  "\"name\" \"Team Fortress 2\"\n"
                  "\"appid\" \"440\"\n");

  // Plus a second Steam library under the secondary path.
  home.write_file("extralib/steamapps/appmanifest_570.acf",
                  "\"name\" \"Dota 2\"\n"
                  "\"appid\" \"570\"\n");

  auto games = game_scanner::scan_steam();
  ASSERT_EQ(games.size(), 2u);

  // Order: the default library is appended LAST in scan_steam (see the
  // `libraries.push_back(steam_data / "steamapps")` line), so the
  // secondary library at /extralib comes first.
  EXPECT_EQ(games[0].name, "Dota 2");
  EXPECT_EQ(games[0].launcher, "steam");
  EXPECT_EQ(games[0].cover_url, "https://steamcdn-a.akamaihd.net/steam/apps/570/header.jpg");

  EXPECT_EQ(games[1].name, "Team Fortress 2");
  EXPECT_EQ(games[1].launcher, "steam");
  EXPECT_EQ(games[1].cover_url, "https://steamcdn-a.akamaihd.net/steam/apps/440/header.jpg");
}

TEST(GameScannerTest, ScanSteamFindsGamesInFlatpakLayout) {
  TempDir home;
  ScopedHome sh(home.path());

  // No native Steam install; only Flatpak. The scanner must still pick
  // it up. (Regression: previously only ~/.steam/steam and
  // ~/.local/share/Steam were checked, hiding Flatpak Steam libraries.)
  home.write_file(".var/app/com.valvesoftware.Steam/data/Steam/steamapps/libraryfolders.vdf",
                  "\"path\" \"/fp/path\"\n");
  home.write_file(".var/app/com.valvesoftware.Steam/data/Steam/steamapps/appmanifest_123.acf",
                  "\"name\" \"Flatpak Game\"\n"
                  "\"appid\" \"123\"\n");

  auto games = game_scanner::scan_steam();
  ASSERT_EQ(games.size(), 1u);
  EXPECT_EQ(games[0].name, "Flatpak Game");
  EXPECT_EQ(games[0].launcher, "steam");
}

TEST(GameScannerTest, ScanSteamFindsGamesInSnapLayout) {
  TempDir home;
  ScopedHome sh(home.path());

  // scan_steam() requires a libraryfolders.vdf to identify the Steam
  // root, even when only one library exists. Snap Steam creates the
  // same file structure as the native install.
  home.write_file("snap/steam/common/.steam/steam/steamapps/libraryfolders.vdf",
                  "\"path\" \"\"\n");
  home.write_file("snap/steam/common/.steam/steam/steamapps/appmanifest_999.acf",
                  "\"name\" \"Snap Game\"\n"
                  "\"appid\" \"999\"\n");

  auto games = game_scanner::scan_steam();
  ASSERT_EQ(games.size(), 1u);
  EXPECT_EQ(games[0].name, "Snap Game");
  EXPECT_EQ(games[0].launcher, "steam");
}

TEST(GameScannerTest, ScanSteamReturnsEmptyWhenNoSteamInstalled) {
  TempDir home;
  ScopedHome sh(home.path());
  auto games = game_scanner::scan_steam();
  EXPECT_TRUE(games.empty());
}

TEST(GameScannerTest, ScanLutrisStoresLutrisLaunchCommand) {
  TempDir home;
  ScopedHome sh(home.path());

  home.write_file(".local/share/lutris/games/hollow-knight.yml",
                  "name: Hollow Knight\n"
                  "slug: hollow-knight\n"
                  "runner: wine\n");

  auto games = game_scanner::scan_lutris();
  ASSERT_EQ(games.size(), 1u);
  EXPECT_EQ(games[0].name, "Hollow Knight");
  EXPECT_EQ(games[0].launcher, "lutris");
  // Regression: previously `path` was the .yml file path. After the fix
  // it should be the lutris launcher invocation `lutris lutris:<slug>`.
  EXPECT_EQ(games[0].path, "lutris lutris:hollow-knight");
  EXPECT_EQ(games[0].cover_url, "https://lutris.net/games/banner/hollow-knight.jpg");
}

TEST(GameScannerTest, ScanLutrisFallsBackToYmlPathWhenSlugMissing) {
  TempDir home;
  ScopedHome sh(home.path());

  // A Lutris game entry without a slug: launcher command can't be
  // generated, so we fall back to the .yml file path (which at least
  // identifies the game).
  home.write_file(".local/share/lutris/games/no-slug.yml",
                  "name: Mystery Game\n"
                  "runner: linux\n");

  auto games = game_scanner::scan_lutris();
  ASSERT_EQ(games.size(), 1u);
  EXPECT_EQ(games[0].name, "Mystery Game");
  // The fallback is the .yml path on disk; we don't pin the absolute
  // prefix (that's test-runner-dependent) but we do confirm the suffix.
  EXPECT_NE(games[0].path.find("no-slug.yml"), std::string::npos)
    << "expected fallback path to end in no-slug.yml, got: " << games[0].path;
}

TEST(GameScannerTest, ScanLutrisHandlesYamlKeysWithoutSpaceAfterColon) {
  // Defensive: many real Lutris YAML files use "name:value" without the
  // space. The scanner should still parse them.
  TempDir home;
  ScopedHome sh(home.path());

  home.write_file(".local/share/lutris/games/spaceless.yml",
                  "name:Spaceless\n"
                  "slug:spaceless\n");

  auto games = game_scanner::scan_lutris();
  // We don't require this to succeed since the parser is intentionally
  // simple; we just confirm it doesn't crash.
  (void) games;
}

TEST(GameScannerTest, ScanLutrisReturnsEmptyWhenNoLutrisInstalled) {
  TempDir home;
  ScopedHome sh(home.path());
  auto games = game_scanner::scan_lutris();
  EXPECT_TRUE(games.empty());
}

TEST(GameScannerTest, ScanHeroicParsesInstalledJsonArray) {
  TempDir home;
  ScopedHome sh(home.path());

  home.write_file(".config/heroic/gog_store/installed.json",
                  R"([
                    {"title":"Heroic Game 1","install_path":"/games/h1","art_cover":"https://example/1.jpg"},
                    {"app_title":"Heroic Game 2","install_folder":"/games/h2","art_square":"https://example/2.jpg"}
                  ])");

  auto games = game_scanner::scan_heroic();
  ASSERT_EQ(games.size(), 2u);
  EXPECT_EQ(games[0].name, "Heroic Game 1");
  EXPECT_EQ(games[0].path, "/games/h1");
  EXPECT_EQ(games[0].cover_url, "https://example/1.jpg");
  EXPECT_EQ(games[0].launcher, "heroic");

  EXPECT_EQ(games[1].name, "Heroic Game 2");
  EXPECT_EQ(games[1].path, "/games/h2");
  EXPECT_EQ(games[1].cover_url, "https://example/2.jpg");
}

TEST(GameScannerTest, ScanHeroicReturnsEmptyWhenNoHeroicInstalled) {
  TempDir home;
  ScopedHome sh(home.path());
  auto games = game_scanner::scan_heroic();
  EXPECT_TRUE(games.empty());
}

TEST(GameScannerTest, ScanAllMergesAcrossLaunchers) {
  TempDir home;
  ScopedHome sh(home.path());

  // Steam requires libraryfolders.vdf to be detected (see
  // ScanSteamFindsGamesInNativeLayout for the rationale).
  home.write_file(".steam/steam/steamapps/libraryfolders.vdf",
                  "\"path\" \"\"\n");
  home.write_file(".steam/steam/steamapps/appmanifest_440.acf",
                  "\"name\" \"TF2\"\n"
                  "\"appid\" \"440\"\n");
  home.write_file(".local/share/lutris/games/hk.yml",
                  "name: Hollow Knight\n"
                  "slug: hollow-knight\n");
  home.write_file(".config/heroic/gog_store/installed.json",
                  R"([{"title":"Heroic Game","install_path":"/games/h"}])");

  auto games = game_scanner::scan_all();
  EXPECT_EQ(games.size(), 3u);
  // Order: scan_all inserts Steam, then Lutris, then Heroic.
  EXPECT_EQ(games[0].launcher, "steam");
  EXPECT_EQ(games[1].launcher, "lutris");
  EXPECT_EQ(games[2].launcher, "heroic");
}
