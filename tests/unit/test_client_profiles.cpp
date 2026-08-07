// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file tests/unit/test_client_profiles.cpp
 * @brief Tests for per-client streaming profiles (src/client_profiles.*).
 *
 * Profiles are parsed from `client_profile_*` config keys and applied on
 * top of the global video/solarflare config at launch. These tests lock in
 * the parse (load_from_config), the apply-override semantics, the reset
 * restore, and the find() lookup.
 */
#include "../tests_common.h"

// standard includes
#include <unordered_map>

// local includes
#include "src/client_profiles.h"
#include "src/config.h"

namespace {

  // Snapshot of the global config fields that apply() can overwrite.
  struct ConfigSnapshot {
    int max_bitrate;
    int hevc_mode;
    int av1_mode;
    std::string latency_mode;

    ConfigSnapshot() {
      max_bitrate = config::video.max_bitrate;
      hevc_mode = config::video.hevc_mode;
      av1_mode = config::video.av1_mode;
      latency_mode = config::solarflare.latency_mode;
    }

    void restore() {
      config::video.max_bitrate = max_bitrate;
      config::video.hevc_mode = hevc_mode;
      config::video.av1_mode = av1_mode;
      config::solarflare.latency_mode = latency_mode;
      sunshine::client_profiles::reset();
    }
  };

  class ClientProfilesTest: public ::testing::Test {
  protected:
    ConfigSnapshot snapshot;

    void SetUp() override {
      // Start from a clean profile set.
      sunshine::client_profiles::load_from_config({});
    }

    void TearDown() override {
      snapshot.restore();
    }
  };

  TEST_F(ClientProfilesTest, FindReturnsNullForUnknownClient) {
    EXPECT_EQ(sunshine::client_profiles::find("nonexistent"), nullptr);
  }

  TEST_F(ClientProfilesTest, LoadParsesProfileKeys) {
    std::unordered_map<std::string, std::string> vars {
      {"client_profile_Phone_max_bitrate", "15000"},
      {"client_profile_Phone_hevc_mode", "2"},
      {"client_profile_Phone_av1_mode", "0"},
      {"client_profile_Phone_latency_mode", "aggressive"},
    };
    sunshine::client_profiles::load_from_config(vars);

    auto profile = sunshine::client_profiles::find("Phone");
    ASSERT_NE(profile, nullptr);
    EXPECT_EQ(profile->max_bitrate, 15000);
    EXPECT_EQ(profile->hevc_mode, 2);
    EXPECT_EQ(profile->av1_mode, 0);
    EXPECT_EQ(profile->latency_mode, "aggressive");
  }

  TEST_F(ClientProfilesTest, LoadIgnoresNonProfileKeys) {
    std::unordered_map<std::string, std::string> vars {
      {"port", "47989"},
      {"client_profile_PC_max_bitrate", "30000"},
    };
    sunshine::client_profiles::load_from_config(vars);

    EXPECT_EQ(sunshine::client_profiles::find("port"), nullptr);
    EXPECT_NE(sunshine::client_profiles::find("PC"), nullptr);
  }

  TEST_F(ClientProfilesTest, LoadSkipsInvalidValues) {
    std::unordered_map<std::string, std::string> vars {
      {"client_profile_PC_max_bitrate", "not-a-number"},
      {"client_profile_PC_hevc_mode", "2"},
    };
    sunshine::client_profiles::load_from_config(vars);

    auto profile = sunshine::client_profiles::find("PC");
    ASSERT_NE(profile, nullptr);
    EXPECT_EQ(profile->max_bitrate, 0);  // invalid value ignored
    EXPECT_EQ(profile->hevc_mode, 2);
  }

  TEST_F(ClientProfilesTest, ApplyOverridesGlobalConfig) {
    std::unordered_map<std::string, std::string> vars {
      {"client_profile_PC_max_bitrate", "40000"},
      {"client_profile_PC_hevc_mode", "3"},
      {"client_profile_PC_latency_mode", "aggressive"},
    };
    sunshine::client_profiles::load_from_config(vars);

    const auto before_bitrate = config::video.max_bitrate;
    const auto before_hevc = config::video.hevc_mode;
    const auto before_latency = config::solarflare.latency_mode;

    sunshine::client_profiles::apply("PC");

    EXPECT_EQ(config::video.max_bitrate, 40000);
    EXPECT_EQ(config::video.hevc_mode, 3);
    EXPECT_EQ(config::solarflare.latency_mode, "aggressive");
    EXPECT_NE(before_bitrate, 40000);
    EXPECT_NE(before_hevc, 3);
    EXPECT_NE(before_latency, "aggressive");
  }

  TEST_F(ClientProfilesTest, ApplyUnknownClientIsNoOp) {
    std::unordered_map<std::string, std::string> vars {
      {"client_profile_PC_max_bitrate", "40000"},
    };
    sunshine::client_profiles::load_from_config(vars);

    const auto before_bitrate = config::video.max_bitrate;
    sunshine::client_profiles::apply("Other");
    EXPECT_EQ(config::video.max_bitrate, before_bitrate);
  }

  TEST_F(ClientProfilesTest, ResetRestoresGlobalConfig) {
    std::unordered_map<std::string, std::string> vars {
      {"client_profile_PC_max_bitrate", "40000"},
      {"client_profile_PC_hevc_mode", "3"},
    };
    sunshine::client_profiles::load_from_config(vars);

    const auto before_bitrate = config::video.max_bitrate;
    const auto before_hevc = config::video.hevc_mode;

    sunshine::client_profiles::apply("PC");
    EXPECT_EQ(config::video.max_bitrate, 40000);
    EXPECT_EQ(config::video.hevc_mode, 3);

    sunshine::client_profiles::reset();
    EXPECT_EQ(config::video.max_bitrate, before_bitrate);
    EXPECT_EQ(config::video.hevc_mode, before_hevc);
  }

  TEST_F(ClientProfilesTest, ResetWithoutApplyIsNoOp) {
    const auto before_bitrate = config::video.max_bitrate;
    sunshine::client_profiles::reset();
    EXPECT_EQ(config::video.max_bitrate, before_bitrate);
  }

  TEST_F(ClientProfilesTest, ApplyDoesNotTouchZeroFields) {
    std::unordered_map<std::string, std::string> vars {
      {"client_profile_PC_max_bitrate", "0"},  // 0 means "use global"
      {"client_profile_PC_hevc_mode", "2"},
    };
    sunshine::client_profiles::load_from_config(vars);

    const auto before_bitrate = config::video.max_bitrate;
    sunshine::client_profiles::apply("PC");

    EXPECT_EQ(config::video.max_bitrate, before_bitrate);  // unchanged
    EXPECT_EQ(config::video.hevc_mode, 2);  // applied
  }

}  // namespace
