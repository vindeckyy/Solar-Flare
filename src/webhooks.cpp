// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/webhooks.cpp
 * @brief Implementation of outbound stream lifecycle webhooks.
 */
#include "webhooks.h"

// standard includes
#include <chrono>
#include <sstream>
#include <string>
#include <vector>

// third-party includes
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>

// local includes
#include "config.h"
#include "logging.h"
#include "utility.h"

using namespace std::literals;

namespace sunshine::webhooks {

  namespace {

    /// Callback sink for libcurl: discard the response body.
    std::size_t discard_body(char *, std::size_t size, std::size_t nmemb, void *) {
      return size * nmemb;
    }

    /// HMAC-SHA256 hex digest of @p data using @p secret.
    std::string hmac_sha256_hex(const std::string &secret, const std::string &data) {
      unsigned char digest[EVP_MAX_MD_SIZE];
      unsigned int digest_len = 0;
      HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char *>(data.data()), data.size(),
        digest, &digest_len);
      return util::hex_vec(std::string_view {(char *) digest, digest_len});
    }

  }  // namespace

  bool enabled() {
    return !config::nvhttp.webhook_urls.empty();
  }

  void notify(const std::string &event, const session_history::record_t &record) {
    if (!enabled()) {
      return;
    }

    nlohmann::json body;
    body["event"] = event;
    body["t_start"] = std::chrono::duration_cast<std::chrono::seconds>(record.t_start.time_since_epoch()).count();
    body["t_end"] = std::chrono::duration_cast<std::chrono::seconds>(record.t_end.time_since_epoch()).count();
    body["app_name"] = record.app_name;
    body["client_name"] = record.client_name;
    body["client_address"] = record.client_address;
    body["codec"] = record.codec;
    body["width"] = record.width;
    body["height"] = record.height;
    body["fps"] = record.fps;
    body["avg_bitrate_kbps"] = record.avg_bitrate_kbps;
    body["avg_rtt_ms"] = record.avg_rtt_ms;
    body["avg_encode_ms"] = record.avg_encode_ms;
    body["dropped_frames"] = record.dropped_frames;
    body["error"] = record.error;

    const auto body_str = body.dump();

    CURL *curl = curl_easy_init();
    if (!curl) {
      BOOST_LOG(warning) << "webhooks: curl init failed"sv;
      return;
    }

    // libcurl needs a writable buffer for CURLOPT_POSTFIELDS.
    auto body_copy = body_str;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!config::nvhttp.webhook_secret.empty()) {
      headers = curl_slist_append(headers,
        ("X-Solarflare-Signature: sha256=" + hmac_sha256_hex(config::nvhttp.webhook_secret, body_str)).c_str());
    }

    for (const auto &url : config::nvhttp.webhook_urls) {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_copy.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body_copy.size()));
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_body);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

      auto rc = curl_easy_perform(curl);
      if (rc != CURLE_OK) {
        BOOST_LOG(warning) << "webhooks: POST to "sv << url << " failed: "sv << curl_easy_strerror(rc);
      } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        BOOST_LOG(info) << "webhooks: POST to "sv << url << " -> HTTP "sv << http_code;
      }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

}  // namespace sunshine::webhooks
