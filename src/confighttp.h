// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/confighttp.h
 * @brief Declarations for the Web UI Config HTTP server.
 */
#pragma once

// standard includes
#include <filesystem>
#include <memory>
#include <string>

// lib includes
#include <nlohmann/json.hpp>
#include <Simple-Web-Server/server_https.hpp>

// local includes
#include "config.h"
#include "thread_safe.h"

#define WEB_DIR SUNSHINE_ASSETS_DIR "/web/"

namespace confighttp {
  /**
   * @brief Result of authentication. Returned by authenticate() so callers
   *        can do per-scope authorization checks. `authenticated` is the
   *        "did the request include valid creds" flag; `is_admin` is true
   *        for Basic Auth callers and STAR-scope token holders;
   *        `granted_scopes` is the set of scopes the token holds.
   */
  struct auth_result_t {
    bool authenticated = false;  ///< Did the request pass auth?
    bool is_admin = false;  ///< Basic Auth or STAR-scope token.
    std::string token_name;  ///< For Bearer: matched token's label.
    std::vector<config::api_scope_t> granted_scopes;  ///< Token scopes.
  };

  constexpr auto PORT_HTTPS = 1;

  // Type aliases for HTTPS server components
  using https_server_t = SimpleWeb::Server<SimpleWeb::HTTPS>;
  using resp_https_t = std::shared_ptr<typename SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response>;
  using req_https_t = std::shared_ptr<typename SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request>;

  /**
   * @brief Main server start function.
   * @note Spins up the HTTP/HTTPS listeners and registers all request handlers.
   *       Blocks until the server is stopped.
   */
  void start();

