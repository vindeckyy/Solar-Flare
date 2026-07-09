/**
 * @file tests/unit/test_solarflare_2c59b2e6_cherrypick.cpp
 * @brief Regression guard for the round-3 cherry-pick of
 *        2c59b2e6 fix(crypto): OpenSSL 4.x compatibility.
 *
 * Upstream commit 2c59b2e6 fixed OpenSSL 4.x compatibility issues in
 * src/crypto.cpp. Before the fix, the code accessed the
 * ASN1_BIT_STRING 'data' and 'length' fields directly; in OpenSSL 4.x
 * these are private. The fix:
 *
 *   1. Added a typedef 'using x509_name_t = util::safe_ptr<X509_NAME,
 *      &X509_NAME_free>' so X509_NAME is wrapped in an RAII smart
 *      pointer (the original code used X509_get_subject_name which
 *      returns an internal pointer that does NOT need to be freed; in
 *      OpenSSL 4.x the internal pointer is private).
 *   2. Replaced direct access to asn1->data / asn1->length with
 *      ASN1_STRING_get0_data(asn1) / ASN1_STRING_length(asn1) which
 *      are the public API.
 *   3. Replaced X509_get_subject_name with an explicit
 *      X509_NAME_new + X509_NAME_add_entry_by_txt so the X509_NAME is
 *      owned and freed properly.
 *
 * The cherry-pick was applied in round 3 with auto-merge (the fork
 * hadn't touched crypto.cpp). The upstream commit also added a new
 * tests/unit/test_crypto.cpp file with 50 lines of test coverage;
 * that file is part of the fork via the cherry-pick.
 *
 * These tests are build-time guards: if a future commit drops the
 * ASN1_STRING_get0_data / ASN1_STRING_length API calls (reverting to
 * direct field access), these tests fail with a clear error message
 * pointing at the round-3 cherry-pick so the maintainer can re-apply
 * the fix.
 */
#include "../tests_common.h"

#include "src/file_handler.h"

#include <string>

namespace {

  std::string read_file(const std::string &path) {
    return file_handler::read_file(path.c_str());
  }

  bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
  }

}  // namespace

// =============================================================================
// 1. The new ASN1_STRING_get0_data / ASN1_STRING_length API calls
//    are present in src/crypto.cpp. The pre-2c59b2e6 code accessed
//    asn1->data / asn1->length directly; the cherry-pick replaced
//    these with the public OpenSSL 4.x-compatible API.
// =============================================================================

TEST(SolarflareCryptoFixCherryPick, UsesAsn1StringPublicApi) {
  const auto content = read_file("src/crypto.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/crypto.cpp.";

  EXPECT_TRUE(contains(content, "ASN1_STRING_get0_data"))
    << "src/crypto.cpp is missing 'ASN1_STRING_get0_data(asn1)'. "
       "The 2c59b2e6 cherry-pick uses this public API instead of "
       "the direct field access 'asn1->data' (which is private in "
       "OpenSSL 4.x). Re-apply the cherry-pick.";
  EXPECT_TRUE(contains(content, "ASN1_STRING_length"))
    << "src/crypto.cpp is missing 'ASN1_STRING_length(asn1)'. "
       "The 2c59b2e6 cherry-pick uses this public API instead of "
       "the direct field access 'asn1->length' (which is private in "
       "OpenSSL 4.x). Re-apply the cherry-pick.";

  // And the OLD direct field access must be gone.
  EXPECT_FALSE(contains(content, "asn1->data"))
    << "src/crypto.cpp still has 'asn1->data' (direct field "
       "access). The 2c59b2e6 cherry-pick replaced this with "
       "ASN1_STRING_get0_data(asn1) for OpenSSL 4.x compatibility. "
       "Re-apply the cherry-pick.";
  EXPECT_FALSE(contains(content, "asn1->length"))
    << "src/crypto.cpp still has 'asn1->length' (direct field "
       "access). The 2c59b2e6 cherry-pick replaced this with "
       "ASN1_STRING_length(asn1) for OpenSSL 4.x compatibility. "
       "Re-apply the cherry-pick.";
}

// =============================================================================
// 2. The x509_name_t typedef is defined and used. The pre-2c59b2e6
//    code called X509_get_subject_name which returns an internal
//    pointer; the cherry-pick replaced this with an explicit
//    X509_NAME_new + safe_ptr wrapper.
// =============================================================================

TEST(SolarflareCryptoFixCherryPick, X509NameTypedefAndUsage) {
  const auto content = read_file("src/crypto.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/crypto.cpp.";

  EXPECT_TRUE(contains(content, "using x509_name_t = util::safe_ptr<X509_NAME, &X509_NAME_free>"))
    << "src/crypto.cpp is missing the x509_name_t typedef. "
       "The 2c59b2e6 cherry-pick added this RAII wrapper so "
       "X509_NAME is owned and freed properly (the old "
       "X509_get_subject_name returns an internal pointer that "
       "must NOT be freed). Re-apply the cherry-pick.";

  // And the new code uses X509_NAME_new + name.get() to construct
  // the X509_NAME explicitly.
  EXPECT_TRUE(contains(content, "x509_name_t name {X509_NAME_new()}"))
    << "src/crypto.cpp is missing the 'x509_name_t name "
       "{X509_NAME_new()}' construction. The 2c59b2e6 cherry-pick "
       "replaced the X509_get_subject_name call with this explicit "
       "X509_NAME_new + name.get() pattern. Re-apply the cherry-pick.";
  EXPECT_TRUE(contains(content, "X509_NAME_add_entry_by_txt(name.get()"))
    << "src/crypto.cpp's X509_NAME_add_entry_by_txt call doesn't "
       "use name.get(). The 2c59b2e6 cherry-pick changed it from "
       "passing the raw name to passing name.get() (the safe_ptr's "
       "raw pointer accessor). Re-apply the cherry-pick.";

  // The OLD X509_get_subject_name call must be gone.
  EXPECT_FALSE(contains(content, "X509_get_subject_name"))
    << "src/crypto.cpp still has the old 'X509_get_subject_name' "
       "call. The 2c59b2e6 cherry-pick replaced this with "
       "X509_NAME_new (the internal pointer returned by "
       "X509_get_subject_name is private in OpenSSL 4.x). "
       "Re-apply the cherry-pick.";
}

// =============================================================================
// 3. The cherry-pick also added a new test file tests/unit/test_crypto.cpp
//    with 50 lines of test coverage for the crypto module. This
//    file is part of the fork via the cherry-pick. If the file is
//    gone, the test coverage for crypto is missing.
// =============================================================================

TEST(SolarflareCryptoFixCherryPick, TestCryptoCppExists) {
  // file_handler::read_file returns empty string on read failure.
  const auto content = read_file("tests/unit/test_crypto.cpp");
  EXPECT_FALSE(content.empty())
    << "tests/unit/test_crypto.cpp is missing or empty. The "
       "2c59b2e6 cherry-pick added this 50-line test file to "
       "cover the crypto module. Re-apply the cherry-pick to "
       "restore it.";

  // And it should contain the actual test body.
  EXPECT_TRUE(content.find("GeneratedCredentialsExposeSubjectAndVerifySignatures") != std::string::npos)
    << "tests/unit/test_crypto.cpp is missing the "
       "GeneratedCredentialsExposeSubjectAndVerifySignatures test. "
       "This is the main test added by 2c59b2e6. If the test is "
       "missing, the cherry-pick has been partially reverted.";
}
