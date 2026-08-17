// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/httpcommon.h
 * @brief Declarations for common HTTP.
 */
#pragma once

// lib includes
#include <curl/curl.h>

// local includes
#include "network.h"
#include "thread_safe.h"

namespace http {

  /**
   * @brief Restrict a libcurl handle to HTTPS URLs.
   *
   * @param curl Initialized libcurl handle.
   * @return libcurl result from applying the protocol restriction.
   */
  CURLcode restrict_protocols_to_https(CURL *curl);

  /**
   * @brief Initialize HTTPS credentials and user state.
   * @return 0 on success, -1 on failure (cert missing or unreadable creds).
   */
  int init();

  /**
   * @brief Generate a self-signed cert/key pair at the given paths.
   * @param pkey Path for the private key.
   * @param cert Path for the certificate.
   * @return 0 on success, -1 on I/O or crypto failure.
   */
  int create_creds(const std::string &pkey, const std::string &cert);

  /**
   * @brief Persist username/password hash to a JSON file.
   * @param file Path to the credentials file.
   * @param username Plaintext username.
   * @param password Plaintext password (min 12 chars).
   * @param run_our_mouth Unused legacy flag.
   * @return 0 on success, -1 on validation or I/O failure.
   */
  int save_user_creds(
    const std::string &file,
    const std::string &username,
    const std::string &password,
    bool run_our_mouth = false
  );

  /**
   * @brief Reload username/hash/salt from the credentials file.
   * @param file Path to the credentials file.
   * @return 0 on success, -1 on parse failure.
   */
  int reload_user_creds(const std::string &file);

  /**
   * @brief Download a HTTPS URL to a file with TLS verification and timeout.
   * @param url Source HTTPS URL.
   * @param file Destination file path.
   * @param ssl_version Minimum SSL version (default TLS 1.2).
   * @return true on success, false on curl or I/O failure.
   */
  bool download_file(const std::string &url, const std::string &file, long ssl_version = CURL_SSLVERSION_TLSv1_2);

  /**
   * @brief URL-escape a string via libcurl.
   * @param url Raw string to escape.
   * @return Escaped string, or empty on curl failure.
   */
  std::string url_escape(const std::string &url);

  /**
   * @brief Extract the host component from a URL.
   * @param url Full URL.
   * @return Host string, or empty on parse failure.
   */
  std::string url_get_host(const std::string &url);

  extern std::string unique_id;
  extern net::net_e origin_web_ui_allowed;

}  // namespace http