  /**
   * @brief Print an HTTP request to the log.
   * @param request The HTTPS request to log.
   */
  void print_req(const req_https_t &request);
  /**
   * @brief Send a JSON response with the given body.
   * @param response The HTTPS response object.
   * @param output_tree The JSON payload to serialize and send.
   */
  void send_response(const resp_https_t &response, const nlohmann::json &output_tree);
  /**
   * @brief Send a 401 Unauthorized response.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object (for logging/context).
   */
  void send_unauthorized(const resp_https_t &response, const req_https_t &request);
  /**
   * @brief Send a redirect response to @p path.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   * @param path The location to redirect to.
   */
  void send_redirect(const resp_https_t &response, const req_https_t &request, const char *path);
  /**
   * @brief Authenticate the request. Returns the full result (who you are,
   *        what scopes you hold) so callers can do scope checks.
   *        Previously this returned bool. See confighttp.cpp for the body.
   */
  auth_result_t authenticate(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Check whether an authenticated request is allowed to use `scope`.
   *        Admin (Basic Auth or STAR-scope token) always passes.
   * @param auth The result returned by authenticate() for this request.
   * @param scope The scope the endpoint requires.
   * @return true if the request is authorized for `scope`.
   */
  bool has_scope(const auth_result_t &auth, config::api_scope_t scope);
  /**
   * @brief Send a 404 Not Found response with an error message.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   * @param error_message Human-readable error message; defaults to "Not Found".
   */
  void not_found(const resp_https_t &response, const req_https_t &request, const std::string &error_message = "Not Found");
  /**
   * @brief Send a 400 Bad Request response with an error message.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   * @param error_message Human-readable error message; defaults to "Bad Request".
   */
  void bad_request(const resp_https_t &response, const req_https_t &request, const std::string &error_message = "Bad Request");
  /**
   * @brief Validate that the request's Content-Type matches an expected type.
   * @param response The HTTPS response object (used to send a 415 on mismatch).
   * @param request The HTTPS request object.
   * @param contentType The expected Content-Type string (as string_view).
   * @return true if the Content-Type matches; false otherwise (and a 415 is sent).
   */
  bool check_content_type(const resp_https_t &response, const req_https_t &request, const std::string_view &contentType);
  /**
   * @brief Generate a CSRF token for the given client session.
   * @param client_id Identifier of the client session.
   * @return The CSRF token string.
   */
  std::string generate_csrf_token(const std::string &client_id);
  /**
   * @brief Validate the CSRF token on an incoming request.
   * @param response The HTTPS response object (used to send a 403 on mismatch).
   * @param request The HTTPS request object.
   * @param client_id Identifier of the client session.
   * @return true if the token is valid; false otherwise.
   */
  bool validate_csrf_token(const resp_https_t &response, const req_https_t &request, const std::string &client_id);
  /**
   * @brief Extract the client id from the request (cookie or session).
   * @param request The HTTPS request object.
   * @return The client id string; empty if not present.
   */
  std::string get_client_id(const req_https_t &request);
  /**
   * @brief Validate that @p index is a valid app index in the current config.
   * @param response The HTTPS response object (used to send a 400 on invalid index).
   * @param request The HTTPS request object.
   * @param index The app index from the request.
   * @return true if @p index is valid; false otherwise.
   */
  bool check_app_index(const resp_https_t &response, const req_https_t &request, int index);
  /**
   * @brief Serve a static HTML page from src_assets.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   * @param html_file Path to the HTML file relative to src_assets.
   * @param require_auth Whether the page requires an authenticated session.
   * @param redirect_if_username If true and a username is set, redirect to the username-scoped URL.
   */
  void getPage(const resp_https_t &response, const req_https_t &request, const char *html_file, bool require_auth = true, bool redirect_if_username = false);
  /**
   * @brief Serve a static asset file (image, CSS, JS, etc.).
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   */
  void getAsset(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Serve a root-level Web UI static file such as @c sw.js or @c images/*.
   * @param response The HTTPS response object.
   * @param request The HTTP request object.
   */
  void getWebStatic(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Serve the PWA web app manifest.
   * @param response The HTTPS response object.
   * @param request The HTTP request object.
   */
  void getManifest(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Choose a Cache-Control value for a Web UI file path.
   * @param relative_path Path relative to @c WEB_DIR (no leading slash).
   * @return Cache-Control header value.
   */
  std::string cache_control_for_web_path(const std::string &relative_path);

  /**
   * @brief Serve the directory-browsing JSON endpoint.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   */
  void browseDirectory(const resp_https_t &response, const req_https_t &request);
  /**
   * @brief Serve the locale JSON for the client's preferred language.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   */
  void getLocale(const resp_https_t &response, const req_https_t &request);
  /**
   * @brief Issue a CSRF token to the client.
   * @param response The HTTPS response object.
   * @param request The HTTPS request object.
   */
  void getCSRFToken(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Handle the POST /api/config endpoint. Validates the JSON payload
   *        and writes the merged settings to @c config::sunshine.config_file.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @note Exposed for unit testing; the real handler is registered in @c start().
   */
  void saveConfig(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Handle the POST /api/apps endpoint.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   */
  void saveApp(const resp_https_t &response, const req_https_t &request);

  // Browse helper functions (also exposed for unit testing)
  /**
   * @brief Checks whether a directory entry qualifies as an executable file.
   * @param entry The directory entry to check.
   * @param status The cached file status for the entry.
   * @return True if the file should be included in an executable-type listing.
   */
  bool is_browsable_executable(const std::filesystem::directory_entry &entry, const std::filesystem::file_status &status);

  /**
   * @brief Lists, filters, and sorts the entries of a directory for the browse API.
   * @param dir_path The directory to list.
   * @param type_str Filter type: "directory", "executable", "file", or "any".
   * @return Sorted JSON array of entry objects with name/type/path fields.
   */
  nlohmann::json build_browse_entries(const std::filesystem::path &dir_path, const std::string &type_str);

  /**
   * @brief Validate a Moonlight client UUID string.
   * @param uuid The UUID to validate.
   * @return `true` when @p uuid matches the canonical hyphenated form.
   */
  bool is_valid_client_uuid(const std::string &uuid);

  /**
   * @brief Validate a Web UI username before persisting credentials.
   * @param username The username to validate.
   * @return `true` when @p username is non-empty, bounded, and control-character free.
   */
  bool is_valid_web_username(const std::string &username);

#ifdef _WIN32
  /**
   * @brief Builds a JSON array of available Windows drive letters.
   * @return JSON array of drive-letter entries.
   */
  nlohmann::json get_windows_drives();
#endif
}  // namespace confighttp

// mime types map
const std::map<std::string, std::string> mime_types = {
  {"css", "text/css"},
  {"gif", "image/gif"},
  {"htm", "text/html"},
  {"html", "text/html"},
  {"ico", "image/x-icon"},
  {"jpeg", "image/jpeg"},
  {"jpg", "image/jpeg"},
  {"js", "application/javascript"},
  {"json", "application/json"},
  {"png", "image/png"},
  {"svg", "image/svg+xml"},
  {"ttf", "font/ttf"},
  {"txt", "text/plain"},
  {"woff2", "font/woff2"},
  {"xml", "text/xml"},
};
