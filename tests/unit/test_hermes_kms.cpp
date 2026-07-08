/**
 * @file tests/unit/test_hermes_kms.cpp
 * @brief Tests for the Hermes-KMS probe that don't require the kernel module.
 *
 * The full capture loop needs a real /dev/dri/card* + loaded hermes_kms
 * module to exercise. These tests cover the public probe contract so that
 * regressions in the failure-propagation paths (module not loaded,
 * UAPI too old, caps missing) are caught without hardware.
 */
#include "../tests_common.h"

#include <src/platform/linux/hermes_kms.h>

// On a CI host without the hermes_kms kernel module loaded, probe_hermes_kms()
// must report module_loaded=false with a non-empty last_error and the device
// must be unavailable.
TEST(HermesKmsTest, ProbeReportsModuleNotLoaded) {
  auto status = platf::probe_hermes_kms();
  // Don't assert == false because a developer's box could have it loaded.
  if (!status.module_loaded) {
    EXPECT_FALSE(status.last_error.empty());
    EXPECT_EQ(status.card_index, -1);
    EXPECT_EQ(status.uapi_version, 0u);
  }
}

// verify_hermes_kms() must agree with probe_hermes_kms()'s view of the world.
TEST(HermesKmsTest, VerifyAgreesWithProbe) {
  auto status = platf::probe_hermes_kms();
  EXPECT_EQ(platf::verify_hermes_kms(), status.module_loaded && status.card_index >= 0);
}

// hermes_kms_display_names() must return an empty list when the module
// isn't usable, and otherwise return exactly one HERMES-1 entry.
TEST(HermesKmsTest, DisplayNamesContract) {
  auto names = platf::hermes_kms_display_names(platf::mem_type_e::system);
  auto status = platf::probe_hermes_kms();
  if (status.module_loaded && status.card_index >= 0) {
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "HERMES-1");
  } else {
    EXPECT_TRUE(names.empty());
  }
}