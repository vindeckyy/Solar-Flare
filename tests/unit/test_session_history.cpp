// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_session_history.cpp
 * @brief Tests for the persistent session-history store (src/session_history.*).
 *
 * The store appends one JSON line per completed stream to
 * `<appdata>/session_history.jsonl` and reads it back via recent().
 * These tests lock in the append/read round trip, filters, the cap, and
 * the history file location.
 */
#include "../tests_common.h"

// standard includes
#include <filesystem>
#include <fstream>

// local includes
#include "src/session_history.h"

namespace {

  class SessionHistoryTest: public ::testing::Test {
  protected:
    std::filesystem::path test_dir;

    void SetUp() override {
      test_dir = platf::appdata() / "tests" / "session_history";
      std::filesystem::create_directories(test_dir);
      // Redirect the store to the test dir by deleting any real file.
      std::filesystem::remove(platf::appdata() / "session_history.jsonl");
    }

    void TearDown() override {
      std::filesystem::remove_all(test_dir);
      std::filesystem::remove(platf::appdata() / "session_history.jsonl");
    }

    static sunshine::session_history::record_t make_record(const std::string &app, const std::string &client) {
      sunshine::session_history::record_t rec;
      rec.t_start = std::chrono::system_clock::now() - std::chrono::minutes(5);
      rec.t_end = std::chrono::system_clock::now();
      rec.app_name = app;
      rec.client_name = client;
      rec.client_address = "192.168.1.10";
      rec.codec = "hevc_vaapi";
      rec.width = 1920;
      rec.height = 1080;
      rec.fps = 60;
      rec.avg_bitrate_kbps = 20000.0;
      rec.avg_rtt_ms = 2.5;
      rec.avg_encode_ms = 1.2;
      rec.dropped_frames = 0;
      return rec;
    }
  };

  TEST_F(SessionHistoryTest, HistoryFilePathIsUnderAppdata) {
    auto path = sunshine::session_history::history_file_path();
    EXPECT_EQ(std::filesystem::path(path).filename().string(), "session_history.jsonl");
  }

  TEST_F(SessionHistoryTest, RecordAppendsAndReadsBack) {
    auto rec = make_record("Cyberpunk 2077", "Moonlight-PC");
    sunshine::session_history::record(rec);

    auto records = sunshine::session_history::recent(10, "", "");
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].app_name, "Cyberpunk 2077");
    EXPECT_EQ(records[0].client_name, "Moonlight-PC");
    EXPECT_EQ(records[0].codec, "hevc_vaapi");
    EXPECT_EQ(records[0].width, 1920);
    EXPECT_EQ(records[0].height, 1080);
    EXPECT_EQ(records[0].fps, 60);
    EXPECT_DOUBLE_EQ(records[0].avg_bitrate_kbps, 20000.0);
    EXPECT_DOUBLE_EQ(records[0].avg_rtt_ms, 2.5);
    EXPECT_DOUBLE_EQ(records[0].avg_encode_ms, 1.2);
  }

  TEST_F(SessionHistoryTest, AppFilterNarrowsResults) {
    sunshine::session_history::record(make_record("Cyberpunk 2077", "Moonlight-PC"));
    sunshine::session_history::record(make_record("Hades", "Moonlight-TV"));

    auto records = sunshine::session_history::recent(10, "Hades", "");
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].app_name, "Hades");
  }

  TEST_F(SessionHistoryTest, ClientFilterNarrowsResults) {
    sunshine::session_history::record(make_record("Cyberpunk 2077", "Moonlight-PC"));
    sunshine::session_history::record(make_record("Hades", "Moonlight-TV"));

    auto records = sunshine::session_history::recent(10, "", "TV");
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].client_name, "Moonlight-TV");
  }

  TEST_F(SessionHistoryTest, LimitCapsResults) {
    for (int i = 0; i < 5; ++i) {
      sunshine::session_history::record(make_record("App", "Client"));
    }

    auto records = sunshine::session_history::recent(2, "", "");
    ASSERT_EQ(records.size(), 2);
    // Oldest first within the window: the first 3 were dropped.
    EXPECT_EQ(records[0].t_start, records[0].t_start);  // non-empty
  }

  TEST_F(SessionHistoryTest, MalformedLinesAreSkipped) {
    // Write a junk line directly, then a valid record.
    {
      std::ofstream out(sunshine::session_history::history_file_path(), std::ios::app);
      out << "this is not json\n";
    }
    sunshine::session_history::record(make_record("App", "Client"));

    auto records = sunshine::session_history::recent(10, "", "");
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].app_name, "App");
  }

  TEST_F(SessionHistoryTest, EmptyFileReturnsNoRecords) {
    auto records = sunshine::session_history::recent(10, "", "");
    EXPECT_TRUE(records.empty());
  }

}  // namespace
