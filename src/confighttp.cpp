// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/confighttp.cpp
 * @brief Definitions for the Web UI Config HTTP server.
 *
 * @todo Authentication, better handling of routes common to nvhttp, cleanup
 */
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

// standard includes
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>

// lib includes
#include <boost/algorithm/string.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <Simple-Web-Server/crypto.hpp>
#include <Simple-Web-Server/server_https.hpp>

#ifdef _WIN32
  #include "platform/windows/misc.h"

  #include <vector>
  #include <Windows.h>
#endif

// local includes
#include "config.h"
#include "confighttp.h"
#include "crypto.h"
#include "display_device.h"
#include "error.h"
#include "file_handler.h"
#include "game_scanner.h"
#include "globals.h"
#include "httpcommon.h"
#include "latency_stats.h"
#include "logging.h"
#include "network.h"
#include "nvhttp.h"
#include "platform/common.h"
#include "process.h"
#include "rtsp.h"
#include "session_history.h"
#include "telemetry.h"
#include "update.h"
#include "utility.h"
#include "uuid.h"

using namespace std::literals;

namespace confighttp {
  namespace fs = std::filesystem;

  using https_server_t = SimpleWeb::Server<SimpleWeb::HTTPS>;

  using args_t = SimpleWeb::CaseInsensitiveMultimap;
  using resp_https_t = std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response>;
  using req_https_t = std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request>;
  using https_handler_t = std::function<void(resp_https_t, req_https_t)>;

  enum class op_e {
    ADD,  ///< Add client
    REMOVE  ///< Remove client
  };

  // CSRF token management
  struct csrf_token_t {
    std::string token;
    std::chrono::steady_clock::time_point expiration;
  };

  // Store CSRF tokens with thread safety
  std::map<std::string, csrf_token_t, std::less<>> csrf_tokens;  // NOSONAR(cpp:S5421) - intentionally mutable global
  std::mutex csrf_tokens_mutex;  // NOSONAR(cpp:S5421) - intentionally mutable global

  // CSRF token configuration
  constexpr auto CSRF_TOKEN_SIZE = 32;  // 32 bytes = 256 bits
  constexpr auto CSRF_TOKEN_LIFETIME = std::chrono::hours(1);  // Tokens valid for 1 hour

  /**
   * @brief Check whether a string contains ASCII control characters.
   * @param value The string to inspect.
   * @return `true` when a control character is present.
   */
  bool contains_control_chars(const std::string &value) {
    return std::any_of(value.begin(), value.end(), [](const unsigned char ch) {
      return ch < 0x20 || ch == 0x7f;
    });
  }

  /**
   * @brief Log the request details.
   * @param request The HTTP request object.
   */
  void print_req(const req_https_t &request) {
    BOOST_LOG(debug) << "METHOD :: "sv << request->method;
    BOOST_LOG(debug) << "DESTINATION :: "sv << request->path;

    for (auto &[name, val] : request->header) {
      BOOST_LOG(debug) << name << " -- " << (name == "Authorization" ? "CREDENTIALS REDACTED" : val);
    }

    BOOST_LOG(debug) << " [--] "sv;

    for (auto &[name, val] : request->parse_query_string()) {
      BOOST_LOG(debug) << name << " -- " << (name == "csrf_token" ? "TOKEN REDACTED" : val);
    }

    BOOST_LOG(debug) << " [--] "sv;
  }

  /**
   * @brief Send a response.
   * @param response The HTTP response object.
   * @param output_tree The JSON tree to send.
   */
  void send_response(const resp_https_t &response, const nlohmann::json &output_tree) {
    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "application/json");
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");
    response->write(output_tree.dump(), headers);
  }

  /**
   * @brief Send a 401 Unauthorized response.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   */
  void send_unauthorized(const resp_https_t &response, const req_https_t &request) {
    auto address = net::addr_to_normalized_string(request->remote_endpoint().address());
    BOOST_LOG(info) << "Web UI: ["sv << address << "] -- not authorized"sv;

    constexpr auto code = SimpleWeb::StatusCode::client_error_unauthorized;

    nlohmann::json tree;
    tree["status_code"] = code;
    tree["status"] = false;
    tree["error"] = "Unauthorized";

    const SimpleWeb::CaseInsensitiveMultimap headers {
      {"Content-Type", "application/json"},
      {"WWW-Authenticate", R"(Basic realm="Sunshine Gamestream Host", charset="UTF-8")"},
      {"X-Frame-Options", "DENY"},
      {"Content-Security-Policy", "frame-ancestors 'none';"}
    };

    response->write(code, tree.dump(), headers);
  }

  // ponytail: M-4 brute-force defense. Token bucket keyed by source IP.
  // 10 failures refills in 30s; success resets. Single global mutex is fine
  // at single-digit req/s per IP; switch to a per-IP LRU if memory matters.
  // Success path bypasses entirely (no entry is added).
  struct rate_bucket_t {
    std::chrono::steady_clock::time_point window_start {std::chrono::steady_clock::now()};
    int failures {0};
  };

  static std::mutex g_rate_mutex;
  static std::unordered_map<std::string, rate_bucket_t> g_login_failures;
  static constexpr int LOGIN_FAIL_LIMIT = 10;
  static constexpr std::chrono::seconds LOGIN_FAIL_WINDOW {30};

  /**
   * @brief Remove expired login rate-limit buckets.
   * @param now The current monotonic time.
   */
  void prune_rate_limits(const std::chrono::steady_clock::time_point now) {
    std::erase_if(g_login_failures, [now](const auto &entry) {
      return now - entry.second.window_start > LOGIN_FAIL_WINDOW;
    });
  }

  bool rate_limit_allow(const std::string &ip) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(g_rate_mutex);
    prune_rate_limits(now);
    auto &b = g_login_failures[ip];
    if (now - b.window_start > LOGIN_FAIL_WINDOW) {
      b.window_start = now;
      b.failures = 0;
    }
    return b.failures < LOGIN_FAIL_LIMIT;
  }

  void rate_limit_record_failure(const std::string &ip) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(g_rate_mutex);
    prune_rate_limits(now);
    auto &b = g_login_failures[ip];
    if (now - b.window_start > LOGIN_FAIL_WINDOW) {
      b.window_start = now;
      b.failures = 0;
    }
    ++b.failures;
  }

  void rate_limit_reset(const std::string &ip) {
    std::lock_guard<std::mutex> lock(g_rate_mutex);
    g_login_failures.erase(ip);
  }

  /**
   * @brief Send a redirect response.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param path The path to redirect to.
   */
  void send_redirect(const resp_https_t &response, const req_https_t &request, const char *path) {
    auto address = net::addr_to_normalized_string(request->remote_endpoint().address());
    BOOST_LOG(info) << "Web UI: ["sv << address << "] -- not authorized"sv;
    const SimpleWeb::CaseInsensitiveMultimap headers {
      {"Location", path},
      {"X-Frame-Options", "DENY"},
      {"Content-Security-Policy", "frame-ancestors 'none';"}
    };
    response->write(SimpleWeb::StatusCode::redirection_temporary_redirect, headers);
  }

  /**
   * @brief Try to authenticate the request via API token (Bearer scheme).
   *
   * Tokens are stored as `<name>\t<hex_hash>\t<hex_salt>\t<scope1,scope2,...>`.
   * We SHA-256 the presented plaintext + per-token salt and compare against the
   * stored hash. STAR-scope tokens behave like admin for scope checks but
   * still report is_admin=false (audit logs can distinguish them).
   */
  auth_result_t authenticate_bearer(const req_https_t &request) {
    auth_result_t result;
    const auto auth = request->header.find("authorization");
    if (auth == request->header.end()) {
      return result;
    }

    const auto &raw = auth->second;
    constexpr std::string_view BEARER = "Bearer "sv;
    if (raw.size() < BEARER.size() || !boost::iequals(raw.substr(0, BEARER.size()), BEARER)) {
      return result;
    }

    std::string presented = raw.substr(BEARER.size());
    presented.erase(0, presented.find_first_not_of(" \t\r\n"));
    presented.erase(presented.find_last_not_of(" \t\r\n") + 1);
    if (presented.empty()) {
      return result;
    }

    for (const auto &token : config::nvhttp.api_tokens) {
      std::string composite = presented + ":" + token.salt;
      auto hashed = util::hex(crypto::hash(composite)).to_string();
      if (hashed == token.token_hash) {
        result.authenticated = true;
        result.token_name = token.name;
        result.granted_scopes = token.scopes;
        for (auto s : token.scopes) {
          if (s == config::api_scope_t::STAR) {
            result.is_admin = true;
          }
        }
        BOOST_LOG(info) << "Web UI: API token '"sv << token.name << "' authenticated request"sv;
        return result;
      }
    }
    BOOST_LOG(info) << "Web UI: bearer token did not match any configured API token"sv;
    return result;
  }

  /**
   * @brief Authenticate the user.
   * @details Tries API token (Bearer) first, then falls back to Basic Auth.
   *          Bearer auth is preferred because it lets callers scope permissions.
   */
  auth_result_t authenticate(const resp_https_t &response, const req_https_t &request) {
    auto address = net::addr_to_normalized_string(request->remote_endpoint().address());

    if (const auto ip_type = net::from_address(address); ip_type > http::origin_web_ui_allowed) {
      BOOST_LOG(info) << "Web UI: ["sv << address << "] -- denied"sv;
      response->write(SimpleWeb::StatusCode::client_error_forbidden);
      return auth_result_t {};
    }

    // ponytail: M-4 rate-limit gate before doing any hash work.
    if (!rate_limit_allow(address)) {
      BOOST_LOG(warning) << "Web UI: ["sv << address << "] -- rate limited"sv;
      response->write(SimpleWeb::StatusCode::client_error_too_many_requests);
      return auth_result_t {};
    }

    // If credentials are shown, redirect the user to a /welcome page
    if (config::sunshine.username.empty()) {
      send_redirect(response, request, "/welcome");
      return auth_result_t {};
    }

    auto fg = util::fail_guard([&]() {
      rate_limit_record_failure(address);
      send_unauthorized(response, request);
    });

    // Try Bearer token first.
    auto bearer_result = authenticate_bearer(request);
    if (bearer_result.authenticated) {
      rate_limit_reset(address);
      fg.disable();
      return bearer_result;
    }

    // Fall back to Basic Auth (admin).
    const auto auth = request->header.find("authorization");
    if (auth == request->header.end()) {
      return auth_result_t {};
    }

    const auto &rawAuth = auth->second;
    auto authData = SimpleWeb::Crypto::Base64::decode(rawAuth.substr("Basic "sv.length()));

    const auto index = static_cast<int>(authData.find(':'));
    if (index >= authData.size() - 1) {
      return auth_result_t {};
    }

    const auto username = authData.substr(0, index);
    const auto password = authData.substr(index + 1);

    if (const auto hash = util::hex(crypto::hash(password + config::sunshine.salt)).to_string(); !boost::iequals(username, config::sunshine.username) || hash != config::sunshine.password) {
      return auth_result_t {};
    }

    rate_limit_reset(address);
    fg.disable();
    auth_result_t result;
    result.authenticated = true;
    result.is_admin = true;
    return result;
  }

  /**
   * @brief Check whether an authenticated request is allowed to use `scope`.
   *        Admin (Basic Auth or STAR-scope token) always passes.
   */
  bool has_scope(const auth_result_t &auth, config::api_scope_t scope) {
    if (!auth.authenticated) {
      return false;
    }
    if (auth.is_admin) {
      return true;
    }
    for (auto s : auth.granted_scopes) {
      if (s == scope || s == config::api_scope_t::STAR) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Send a 403 Forbidden response (authenticated but lacks scope).
   */
  void send_forbidden(const resp_https_t &response, [[maybe_unused]] const req_https_t &request) {
    auto address = net::addr_to_normalized_string(request->remote_endpoint().address());
    BOOST_LOG(info) << "Web UI: ["sv << address << "] -- forbidden (lacks scope)"sv;

    constexpr auto code = SimpleWeb::StatusCode::client_error_forbidden;
    nlohmann::json tree;
    tree["status_code"] = code;
    tree["status"] = false;
    tree["error"] = "Token does not have the required scope for this endpoint";
    const SimpleWeb::CaseInsensitiveMultimap headers {
      {"Content-Type", "application/json"},
      {"X-Frame-Options", "DENY"},
      {"Content-Security-Policy", "frame-ancestors 'none';"}
    };
    response->write(code, tree.dump(), headers);
  }

  /**
   * @brief Authenticate a request and enforce a single API scope.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param scope The required API scope for this endpoint.
   * @return true when the request is authenticated and authorized.
   */
  bool require_scope(const resp_https_t &response, const req_https_t &request, config::api_scope_t scope) {
    const auto auth = authenticate(response, request);
    if (!auth.authenticated) {
      return false;
    }
    if (!has_scope(auth, scope)) {
      send_forbidden(response, request);
      return false;
    }
    return true;
  }

  /**
   * @brief Send a 404 Not Found response.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param error_message The error message to include in the response.
   */
  void not_found(const resp_https_t &response, [[maybe_unused]] const req_https_t &request, const std::string &error_message) {
    constexpr auto code = SimpleWeb::StatusCode::client_error_not_found;

    nlohmann::json tree;
    tree["status_code"] = code;
    tree["error"] = error_message;

    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "application/json");
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");

    response->write(code, tree.dump(), headers);
  }

  /**
   * @brief Send a 400 Bad Request response.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param error_message The error message to include in the response.
   */
  void bad_request(const resp_https_t &response, [[maybe_unused]] const req_https_t &request, const std::string &error_message) {
    constexpr auto code = SimpleWeb::StatusCode::client_error_bad_request;

    nlohmann::json tree;
    tree["status_code"] = code;
    tree["status"] = false;
    tree["error"] = error_message;

    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "application/json");
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");

    response->write(code, tree.dump(), headers);
  }

  /**
   * @brief Validate the request content type and send a bad request when mismatched.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param contentType The expected content type
   */
  bool check_content_type(const resp_https_t &response, const req_https_t &request, const std::string_view &contentType) {
    const auto requestContentType = request->header.find("content-type");
    if (requestContentType == request->header.end()) {
      bad_request(response, request, "Content type not provided");
      return false;
    }
    // Extract the media type part before any parameters (e.g., charset)
    std::string actualContentType = requestContentType->second;
    if (const size_t semicolonPos = actualContentType.find(';'); semicolonPos != std::string::npos) {
      actualContentType = actualContentType.substr(0, semicolonPos);
    }

    // Trim whitespace and convert to lowercase for case-insensitive comparison
    boost::algorithm::trim(actualContentType);
    boost::algorithm::to_lower(actualContentType);

    std::string expectedContentType(contentType);
    boost::algorithm::to_lower(expectedContentType);

    if (actualContentType != expectedContentType) {
      bad_request(response, request, "Content type mismatch");
      return false;
    }
    return true;
  }

  /**
   * @brief Get a unique client identifier for CSRF token management.
   * @param request The HTTP request object.
   * @return A unique identifier based on username or IP address.
   */
  std::string get_client_id(const req_https_t &request) {
    // Try to use the authenticated username as client ID
    if (const auto auth = request->header.find("authorization"); !config::sunshine.username.empty() && auth != request->header.end()) {
      if (const auto &rawAuth = auth->second; rawAuth.rfind("Basic "sv, 0) == 0) {
        auto authData = SimpleWeb::Crypto::Base64::decode(rawAuth.substr("Basic "sv.length()));
        if (const auto index = static_cast<int>(authData.find(':')); index < authData.size() - 1) {
          return authData.substr(0, index);  // Return username
        }
      }
    }

    // Fall back to IP address if no username
    return net::addr_to_normalized_string(request->remote_endpoint().address());
  }

  /**
   * @brief Generate a new CSRF token for a client.
   * @param client_id A unique identifier for the client (e.g., session ID or username).
   * @return The generated CSRF token.
   */
  std::string generate_csrf_token(const std::string &client_id) {
    // Generate a cryptographically secure random token
    std::string token = crypto::rand_alphabet(CSRF_TOKEN_SIZE);

    std::scoped_lock lock(csrf_tokens_mutex);

    // Clean up expired tokens first
    const auto now = std::chrono::steady_clock::now();
    std::erase_if(csrf_tokens, [&now](const auto &entry) {
      return entry.second.expiration < now;
    });

    // Store the token with expiration
    csrf_tokens[client_id] = csrf_token_t {
      token,
      now + CSRF_TOKEN_LIFETIME
    };

    return token;
  }

  /**
   * @brief Validate a stored CSRF token for a client against a provided token string.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param client_id A unique identifier for the client.
   * @param provided_token The token string to validate.
   * @return True if the token is valid, false otherwise.
   */
  bool validate_stored_csrf_token(const resp_https_t &response, const req_https_t &request, const std::string_view client_id, const std::string_view provided_token) {
    std::scoped_lock lock(csrf_tokens_mutex);
    const auto token_it = csrf_tokens.find(client_id);

    if (token_it == csrf_tokens.end()) {
      auto address = net::addr_to_normalized_string(request->remote_endpoint().address());
      BOOST_LOG(error) << "Web UI: ["sv << address << "] -- CSRF token validation failed: no token found for client"sv;
      bad_request(response, request, "Invalid CSRF token");
      return false;
    }

    if (const auto now = std::chrono::steady_clock::now(); token_it->second.expiration < now) {
      csrf_tokens.erase(token_it);
      auto address = net::addr_to_normalized_string(request->remote_endpoint().address());
      BOOST_LOG(error) << "Web UI: ["sv << address << "] -- CSRF token validation failed: token expired"sv;
      bad_request(response, request, "CSRF token expired");
      return false;
    }

    if (token_it->second.token != provided_token) {
      auto address = net::addr_to_normalized_string(request->remote_endpoint().address());
      BOOST_LOG(error) << "Web UI: ["sv << address << "] -- CSRF token validation failed: token mismatch"sv;
      bad_request(response, request, "Invalid CSRF token");
      return false;
    }

    return true;
  }

  bool validate_csrf_token(const resp_https_t &response, const req_https_t &request, const std::string &client_id) {
    // Helper function to check if a URL starts with any allowed origin
    auto is_allowed_origin = [](const std::string_view url) {
      return std::ranges::any_of(config::sunshine.csrf_allowed_origins, [&url](const std::string &allowed_origin) {
        // Ensure exact prefix match (with ":" or "/" after to prevent malicious.com matching allowed.com)
        if (url.rfind(allowed_origin, 0) != 0) {  // rfind with pos=0 checks if the url starts with allowed_origin
          return false;
        }
        // Check that it's followed by ":" (port) or "/" (path) or is an exact match
        const size_t len = allowed_origin.length();
        return url.length() == len || url[len] == ':' || url[len] == '/';
      });
    };

    // Check if the request is from the same origin (Origin or Referer header matches configured allowed origins)
    const auto origin_it = request->header.find("Origin");
    if (origin_it != request->header.end() && is_allowed_origin(origin_it->second)) {
      // Same origin request - allow without CSRF token
      return true;
    }

    // If we have a Referer header, check if it's same-origin
    const auto referer_it = request->header.find("Referer");
    if (referer_it != request->header.end() && is_allowed_origin(referer_it->second)) {
      // Same origin request - allow without CSRF token
      return true;
    }

    // If neither Origin nor Referer is present, this cannot be a browser-initiated CSRF attack.
    // Non-browser clients (e.g. curl, scripts) never send these headers, and a malicious web page
    // cannot cause a non-browser client to make requests on a user's behalf.
    if (origin_it == request->header.end() && referer_it == request->header.end()) {
      return true;
    }

    // A browser-like request arrived with an Origin/Referer that doesn't match an allowed origin.
    // Require a CSRF token.
    const std::string_view blocked_origin = (origin_it != request->header.end()) ? origin_it->second : referer_it->second;
    // Extract token from X-CSRF-Token header
    const auto header_it = request->header.find("X-CSRF-Token");
    if (header_it == request->header.end()) {
      // Also check query parameters as fallback
      auto query_params = request->parse_query_string();
      const auto query_it = query_params.find("csrf_token");
      if (query_it == query_params.end()) {
        auto address = net::addr_to_normalized_string(request->remote_endpoint().address());
        BOOST_LOG(error) << "Web UI: ["sv << address << "] -- CSRF protection blocked request from origin: "sv << blocked_origin;
        BOOST_LOG(error) << "Web UI: To allow this origin, add it to the 'csrf_allowed_origins' option in your Sunshine configuration"sv;
        bad_request(response, request, "Missing CSRF token");
        return false;
      }

      return validate_stored_csrf_token(response, request, client_id, query_it->second);
    }

    // Validate token from header
    return validate_stored_csrf_token(response, request, client_id, header_it->second);
  }

  /**
   * @brief Validates the application index and sends an error response if invalid.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param index The application index/id.
   */
  bool check_app_index(const resp_https_t &response, const req_https_t &request, int index) {
    std::string file = file_handler::read_file(config::stream.file_apps.c_str());
    nlohmann::json file_tree = nlohmann::json::parse(file);
    if (const auto &apps = file_tree["apps"]; index < 0 || index >= static_cast<int>(apps.size())) {
      std::string error;
      if (const int max_index = static_cast<int>(apps.size()) - 1; max_index < 0) {
        error = "No applications found";
      } else {
        error = std::format("'index' {} out of range, max index is {}", index, max_index);
      }
      bad_request(response, request, error);
      return false;
    }
    return true;
  }

  /**
   * @brief Get an HTML page.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @param html_file The HTML file to serve (relative to WEB_DIR).
   * @param require_auth Whether to require authentication (default: true).
   * @param redirect_if_username If true, redirect to "/" when the username is set (for welcome page).
   */
  void getPage(const resp_https_t &response, const req_https_t &request, const char *html_file, const bool require_auth, const bool redirect_if_username) {
    // Special handling for welcome page: redirect if the username is already set
    if (redirect_if_username && !config::sunshine.username.empty()) {
      send_redirect(response, request, "/");
      return;
    }

    if (require_auth && !authenticate(response, request).authenticated) {
      return;
    }

    print_req(request);

    const std::string content = file_handler::read_file((std::string(WEB_DIR) + html_file).c_str());
    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "text/html; charset=utf-8");

    // prevent click jacking
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");

    response->write(content, headers);
  }

  /**
   * @brief Get the favicon image.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @todo combine function with getSunshineLogoImage and possibly getNodeModules
   * @todo use mime_types map
   */
  void getFaviconImage(const resp_https_t &response, const req_https_t &request) {
    print_req(request);

    std::ifstream in(WEB_DIR "images/sunshine.ico", std::ios::binary);
    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "image/x-icon");
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");
    response->write(SimpleWeb::StatusCode::success_ok, in, headers);
  }

  /**
   * @brief Get the Sunshine logo image.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @todo combine function with getFaviconImage and possibly getNodeModules
   * @todo use mime_types map
   */
  void getSunshineLogoImage(const resp_https_t &response, const req_https_t &request) {
    print_req(request);

    std::ifstream in(WEB_DIR "images/logo-sunshine-45.png", std::ios::binary);
    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "image/png");
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");
    response->write(SimpleWeb::StatusCode::success_ok, in, headers);
  }

  /**
   * @brief Serve the PWA web app manifest.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   */
  void getManifest(const resp_https_t &response, const req_https_t &request) {
    print_req(request);

    std::ifstream in(WEB_DIR "manifest.webmanifest");
    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "application/manifest+json");
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");
    response->write(SimpleWeb::StatusCode::success_ok, in, headers);
  }

  /**
   * @brief Check if a path is a child of another path.
   * @param base The base path.
   * @param query The path to check.
   * @return True if the path is a child of the base path, false otherwise.
   */
  bool isChildPath(fs::path const &base, fs::path const &query) {
    auto relPath = fs::relative(base, query);
    return *(relPath.begin()) != fs::path("..");
  }

  /**
   * @brief Get an asset.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   */
  void getAsset(const resp_https_t &response, const req_https_t &request) {
    print_req(request);
    fs::path webDirPath(WEB_DIR);
    fs::path nodeModulesPath(webDirPath / "assets");

    // .relative_path is needed to shed any leading slash that might exist in the request path
    auto filePath = fs::weakly_canonical(webDirPath / fs::path(request->path).relative_path());

    // Don't do anything if the file does not exist or is outside the assets directory
    if (!isChildPath(filePath, nodeModulesPath)) {
      BOOST_LOG(warning) << "Someone requested a path " << filePath << " that is outside the assets folder";
      bad_request(response, request);
      return;
    }
    if (!fs::exists(filePath)) {
      not_found(response, request);
      return;
    }

    auto relPath = fs::relative(filePath, webDirPath);
    // get the mime type from the file extension mime_types map
    // remove the leading period from the extension
    auto mimeType = mime_types.find(relPath.extension().string().substr(1));
    // check if the extension is in the map at the x position
    if (mimeType == mime_types.end()) {
      bad_request(response, request);
      return;
    }

    // if it is, set the content type to the mime type
    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", mimeType->second);
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");
    std::ifstream in(filePath.string(), std::ios::binary);
    response->write(SimpleWeb::StatusCode::success_ok, in, headers);
  }

  /**
   * @brief Get a CSRF token for the authenticated user.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/csrf-token| GET| null}
   */
  void getCSRFToken(const resp_https_t &response, const req_https_t &request) {
    if (!authenticate(response, request).authenticated) {
      return;
    }

    print_req(request);

    std::string client_id = get_client_id(request);
    std::string token = generate_csrf_token(client_id);

    nlohmann::json output_tree;
    output_tree["csrf_token"] = token;
    send_response(response, output_tree);
  }

  /**
   * @brief Get the list of available applications.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/apps| GET| null}
   */
  void getApps(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::APPS_GET)) {
      return;
    }

    print_req(request);

    try {
      std::string content = file_handler::read_file(config::stream.file_apps.c_str());
      nlohmann::json file_tree = nlohmann::json::parse(content);

      // Legacy versions of Sunshine used strings for boolean and integers, let's convert them
      // List of keys to convert to boolean
      const std::vector<std::string> boolean_keys = {
        "exclude-global-prep-cmd",
        "elevated",
        "auto-detach",
        "wait-all"
      };

      // List of keys to convert to integers
      std::vector<std::string> integer_keys = {
        "exit-timeout"
      };

      // Walk fileTree and convert true/false strings to boolean or integer values
      for (auto &app : file_tree["apps"]) {
        for (const auto &key : boolean_keys) {
          if (app.contains(key) && app[key].is_string()) {
            app[key] = app[key] == "true";
          }
        }
        for (const auto &key : integer_keys) {
          if (app.contains(key) && app[key].is_string()) {
            app[key] = std::stoi(app[key].get<std::string>());
          }
        }
        if (app.contains("prep-cmd")) {
          for (auto &prep : app["prep-cmd"]) {
            if (prep.contains("elevated") && prep["elevated"].is_string()) {
              prep["elevated"] = prep["elevated"] == "true";
            }
          }
        }
      }

      send_response(response, file_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "GetApps: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Scan for installed games from Steam, Lutris, and Heroic.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/games/scan| GET| null}
   */
  void scanGames(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::APPS_GET)) {
      return;
    }

    print_req(request);

    try {
      auto discovered = game_scanner::scan_all();
      nlohmann::json tree = nlohmann::json::array();
      for (const auto &g : discovered) {
        nlohmann::json entry;
        entry["name"] = g.name;
        entry["path"] = g.path;
        entry["launcher"] = g.launcher;
        entry["cover_url"] = g.cover_url;
        tree.push_back(entry);
      }
      send_response(response, tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "ScanGames: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Save an application. To save a new application, the index must be `-1`. To update an existing application, you must provide the current index of the application.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * The body for the post request should be JSON serialized in the following format:
   * @code{.json}
   * {
   *   "name": "Application Name",
   *   "output": "Log Output Path",
   *   "cmd": "Command to run the application",
   *   "index": -1,
   *   "exclude-global-prep-cmd": false,
   *   "elevated": false,
   *   "auto-detach": true,
   *   "wait-all": true,
   *   "exit-timeout": 5,
   *   "prep-cmd": [
   *     {
   *       "do": "Command to prepare",
   *       "undo": "Command to undo preparation",
   *       "elevated": false
   *     }
   *   ],
   *   "detached": [
   *     "Detached command"
   *   ],
   *   "image-path": "Full path to the application image. Must be a png file."
   * }
   * @endcode
   *
   * @api_examples{/api/apps| POST| {"name":"Hello, World!","index":-1}}
   */
  void saveApp(const resp_https_t &response, const req_https_t &request) {
    if (!check_content_type(response, request, "application/json")) {
      return;
    }
    if (!require_scope(response, request, config::api_scope_t::CONFIG_SET)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    std::stringstream ss;
    ss << request->content.rdbuf();
    try {
      nlohmann::json output_tree;
      nlohmann::json input_tree = nlohmann::json::parse(ss);
      if (!input_tree.is_object() || !input_tree.contains("index") || !input_tree["index"].is_number_integer()) {
        throw std::invalid_argument("'index' must be an integer");
      }
      if (!input_tree.contains("name") || !input_tree["name"].is_string()) {
        throw std::invalid_argument("'name' must be a string");
      }
      std::string file = file_handler::read_file(config::stream.file_apps.c_str());
      BOOST_LOG(info) << file;
      nlohmann::json file_tree = nlohmann::json::parse(file);

      if (input_tree["prep-cmd"].empty()) {
        input_tree.erase("prep-cmd");
      }

      if (input_tree["detached"].empty()) {
        input_tree.erase("detached");
      }

      auto &apps_node = file_tree["apps"];
      const int index = input_tree["index"].get<int>();
      if (index < -1 || index >= static_cast<int>(apps_node.size())) {
        throw std::out_of_range("'index' is out of range");
      }

      input_tree.erase("index");
      if (index == -1) {
        apps_node.push_back(std::move(input_tree));
      } else {
        apps_node[index] = std::move(input_tree);
      }

      std::sort(apps_node.begin(), apps_node.end(), [](const nlohmann::json &a, const nlohmann::json &b) {
        return a.at("name").get<std::string>() < b.at("name").get<std::string>();
      });

      file_handler::write_file(config::stream.file_apps.c_str(), file_tree.dump(4));
      proc::refresh(config::stream.file_apps);

      output_tree["status"] = true;
      send_response(response, output_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "SaveApp: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Close the currently running application.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/apps/close| POST| null}
   */
  void closeApp(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::APPS_CLOSE)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    proc::proc.terminate();

    nlohmann::json output_tree;
    output_tree["status"] = true;
    send_response(response, output_tree);
  }

  /**
   * @brief Delete an application.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/apps/9999| DELETE| null}
   */
  void deleteApp(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CONFIG_SET)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    try {
      nlohmann::json output_tree;
      nlohmann::json new_apps = nlohmann::json::array();
      const int index = std::stoi(request->path_match[1]);

      if (!check_app_index(response, request, index)) {
        return;
      }

      std::string file = file_handler::read_file(config::stream.file_apps.c_str());
      nlohmann::json file_tree = nlohmann::json::parse(file);
      auto &apps = file_tree["apps"];

      for (size_t i = 0; i < apps.size(); ++i) {
        if (i != index) {
          new_apps.push_back(apps[i]);
        }
      }
      file_tree["apps"] = new_apps;

      file_handler::write_file(config::stream.file_apps.c_str(), file_tree.dump(4));
      proc::refresh(config::stream.file_apps);

      output_tree["status"] = true;
      output_tree["result"] = std::format("application {} deleted", index);
      send_response(response, output_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "DeleteApp: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Get the list of paired clients.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/clients/list| GET| null}
   */
  void getClients(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CLIENTS_LIST)) {
      return;
    }

    print_req(request);

    const nlohmann::json named_certs = nvhttp::get_all_clients();

    nlohmann::json output_tree;
    output_tree["named_certs"] = named_certs;
    output_tree["status"] = true;
    send_response(response, output_tree);
  }

  /**
   * @brief Enable or disable a client.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * The body for the POST request should be JSON serialized in the following format:
   * @code{.json}
   * {
   *   "uuid": "<uuid>",
   *   "enabled": true
   * }
   * @endcode
   *
   * @api_examples{/api/clients/update| POST| {"uuid":"<uuid>","enabled":true}}
   */
  void updateClient(resp_https_t response, req_https_t request) {
    if (!check_content_type(response, request, "application/json")) {
      return;
    }
    if (!require_scope(response, request, config::api_scope_t::STAR)) {
      return;
    }
    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    std::stringstream ss;
    ss << request->content.rdbuf();
    try {
      nlohmann::json input_tree = nlohmann::json::parse(ss.str());
      nlohmann::json output_tree;
      std::string uuid = input_tree.value("uuid", "");
      if (!is_valid_client_uuid(uuid)) {
        bad_request(response, request, "Invalid UUID");
        return;
      }
      bool enabled = input_tree.value("enabled", true);
      output_tree["status"] = nvhttp::set_client_enabled(uuid, enabled);

      if (!enabled && output_tree["status"]) {
        auto cert = nvhttp::get_cert_by_uuid(uuid);
        if (!cert.empty()) {
          rtsp_stream::terminate_sessions_by_cert(cert);
        }

        if (rtsp_stream::session_count() == 0 && proc::proc.running() > 0) {
          proc::proc.terminate();
        }
      }

      send_response(response, output_tree);
    } catch (nlohmann::json::exception &e) {
      BOOST_LOG(warning) << "Update Client: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Unpair a client.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * The body for the POST request should be JSON serialized in the following format:
   * @code{.json}
   * {
   *  "uuid": "<uuid>"
   * }
   * @endcode
   *
   * @api_examples{/api/unpair| POST| {"uuid":"1234"}}
   */
  void unpair(const resp_https_t &response, const req_https_t &request) {
    if (!check_content_type(response, request, "application/json")) {
      return;
    }
    if (!require_scope(response, request, config::api_scope_t::CLIENTS_UNPAIR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    std::stringstream ss;
    ss << request->content.rdbuf();

    try {
      nlohmann::json output_tree;
      const nlohmann::json input_tree = nlohmann::json::parse(ss);
      const std::string uuid = input_tree.value("uuid", "");
      if (!is_valid_client_uuid(uuid)) {
        bad_request(response, request, "Invalid UUID");
        return;
      }
      const bool removed = nvhttp::unpair_client(uuid);
      output_tree["status"] = removed;

      if (removed && nvhttp::get_all_clients().empty()) {
        proc::proc.terminate();
      }

      send_response(response, output_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "Unpair: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Unpair all clients.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/clients/unpair-all| POST| null}
   */
  void unpairAll(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CLIENTS_UNPAIR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    nvhttp::erase_all_clients();
    proc::proc.terminate();

    nlohmann::json output_tree;
    output_tree["status"] = true;
    send_response(response, output_tree);
  }

  /**
   * @brief Get the configuration settings.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/config| GET| null}
   */
  void getConfig(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CONFIG_GET)) {
      return;
    }

    print_req(request);

    nlohmann::json output_tree;
    output_tree["status"] = true;
    output_tree["platform"] = SUNSHINE_PLATFORM;
    output_tree["version"] = PROJECT_VERSION;

    auto vars = config::parse_config(file_handler::read_file(config::sunshine.config_file.c_str()));

    for (auto &[name, value] : vars) {
      output_tree[name] = std::move(value);
    }

    // Emit SolarFlare audio_fx defaults so the Web UI sees the controls
    // on first load (before the user has saved anything). User-saved
    // values in `vars` already won the loop above.
    const auto &fx = config::solarflare.audio_fx;
    auto bool_or_default = [&](const char *name, bool val) {
      if (vars.find(name) == vars.end()) {
        output_tree[name] = val ? "enabled" : "disabled";
      }
    };
    auto num_or_default = [&](const char *name, int val) {
      if (vars.find(name) == vars.end()) {
        output_tree[name] = val;
      }
    };
    auto float_or_default = [&](const char *name, float val) {
      if (vars.find(name) == vars.end()) {
        output_tree[name] = val;
      }
    };
    bool_or_default("sf_audio_agc", fx.enable_agc);
    bool_or_default("sf_audio_vad", fx.enable_vad);
    bool_or_default("sf_audio_ducking", fx.enable_ducking);
    bool_or_default("sf_audio_noise_gate", fx.enable_noise_gate);
    bool_or_default("sf_opus_fec", fx.opus_fec);
    bool_or_default("sf_opus_bandwidth_extension", fx.opus_bandwidth_extension);
    float_or_default("sf_audio_noise_gate_db", fx.noise_gate_threshold_db);
    float_or_default("sf_audio_agc_target_db", fx.agc_target_rms_db);
    float_or_default("sf_audio_agc_max_gain_db", fx.agc_max_gain_db);
    float_or_default("sf_audio_agc_min_gain_db", fx.agc_min_gain_db);
    float_or_default("sf_audio_agc_attack_ms", fx.agc_attack_ms);
    float_or_default("sf_audio_agc_hold_ms", fx.agc_hold_ms);
    float_or_default("sf_audio_agc_release_ms", fx.agc_release_ms);
    float_or_default("sf_audio_vad_threshold_db", fx.vad_threshold_db);
    float_or_default("sf_audio_vad_hysteresis_db", fx.vad_hysteresis_db);
    float_or_default("sf_audio_vad_min_speech_ms", fx.vad_min_speech_ms);
    float_or_default("sf_audio_vad_min_silence_ms", fx.vad_min_silence_ms);
    float_or_default("sf_audio_ducker_attenuation_db", fx.ducker_target_attenuation_db);
    float_or_default("sf_audio_ducker_attack_ms", fx.ducker_attack_ms);
    float_or_default("sf_audio_ducker_release_ms", fx.ducker_release_ms);
    num_or_default("sf_opus_application", fx.opus_application);
    num_or_default("sf_opus_vbr", fx.opus_vbr);
    num_or_default("sf_opus_complexity", fx.opus_complexity);
    num_or_default("sf_opus_expected_loss_pct", fx.opus_expected_loss_pct);

    // Emit defaults for headless stream options so the Web UI sees the controls
    // on first load before the user has saved anything.
    bool_or_default("headless_mode", config::video.linux_display.headless_mode);
    bool_or_default("linux_use_cage_compositor", config::video.linux_display.use_cage_compositor);
    bool_or_default("linux_prefer_gpu_native_capture", config::video.linux_display.prefer_gpu_native_capture);
    if (vars.find("compositor_backend") == vars.end()) {
      output_tree["compositor_backend"] = config::video.linux_display.compositor_backend;
    }

    // Emit defaults for adaptive bitrate options.
    bool_or_default("adaptive_bitrate_enabled", config::video.adaptive_bitrate_enabled);
    num_or_default("adaptive_bitrate_min", config::video.adaptive_bitrate_min);
    num_or_default("adaptive_bitrate_max", config::video.adaptive_bitrate_max);

    // Emit defaults for trusted subnet options.
    bool_or_default("trusted_subnet_auto_pairing", config::nvhttp.trusted_subnet_auto_pairing);
    if (vars.find("trusted_subnets") == vars.end()) {
      output_tree["trusted_subnets"] = config::nvhttp.trusted_subnets;
    }

    send_response(response, output_tree);
  }

  /**
   * @brief Get the locale setting. This endpoint does not require authentication.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/configLocale| GET| null}
   */
  void getLocale(const resp_https_t &response, const req_https_t &request) {
    // we need to return the locale whether authenticated or not

    print_req(request);

    nlohmann::json output_tree;
    output_tree["status"] = true;
    output_tree["locale"] = config::sunshine.locale;
    send_response(response, output_tree);
  }

  /**
   * @brief Save the configuration settings.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * The body for the POST request should be JSON serialized in the following format:
   * @code{.json}
   * {
   *   "key": "value"
   * }
   * @endcode
   *
   * @attention{It is recommended to ONLY save the config settings that differ from the default behavior.}
   *
   * @api_examples{/api/config| POST| {"key":"value"}}
   */
  void saveConfig(const resp_https_t &response, const req_https_t &request) {
    if (!check_content_type(response, request, "application/json")) {
      return;
    }
    if (!require_scope(response, request, config::api_scope_t::CONFIG_SET)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    std::stringstream ss;
    ss << request->content.rdbuf();
    try {
      nlohmann::json output_tree;
      nlohmann::json input_tree = nlohmann::json::parse(ss);

      // Reject non-object payloads (arrays, scalars, null). Iterating those as
      // if they were objects silently corrupts the config file with synthetic
      // numeric keys (e.g. "0 = a\n1 = b") or spurious "<number> = null"
      // lines, then reports success. Surface the rejection as a 400 so the
      // client can correct the request instead of corrupting the file.
      if (!input_tree.is_object()) {
        BOOST_LOG(warning) << "SaveConfig: rejected non-object payload of type "sv
                           << input_tree.type_name();
        bad_request(response, request, "Request body must be a JSON object");
        return;
      }

      std::stringstream config_stream;
      for (const auto &[k, v] : input_tree.items()) {
        if (v.is_null() || (v.is_string() && v.get<std::string>().empty())) {
          continue;
        }

        // v.dump() will dump valid json, which we do not want for strings in the config, right now
        // we should migrate the config file to straight JSON and get rid of all this nonsense
        config_stream << k << " = " << (v.is_string() ? v.get<std::string>() : v.dump()) << std::endl;
      }

      // Refuse to write an empty payload. A zero-byte config_stream would
      // truncate the on-disk config file to zero bytes via
      // file_handler::write_file, silently wiping every previously saved
      // setting (the upstream web UI sends {} when the user clicks "Save"
      // with no settings differing from defaults; payloads of all-null
      // entries also produce an empty stream). Surface as a 400 instead of
      // silently destroying the user's config.
      if (config_stream.tellp() == std::streampos {0}) {
        BOOST_LOG(warning) << "SaveConfig: rejected empty payload (would wipe existing config)"sv;
        bad_request(response, request, "Refusing to save an empty config: at least one setting must differ from defaults");
        return;
      }

      // file_handler::write_file returns -1 if the file cannot be opened or
      // written (permission denied, read-only filesystem, disk full, parent
      // directory missing, etc.). Surface the failure as a JSON 500-equivalent
      // body so the client and logs reflect that the settings did NOT
      // persist. Returning {"status": true} on a failed write is what
      // produced the user-visible "config save not working" symptom -- the
      // request looked successful while the on-disk file was left untouched.
      const std::string contents = config_stream.str();
      if (file_handler::write_file(config::sunshine.config_file.c_str(), contents) != 0) {
        BOOST_LOG(error) << "SaveConfig: failed to write config file "sv
                         << config::sunshine.config_file;
        output_tree["status"] = false;
        output_tree["error"] = "Failed to write config file to disk";
        send_response(response, output_tree);
        return;
      }
      output_tree["status"] = true;
      send_response(response, output_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "SaveConfig: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Get an application's image.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @note{The index in the url path is the application index.}
   *
   * @api_examples{/api/covers/9999 | GET| null}
   */
  void getCover(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::APPS_GET)) {
      return;
    }

    print_req(request);

    try {
      const int index = std::stoi(request->path_match[1]);
      if (!check_app_index(response, request, index)) {
        return;
      }

      std::string file = file_handler::read_file(config::stream.file_apps.c_str());
      nlohmann::json file_tree = nlohmann::json::parse(file);
      auto &apps = file_tree["apps"];

      auto &app = apps[index];

      // Get the image path from the app configuration
      std::string app_image_path;
      if (app.contains("image-path") && !app["image-path"].is_null()) {
        app_image_path = app["image-path"];
      }

      // Use validate_app_image_path to resolve and validate the path
      // This handles extension validation, PNG signature validation, and path resolution
      std::string validated_path = proc::validate_app_image_path(app_image_path);

      // Check if we got the default image path (means validation failed or no image configured)
      if (validated_path == DEFAULT_APP_IMAGE_PATH) {
        BOOST_LOG(debug) << "Application at index " << index << " does not have a valid cover image";
        not_found(response, request, "Cover image not found");
        return;
      }

      // Open and stream the validated file
      std::ifstream in(validated_path, std::ios::binary);
      if (!in) {
        BOOST_LOG(warning) << "Unable to read cover image file: " << validated_path;
        bad_request(response, request, "Unable to read cover image file");
        return;
      }

      SimpleWeb::CaseInsensitiveMultimap headers;
      headers.emplace("Content-Type", "image/png");
      headers.emplace("X-Frame-Options", "DENY");
      headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");

      response->write(SimpleWeb::StatusCode::success_ok, in, headers);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "GetCover: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Upload a cover image.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * The body for the post request should be JSON serialized in the following format:
   * @code{.json}
   * {
   *   "key": "igdb_<game_id>",
   *   "url": "https://images.igdb.com/igdb/image/upload/t_cover_big_2x/<slug>.png"
   * }
   * @endcode
   *
   * @api_examples{/api/covers/upload| POST| {"key":"igdb_1234","url":"https://images.igdb.com/igdb/image/upload/t_cover_big_2x/abc123.png"}}
   */
  void uploadCover(const resp_https_t &response, const req_https_t &request) {
    if (!check_content_type(response, request, "application/json")) {
      return;
    }
    if (!require_scope(response, request, config::api_scope_t::CONFIG_SET)) {
      return;
    }

    std::stringstream ss;
    ss << request->content.rdbuf();
    try {
      nlohmann::json output_tree;
      nlohmann::json input_tree = nlohmann::json::parse(ss);

      std::string key = input_tree.value("key", "");
      if (key.empty() || key.find('/') != std::string::npos || key.find("..") != std::string::npos || key.find('\0') != std::string::npos) {  // ponytail: L-1 path traversal guard
        bad_request(response, request, "Invalid cover key");
        return;
      }
      std::string url = input_tree.value("url", "");

      const std::string coverdir = platf::appdata().string() + "/covers/";
      file_handler::make_directory(coverdir);

      std::basic_string path = coverdir + http::url_escape(key) + ".png";
      if (!url.empty()) {
        if (http::url_get_host(url) != "images.igdb.com") {
          bad_request(response, request, "Only images.igdb.com is allowed");
          return;
        }
        if (!http::download_file(url, path)) {
          bad_request(response, request, "Failed to download cover");
          return;
        }
      } else {
        auto data = SimpleWeb::Crypto::Base64::decode(input_tree.value("data", ""));

        std::ofstream imgfile(path);
        imgfile.write(data.data(), static_cast<int>(data.size()));
      }
      output_tree["status"] = true;
      output_tree["path"] = path;
      send_response(response, output_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "UploadCover: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Get the logs from the log file.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/logs| GET| null}
   */
  /**
   * @brief One freshly-minted API token, returned from mint_api_token().
   * @details The plaintext is shown to the admin exactly once (in the
   *          POST /api/tokens response). It is not stored anywhere; only
   *          the SHA-256 hash + per-token salt land in the config.
   */
  struct minted_token_t {
    std::string plaintext;  ///< The 64-char hex plaintext to display once.
    std::string hash;  ///< Hex SHA-256 of `plaintext:salt`.
    std::string salt;  ///< Per-token random salt, hex-encoded.
  };

  /**
   * @brief Generate a fresh random API token.
   * @details Reads 32 bytes of randomness from /dev/urandom for the plaintext
   *          and 16 bytes for the per-token salt. Computes SHA-256(plaintext:salt)
   *          as the stored hash.
   * @return A minted_token_t with the plaintext, hash, and salt.
   */
  minted_token_t mint_api_token();

  // Helper: hex-encode a raw byte string.
  std::string hex_encode(const std::string &raw) {
    static const char *hex = "0123456789abcdef";
    std::string out;
    out.reserve(raw.size() * 2);
    for (unsigned char c : raw) {
      out.push_back(hex[(c >> 4) & 0xF]);
      out.push_back(hex[c & 0xF]);
    }
    return out;
  }

  minted_token_t mint_api_token() {
    minted_token_t out;
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    std::string plain_raw(32, '\0');
    urandom.read(plain_raw.data(), 32);
    std::string salt_raw(16, '\0');
    urandom.read(salt_raw.data(), 16);
    out.plaintext = hex_encode(plain_raw);
    out.salt = hex_encode(salt_raw);
    std::string composite = out.plaintext + ":" + out.salt;
    out.hash = util::hex(crypto::hash(composite)).to_string();
    return out;
  }

  /**
   * @brief Push network stats from an HTTP client into the AdaptiveBitrate
   *        controller. The video loop drains the queue once per frame.
   *
   * Body: {"packet_loss_pct": 0.5, "rtt_ms": 23.4}
   *
   * @api_examples{/api/stream/network-stats| POST| {"packet_loss_pct":0.5,"rtt_ms":23.4}}
   */
  void postNetworkStats(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::LOGS_GET)) {
      return;
    }
    print_req(request);

    std::stringstream ss;
    ss << request->content.rdbuf();
    try {
      auto j = nlohmann::json::parse(ss.str());
      float loss = j.value("packet_loss_pct", 0.0f);
      float rtt = j.value("rtt_ms", 0.0f);
      if (loss < 0 || loss > 100) {
        bad_request(response, request, "packet_loss_pct must be in [0, 100]");
        return;
      }
      if (rtt < 0) {
        bad_request(response, request, "rtt_ms must be >= 0");
        return;
      }
      // Push into the same queue the video loop drains.
      mail::man->event<std::pair<float, float>>(mail::adaptive_bitrate_net_stats)->raise(std::make_pair(loss, rtt));
    } catch (const std::exception &e) {
      bad_request(response, request, std::string("Invalid JSON: ") + e.what());
      return;
    }

    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    send_response(response, tree);
  }

  /**
   * @brief Return current adaptive-bitrate state. Read-only.
   *
   * @api_examples{/api/stream/bitrate| GET| null}
   */
  void getBitrate(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CONFIG_GET)) {
      return;
    }
    print_req(request);

    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    tree["adaptive_bitrate_enabled"] = config::video.adaptive_bitrate_enabled;
    tree["adaptive_bitrate_min_kbps"] = config::video.adaptive_bitrate_min;
    tree["adaptive_bitrate_max_kbps"] = config::video.adaptive_bitrate_max;
    send_response(response, tree);
  }

  /**
   * @brief Return host-side latency statistics and effective encoder
   *        settings. Read-only.
   * @details Requires @c api_scope_t::LOGS_GET (`logs:get`). Used by the
   *          SolarFlare Web UI to break down capture, conversion, encode,
   *          and send-path latency. Each metric is a min/max/avg/samples
   *          object in milliseconds: `capture_ms`, `convert_ms`,
   *          `encode_ms`, `network_total_ms`, `network_queue_dwell_ms`,
   *          `network_fec_ms`, `network_send_ms`, `rtt_ms`. Also returns
   *          `effective_settings` for the last encoder snapshot. Samples
   *          clear when the last streaming session tears down; idle polls
   *          with no samples return zeros.
   *
   * @api_examples{/api/stream/latency| GET| null}
   */
  void getStreamLatency(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::LOGS_GET)) {
      return;
    }
    print_req(request);

    auto &stats = sunshine::latency_stats();

    auto snapshot_to_json = [](const sunshine::stat_snapshot_t &snapshot) {
      nlohmann::json tree;
      tree["min"] = snapshot.min;
      tree["max"] = snapshot.max;
      tree["avg"] = snapshot.avg;
      tree["samples"] = snapshot.samples;
      return tree;
    };

    auto settings = stats.effective_settings();

    nlohmann::json effective_settings;
    effective_settings["codec"] = settings.codec;
    effective_settings["hwdevice"] = settings.hwdevice;
    effective_settings["vendor"] = settings.vendor;
    effective_settings["va_entrypoint"] = settings.va_entrypoint;
    effective_settings["rc_mode"] = settings.rc_mode;
    effective_settings["quality"] = settings.quality;
    effective_settings["slices"] = settings.slices;
    effective_settings["async_depth"] = settings.async_depth;
    effective_settings["qmin"] = settings.qmin;
    effective_settings["qmax"] = settings.qmax;
    effective_settings["rc_buffer_size"] = settings.rc_buffer_size;
    effective_settings["bit_rate"] = settings.bit_rate;
    effective_settings["framerate"] = settings.framerate;

    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    tree["capture_ms"] = snapshot_to_json(stats.capture_ms.snapshot());
    tree["convert_ms"] = snapshot_to_json(stats.convert_ms.snapshot());
    tree["encode_ms"] = snapshot_to_json(stats.encode_ms.snapshot());
    tree["network_total_ms"] = snapshot_to_json(stats.network_total_ms.snapshot());
    tree["network_queue_dwell_ms"] = snapshot_to_json(stats.network_queue_dwell_ms.snapshot());
    tree["network_fec_ms"] = snapshot_to_json(stats.network_fec_ms.snapshot());
    tree["network_send_ms"] = snapshot_to_json(stats.network_send_ms.snapshot());
    tree["rtt_ms"] = snapshot_to_json(stats.rtt_ms.snapshot());
    tree["effective_settings"] = effective_settings;
    send_response(response, tree);
  }

  /**
   * @brief Return host resource time-series telemetry. Read-only.
   * @details Requires @c api_scope_t::LOGS_GET (`logs:get`). Used by the
   *          SolarFlare Web UI to chart host CPU / memory / GPU utilisation
   *          over the last 10 minutes. Each key maps to an array of samples
   *          (oldest first); `host_cpu_pct` and `host_gpu_pct` are
   *          percentages 0-100, `host_ram_used_mb` is MiB. The store is
   *          Linux-only (the poll thread is a no-op elsewhere), so
   *          non-Linux platforms return an object with only `window_s`.
   *
   * @api_examples{/api/stream/telemetry| GET| null}
   */
  void getTelemetry(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::LOGS_GET)) {
      return;
    }
    print_req(request);

    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    tree["telemetry"] = sunshine::telemetry::snapshot();
    send_response(response, tree);
  }

  /**
   * @brief Return recent streaming-session history. Read-only.
   * @details Requires @c api_scope_t::LOGS_GET (`logs:get`). Reads
   *          session_history.jsonl and returns the most recent sessions
   *          (oldest first within the window), optionally filtered by
   *          `app` / `client` query parameters, capped by `limit`.
   *
   * @api_examples{/api/sessions| GET| null}
   */
  void getSessions(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::LOGS_GET)) {
      return;
    }
    print_req(request);

    auto limit = 100uz;
    std::string app_filter;
    std::string client_filter;
    auto query_params = request->parse_query_string();
    if (auto it = query_params.find("limit"); it != query_params.end()) {
      try {
        limit = static_cast<std::size_t>(std::stoull(it->second));
      } catch (...) {
        limit = 100uz;
      }
    }
    if (auto it = query_params.find("app"); it != query_params.end()) {
      app_filter = it->second;
    }
    if (auto it = query_params.find("client"); it != query_params.end()) {
      client_filter = it->second;
    }

    auto records = sunshine::session_history::recent(limit, app_filter, client_filter);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto &rec : records) {
      nlohmann::json item;
      item["t_start"] = std::chrono::duration_cast<std::chrono::seconds>(rec.t_start.time_since_epoch()).count();
      item["t_end"] = std::chrono::duration_cast<std::chrono::seconds>(rec.t_end.time_since_epoch()).count();
      item["app_name"] = rec.app_name;
      item["client_name"] = rec.client_name;
      item["client_address"] = rec.client_address;
      item["codec"] = rec.codec;
      item["width"] = rec.width;
      item["height"] = rec.height;
      item["fps"] = rec.fps;
      item["avg_bitrate_kbps"] = rec.avg_bitrate_kbps;
      item["avg_rtt_ms"] = rec.avg_rtt_ms;
      item["avg_encode_ms"] = rec.avg_encode_ms;
      item["dropped_frames"] = rec.dropped_frames;
      item["error"] = rec.error;
      arr.push_back(std::move(item));
    }

    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    tree["sessions"] = std::move(arr);
    send_response(response, tree);
  }

  /**
   * @brief Return process-wide error counters grouped by category.
   * @details Used by the SolarFlare fork Web UI to surface a 'recent errors'
   *          widget. Read-only; the counters are updated by every SUN_ERR()
   *          call in the codebase. Counters are monotonically increasing
   *          since process start -- the Web UI can diff against a snapshot
   *          to compute deltas if it wants recent-error rate.
   *
   * @api_examples{/api/errors| GET| null}
   */
  void getErrors(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::LOGS_GET)) {
      return;
    }
    print_req(request);

    auto &c = sunshine::counters();
    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    tree["encoder"] = c.encoder.load();
    tree["capture"] = c.capture.load();
    tree["network"] = c.network.load();
    tree["session"] = c.session.load();
    tree["process"] = c.process.load();
    tree["config"] = c.config.load();
    tree["crypto"] = c.crypto.load();
    tree["unknown"] = c.unknown.load();
    tree["total"] = c.total.load();
    send_response(response, tree);
  }

  /**
   * @brief List existing API tokens (admin only).
   * @details Returns the name and granted scopes of each token. Never returns
   *          the hash or salt — those are write-only secrets.
   *
   * @api_examples{/api/tokens| GET| null}
   */
  void listTokens(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::TOKENS_MANAGE)) {
      return;
    }
    print_req(request);

    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    auto arr = nlohmann::json::array();
    for (const auto &token : config::nvhttp.api_tokens) {
      auto entry = nlohmann::json::object();
      entry["name"] = token.name;
      auto scopes_arr = nlohmann::json::array();
      for (auto s : token.scopes) {
        scopes_arr.push_back(config::to_string(s));
      }
      entry["scopes"] = scopes_arr;
      arr.push_back(entry);
    }
    tree["tokens"] = arr;
    send_response(response, tree);
  }

  /**
   * @brief Create a new API token (admin only).
   * @details Body: `{"name": "...", "scopes": ["config:get", "apps:launch"]}`.
   *          The plaintext is generated server-side and returned in the
   *          response exactly once — it cannot be retrieved later.
   *
   * @api_examples{/api/tokens| POST| {"name":"ci-bot","scopes":["config:get","apps:launch"]}}
   */
  void createToken(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::TOKENS_MANAGE)) {
      return;
    }
    print_req(request);

    std::stringstream ss;
    ss << request->content.rdbuf();
    auto body = ss.str();

    nlohmann::json input;
    try {
      input = nlohmann::json::parse(body);
    } catch (const std::exception &e) {
      bad_request(response, request, "Invalid JSON body");
      return;
    }

    std::string name = input.value("name", "");
    if (name.empty()) {
      bad_request(response, request, "Token name is required");
      return;
    }
    // Reject duplicate names
    for (const auto &t : config::nvhttp.api_tokens) {
      if (t.name == name) {
        bad_request(response, request, "Token name already exists");
        return;
      }
    }

    std::vector<config::api_scope_t> scopes;
    auto scopes_in = input.find("scopes");
    if (scopes_in == input.end() || !scopes_in->is_array() || scopes_in->empty()) {
      bad_request(response, request, "At least one scope is required");
      return;
    }
    for (const auto &s : *scopes_in) {
      if (!s.is_string()) {
        bad_request(response, request, "Each scope must be a string");
        return;
      }
      auto parsed = config::api_scope_from_string(s.get<std::string>());
      if (!parsed) {
        bad_request(response, request, "Unknown scope: '" + s.get<std::string>() + "'");
        return;
      }
      scopes.push_back(*parsed);
    }

    auto minted = mint_api_token();

    // Persist to sunshine.conf via the existing saveConfig machinery:
    // append/replace the api_tokens entry. For simplicity we update the
    // in-memory config and emit a hint that the admin must add the new
    // entry to sunshine.conf manually. A future revision can wire this into
    // saveConfig to write the line automatically.
    // Build the api_tokens config entry that the admin must add to sunshine.conf.
    std::string cfg_entry = name + "\t" + minted.hash + "\t" + minted.salt + "\t";
    for (size_t i = 0; i < scopes.size(); ++i) {
      if (i > 0) {
        cfg_entry += ",";
      }
      cfg_entry += config::to_string(scopes[i]);
    }
    // ponytail: L-2 log injection guard -- strip CR/LF from admin-supplied name
    // before logging. JSON output is auto-escaped by nlohmann::json, but the
    // log line isn't.
    auto safe_name = name;
    for (auto &c : safe_name) {
      if (c == '\n' || c == '\r') {
        c = ' ';
      }
    }
    BOOST_LOG(info) << "API token '" << safe_name << "' minted. Add this line to sunshine.conf:\n"
                    << "  api_tokens += \"" << cfg_entry << "\"";

    nlohmann::json tree;
    tree["status_code"] = SimpleWeb::StatusCode::success_ok;
    tree["status"] = true;
    tree["name"] = name;
    tree["plaintext"] = minted.plaintext;
    tree["scopes"] = nlohmann::json::array();
    for (auto s : scopes) {
      tree["scopes"].push_back(config::to_string(s));
    }
    tree["warning"] = "Add the api_tokens line printed to the sunshine log to sunshine.conf to persist.";
    send_response(response, tree);
  }

  /**
   * @brief Delete an API token by name (admin only).
   *
   * @api_examples{/api/tokens/ci-bot| DELETE| null}
   */
  void deleteToken(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::TOKENS_MANAGE)) {
      return;
    }
    print_req(request);

    // Path looks like /api/tokens/<name>; <name> comes via SimpleWeb's
    // path matcher. We accept the name as the last URL path component.
    auto path = request->path;
    auto pos = path.rfind('/');
    std::string name = (pos != std::string::npos) ? path.substr(pos + 1) : path;

    bool found = false;
    for (auto it = config::nvhttp.api_tokens.begin(); it != config::nvhttp.api_tokens.end(); ++it) {
      if (it->name == name) {
        config::nvhttp.api_tokens.erase(it);
        found = true;
        break;
      }
    }

    nlohmann::json tree;
    if (found) {
      tree["status_code"] = SimpleWeb::StatusCode::success_ok;
      tree["status"] = true;
      BOOST_LOG(info) << "API token '"sv << name << "' deleted (in-memory; restart to also drop from sunshine.conf)"sv;
    } else {
      tree["status_code"] = SimpleWeb::StatusCode::client_error_not_found;
      tree["status"] = false;
      tree["error"] = "Token not found";
    }
    send_response(response, tree);
  }

  void getLogs(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::LOGS_GET)) {
      return;
    }

    print_req(request);

    std::string content = file_handler::read_file(config::sunshine.log_file.c_str());
    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Content-Type", "text/plain");
    headers.emplace("X-Frame-Options", "DENY");
    headers.emplace("Content-Security-Policy", "frame-ancestors 'none';");
    response->write(SimpleWeb::StatusCode::success_ok, content, headers);
  }

  /**
   * @brief Update existing credentials.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * The body for the post request should be JSON serialized in the following format:
   * @code{.json}
   * {
   *   "currentUsername": "Current Username",
   *   "currentPassword": "Current Password",
   *   "newUsername": "New Username",
   *   "newPassword": "New Password",
   *   "confirmNewPassword": "Confirm New Password"
   * }
   * @endcode
   *
   * @api_examples{/api/password| POST| {"currentUsername":"admin","currentPassword":"admin","newUsername":"admin","newPassword":"admin","confirmNewPassword":"admin"}}
   */
  void savePassword(const resp_https_t &response, const req_https_t &request) {
    if (!check_content_type(response, request, "application/json")) {
      return;
    }
    if (!config::sunshine.username.empty() && !require_scope(response, request, config::api_scope_t::STAR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    std::vector<std::string> errors = {};
    std::stringstream ss;
    std::stringstream config_stream;
    ss << request->content.rdbuf();
    try {
      nlohmann::json output_tree;
      nlohmann::json input_tree = nlohmann::json::parse(ss);
      std::string username = input_tree.value("currentUsername", "");
      std::string newUsername = input_tree.value("newUsername", "");
      std::string password = input_tree.value("currentPassword", "");
      std::string newPassword = input_tree.value("newPassword", "");
      std::string confirmPassword = input_tree.value("confirmNewPassword", "");
      if (newUsername.empty()) {
        newUsername = username;
      }
      if (!is_valid_web_username(newUsername)) {
        errors.emplace_back("Invalid Username");
      } else {
        auto hash = util::hex(crypto::hash(password + config::sunshine.salt)).to_string();
        if (config::sunshine.username.empty() || (boost::iequals(username, config::sunshine.username) && hash == config::sunshine.password)) {
          if (newPassword.empty() || newPassword != confirmPassword) {
            errors.emplace_back("Password Mismatch");
          } else if (http::save_user_creds(config::sunshine.credentials_file, newUsername, newPassword) != 0) {
            errors.emplace_back("Invalid Password");
          } else {
            http::reload_user_creds(config::sunshine.credentials_file);
            output_tree["status"] = true;
          }
        } else {
          errors.emplace_back("Invalid Current Credentials");
        }
      }

      if (!errors.empty()) {
        // join the errors array
        std::string error = std::accumulate(errors.begin(), errors.end(), std::string(), [](const std::string &a, const std::string &b) {
          return a.empty() ? b : a + ", " + b;
        });
        bad_request(response, request, error);
        return;
      }

      send_response(response, output_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "SavePassword: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Send a pin code to the host. The pin is generated from the Moonlight client during the pairing process.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * The body for the post request should be JSON serialized in the following format:
   * @code{.json}
   * {
   *   "pin": "<pin>",
   *   "name": "Friendly Client Name"
   * }
   * @endcode
   *
   * @api_examples{/api/pin| POST| {"pin":"1234","name":"My PC"}}
   */
  void savePin(const resp_https_t &response, const req_https_t &request) {
    if (!check_content_type(response, request, "application/json")) {
      return;
    }
    if (!require_scope(response, request, config::api_scope_t::CLIENTS_PAIR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    std::stringstream ss;
    ss << request->content.rdbuf();
    try {
      nlohmann::json output_tree;
      nlohmann::json input_tree = nlohmann::json::parse(ss);
      const std::string name = input_tree.value("name", "");
      const std::string pin = input_tree.value("pin", "");

      int _pin = 0;
      _pin = std::stoi(pin);
      if (_pin < 0 || _pin > 9999) {
        bad_request(response, request, "PIN must be between 0000 and 9999");
        return;
      }

      output_tree["status"] = nvhttp::pin(pin, name);
      send_response(response, output_tree);
    } catch (std::exception &e) {
      BOOST_LOG(warning) << "SavePin: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  /**
   * @brief Reset the display device persistence.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/reset-display-device-persistence| POST| null}
   */
  void resetDisplayDevicePersistence(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::DISPLAY_RESET)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    nlohmann::json output_tree;
    output_tree["status"] = display_device::reset_persistence();
    send_response(response, output_tree);
  }

  /**
   * @brief Restart Sunshine.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/restart| POST| null}
   */
  void restart(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::STAR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    // We may not return from this call
    platf::restart();
  }

  /**
   * @brief Return the SolarFlare self-update status (phase, percent, command log).
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/update| GET| null}
   */
  void getUpdateStatus(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CONFIG_GET)) {
      return;
    }

    print_req(request);
    send_response(response, update::to_json(update::status()));
  }

  /**
   * @brief Start downloading and staging the latest Linux release.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/update/start| POST| null}
   */
  void startUpdate(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::STAR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    if (const auto err = update::start()) {
      bad_request(response, request, *err);
      return;
    }

    send_response(response, update::to_json(update::status()));
  }

  /**
   * @brief Apply a staged SolarFlare update, optionally waiting for idle.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/update/apply| POST| {"when_idle":false}}
   */
  void applyUpdate(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::STAR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    bool when_idle = false;
    if (!request->content.string().empty()) {
      const auto tree = nlohmann::json::parse(request->content.string(), nullptr, false);
      if (!tree.is_discarded() && tree.contains("when_idle")) {
        when_idle = tree["when_idle"].get<bool>();
      }
    }

    if (const auto err = update::apply(when_idle)) {
      bad_request(response, request, *err);
      return;
    }

    send_response(response, update::to_json(update::status()));
  }

  /**
   * @brief Cancel a pending when-idle SolarFlare update apply.
   * @details Requires @c api_scope_t::CONFIG_SET (`config:set`) and CSRF
   *          validation for browser clients. Accepted only while phase is
   *          `ready` or `waiting_idle`. On success returns the updater
   *          status JSON; otherwise HTTP 400 with an error string.
   *          Clearing `waiting_idle` without a live wait worker restores
   *          `ready` immediately.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/update/cancel| POST| null}
   */
  void cancelUpdate(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CONFIG_SET)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    if (const auto err = update::cancel()) {
      bad_request(response, request, *err);
      return;
    }

    send_response(response, update::to_json(update::status()));
  }

  /**
   * @brief Get ViGEmBus driver version and installation status.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/vigembus/status| GET| null}
   */
  void getViGEmBusStatus(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CONFIG_GET)) {
      return;
    }

    print_req(request);

    nlohmann::json output_tree;

#ifdef _WIN32
    std::string version_str;
    bool installed = false;
    bool version_compatible = false;

    // Check if ViGEmBus driver exists
    std::filesystem::path driver_path = std::filesystem::path(std::getenv("SystemRoot") ? std::getenv("SystemRoot") : "C:\\Windows") / "System32" / "drivers" / "ViGEmBus.sys";

    if (std::filesystem::exists(driver_path)) {
      installed = platf::getFileVersionInfo(driver_path, version_str);
      if (installed) {
        // Parse version string to check compatibility (>= 1.17.0.0)
        std::vector<std::string> version_parts;
        std::stringstream ss(version_str);
        std::string part;
        while (std::getline(ss, part, '.')) {
          version_parts.push_back(part);
        }

        if (version_parts.size() >= 2) {
          int major = std::stoi(version_parts[0]);
          int minor = std::stoi(version_parts[1]);
          version_compatible = (major > 1) || (major == 1 && minor >= 17);
        }
      }
    }

    output_tree["installed"] = installed;
    output_tree["version"] = version_str;
    output_tree["version_compatible"] = version_compatible;
    output_tree["packaged_version"] = VIGEMBUS_PACKAGED_VERSION;
#else
    output_tree["error"] = "ViGEmBus is only available on Windows";
    output_tree["installed"] = false;
    output_tree["version"] = "";
    output_tree["version_compatible"] = false;
    output_tree["packaged_version"] = "";
#endif

    send_response(response, output_tree);
  }

  /**
   * @brief Install ViGEmBus driver with elevated permissions.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   *
   * @api_examples{/api/vigembus/install| POST| null}
   */
  void installViGEmBus(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::STAR)) {
      return;
    }

    std::string client_id = get_client_id(request);
    if (!validate_csrf_token(response, request, client_id)) {
      return;
    }

    print_req(request);

    nlohmann::json output_tree;

#ifdef _WIN32
    // Get the path to the packaged ViGEmBus installer.
    const std::filesystem::path installer_path = platf::appdata().parent_path() / "third-party" / "vigembus_installer.exe";

    if (!std::filesystem::exists(installer_path)) {
      output_tree["status"] = false;
      output_tree["error"] = "ViGEmBus installer not found";
      send_response(response, output_tree);
      return;
    }

    // Run the installer with elevated permissions
    std::error_code ec;
    boost::filesystem::path working_dir = boost::filesystem::path(installer_path.string()).parent_path();
    boost::process::v1::environment env = boost::this_process::environment();

    // Run with elevated permissions, non-interactive
    const std::string install_cmd = std::format("{} /quiet", installer_path.string());
    auto child = platf::run_command(true, false, install_cmd, working_dir, env, nullptr, ec, nullptr);

    if (ec) {
      output_tree["status"] = false;
      output_tree["error"] = "Failed to start installer: " + ec.message();
      send_response(response, output_tree);
      return;
    }

    // Wait for the installer to complete
    child.wait(ec);

    if (ec) {
      output_tree["status"] = false;
      output_tree["error"] = "Installer failed: " + ec.message();
    } else {
      int exit_code = child.exit_code();
      output_tree["status"] = (exit_code == 0);
      output_tree["exit_code"] = exit_code;
      if (exit_code != 0) {
        output_tree["error"] = std::format("Installer exited with code {}", exit_code);
      }
    }
#else
    output_tree["status"] = false;
    output_tree["error"] = "ViGEmBus installation is only available on Windows";
#endif

    send_response(response, output_tree);
  }

  /**
   * @brief Checks whether a directory entry qualifies as an executable file.
   * @param entry The directory entry to check.
   * @param status The cached file status for the entry.
   * @return True if the file should be included in an executable-type listing.
   */
  bool is_browsable_executable([[maybe_unused]] const fs::directory_entry &entry, [[maybe_unused]] const fs::file_status &status) {
#ifdef _WIN32
    auto ext = entry.path().extension().string();
    boost::algorithm::to_lower(ext);
    return ext == ".exe" || ext == ".bat" || ext == ".cmd" || ext == ".com" || ext == ".ps1";
#else
    const auto perms = status.permissions();
    return (perms & fs::perms::owner_exec) != fs::perms::none ||
           (perms & fs::perms::group_exec) != fs::perms::none ||
           (perms & fs::perms::others_exec) != fs::perms::none;
#endif
  }

#ifdef _WIN32
  /**
   * @brief Builds a JSON array of available Windows drive letters.
   * @return JSON array of drive-letter entries.
   */
  nlohmann::json get_windows_drives() {
    nlohmann::json entries = nlohmann::json::array();
    const DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
      if (drives & (1 << i)) {
        const auto drive_letter = static_cast<char>('A' + i);
        const auto drive_path = std::string(1, drive_letter) + ":\\";
        nlohmann::json entry;
        entry["name"] = drive_path;
        entry["type"] = "directory";
        entry["path"] = drive_path;
        entries.push_back(entry);
      }
    }
    return entries;
  }
#endif

  /**
   * @brief Lists, filters, and sorts the entries of a directory for the browse API.
   * @param dir_path The directory to list.
   * @param type_str Filter type: "directory", "executable", "file", or "any".
   * @return Sorted JSON array of entry objects with name/type/path fields.
   */
  nlohmann::json build_browse_entries(const fs::path &dir_path, const std::string &type_str) {
    nlohmann::json entries = nlohmann::json::array();

    std::error_code iter_ec;
    for (auto it = fs::directory_iterator(dir_path, fs::directory_options::skip_permission_denied, iter_ec);
         !iter_ec && it != fs::directory_iterator();
         it.increment(iter_ec)) {
      try {
        const auto status = it->status();
        const bool is_dir = fs::is_directory(status);

        if (const bool is_regular = fs::is_regular_file(status); !is_dir && !is_regular) {
          continue;
        }

        // Apply type filter (directories are always included for navigation)
        if (type_str == "directory" && !is_dir) {
          continue;
        }

        if (type_str == "executable" && !is_dir && !is_browsable_executable(*it, status)) {
          continue;
        }

        nlohmann::json file_entry;
        file_entry["name"] = it->path().filename().string();
        file_entry["path"] = it->path().string();
        file_entry["type"] = is_dir ? "directory" : "file";
        entries.push_back(file_entry);
      } catch (const fs::filesystem_error &e) {
        BOOST_LOG(debug) << "BrowseDirectory: skipping entry due to error: "sv << e.what();
      }
    }

    if (iter_ec) {
      BOOST_LOG(debug) << "BrowseDirectory: directory iteration error: "sv << iter_ec.message();
    }

    // Sort: directories first, then files; both case-insensitively alphabetical
    std::sort(entries.begin(), entries.end(), [](const nlohmann::json &a, const nlohmann::json &b) {
      const bool a_dir = (a["type"] == "directory");
      if (const bool b_dir = (b["type"] == "directory"); a_dir != b_dir) {
        return a_dir && !b_dir;
      }
      auto a_name = a["name"].get<std::string>();
      auto b_name = b["name"].get<std::string>();
      boost::algorithm::to_lower(a_name);
      boost::algorithm::to_lower(b_name);
      return a_name < b_name;
    });

    return entries;
  }

  /**
   * @brief Browse the server filesystem.
   * @param response The HTTP response object.
   * @param request The HTTP request object.
   * @note On Windows, an empty or root path returns the list of available drive letters.
   * @note On non-Windows, an empty path defaults to the filesystem root ("/").
   *
   * @api_examples{/api/browse?path=/home/user&type=directory| GET| null}
   */
  void browseDirectory(const resp_https_t &response, const req_https_t &request) {
    if (!require_scope(response, request, config::api_scope_t::CONFIG_GET)) {
      return;
    }

    print_req(request);

    try {
      const auto query_params = request->parse_query_string();

      std::string path_str;
      if (const auto path_it = query_params.find("path"); path_it != query_params.end()) {
        path_str = path_it->second;
      }

      std::string type_str = "any";
      if (const auto type_it = query_params.find("type"); type_it != query_params.end() && !type_it->second.empty()) {
        type_str = type_it->second;
      }

      nlohmann::json output_tree;

#ifdef _WIN32
      // On Windows with an empty or root path, return the list of available drive letters
      if (path_str.empty() || path_str == "/" || path_str == "\\") {
        output_tree["path"] = "";
        output_tree["parent"] = "";
        output_tree["entries"] = get_windows_drives();
        send_response(response, output_tree);
        return;
      }
#else
      // On non-Windows, default an empty path to the filesystem root
      if (path_str.empty()) {
        path_str = "/";
      }
#endif

      // Normalize the path
      fs::path dir_path = fs::weakly_canonical(fs::path(path_str));

      // If the path points to a file, use its parent directory
      std::error_code ec;
      if (fs::is_regular_file(dir_path, ec)) {
        dir_path = dir_path.parent_path();
      }

      // If the path doesn't exist, try the parent
      if (!fs::exists(dir_path, ec)) {
        dir_path = dir_path.parent_path();
      }

      if (!fs::is_directory(dir_path, ec)) {
        bad_request(response, request, "Path is not a directory");
        return;
      }

      output_tree["path"] = dir_path.string();

      // Determine the parent path for the "Up" navigation
      const fs::path parent = dir_path.parent_path();
#ifdef _WIN32
      // At a drive root (e.g., C:\) the parent equals itself; signal the drive list with an empty string
      output_tree["parent"] = (parent == dir_path) ? "" : parent.string();
#else
      output_tree["parent"] = parent.string();
#endif

      output_tree["entries"] = build_browse_entries(dir_path, type_str);
      send_response(response, output_tree);
    } catch (const fs::filesystem_error &e) {
      BOOST_LOG(warning) << "BrowseDirectory: "sv << e.what();
      bad_request(response, request, e.what());
    }
  }

  bool is_valid_client_uuid(const std::string &uuid) {
    static const std::regex pattern {
      R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)"
    };
    return std::regex_match(uuid, pattern);
  }

  bool is_valid_web_username(const std::string &username) {
    constexpr std::size_t MAX_USERNAME_LEN = 256;
    return !username.empty() && username.size() <= MAX_USERNAME_LEN && !contains_control_chars(username);
  }

  void start() {
    platf::set_thread_name("confighttp");
    const auto shutdown_event = mail::man->event<bool>(mail::shutdown);

    const auto port_https = net::map_port(PORT_HTTPS);
    const auto address_family = net::af_from_enum_string(config::sunshine.address_family);

    https_server_t server {config::nvhttp.cert, config::nvhttp.pkey};

    // Helper to create page handler lambdas without repeating the signature
    auto page_handler = [](const char *file, bool require_auth = true, bool redirect_if_username = false) {
      return [file, require_auth, redirect_if_username](const resp_https_t &response, const req_https_t &request) {
        getPage(response, request, file, require_auth, redirect_if_username);
      };
    };

    // Default resource handlers
    const https_handler_t bad_request_handler = [](const resp_https_t &response, const req_https_t &request) {
      bad_request(response, request);
    };
    const https_handler_t not_found_handler = [](const resp_https_t &response, const req_https_t &request) {
      not_found(response, request);
    };

    // error by default
    server.default_resource["DELETE"] = bad_request_handler;
    server.default_resource["PATCH"] = bad_request_handler;
    server.default_resource["POST"] = bad_request_handler;
    server.default_resource["PUT"] = bad_request_handler;
    server.default_resource["GET"] = not_found_handler;

    // web pages
    server.resource["^/$"]["GET"] = page_handler("index.html");
    server.resource["^/apps/?$"]["GET"] = page_handler("apps.html");
    server.resource["^/clients/?$"]["GET"] = page_handler("clients.html");
    server.resource["^/config/?$"]["GET"] = page_handler("config.html");
    server.resource["^/featured/?$"]["GET"] = page_handler("featured.html");
    server.resource["^/logout/?$"]["GET"] = page_handler("logout.html", false);
    server.resource["^/password/?$"]["GET"] = page_handler("password.html");
    server.resource["^/pin/?$"]["GET"] = page_handler("pin.html");
    server.resource["^/troubleshooting/?$"]["GET"] = page_handler("troubleshooting.html");
    server.resource["^/welcome/?$"]["GET"] = page_handler("welcome.html", false, true);

    // rest api
    server.resource["^/api/browse$"]["GET"] = browseDirectory;
    server.resource["^/api/apps$"]["GET"] = getApps;
    server.resource["^/api/apps$"]["POST"] = saveApp;
    server.resource["^/api/apps/([0-9]+)$"]["DELETE"] = deleteApp;
    server.resource["^/api/apps/close$"]["POST"] = closeApp;
    server.resource["^/api/clients/list$"]["GET"] = getClients;
    server.resource["^/api/clients/unpair$"]["POST"] = unpair;
    server.resource["^/api/clients/unpair-all$"]["POST"] = unpairAll;
    server.resource["^/api/clients/update$"]["POST"] = updateClient;
    server.resource["^/api/config$"]["GET"] = getConfig;
    server.resource["^/api/config$"]["POST"] = saveConfig;
    server.resource["^/api/configLocale$"]["GET"] = getLocale;
    server.resource["^/api/covers/([0-9]+)$"]["GET"] = getCover;
    server.resource["^/api/covers/upload$"]["POST"] = uploadCover;
    server.resource["^/api/csrf-token$"]["GET"] = getCSRFToken;
    server.resource["^/api/games/scan$"]["GET"] = scanGames;
    server.resource["^/api/password$"]["POST"] = savePassword;
    server.resource["^/api/pin$"]["POST"] = savePin;
    server.resource["^/api/logs$"]["GET"] = getLogs;
    server.resource["^/api/tokens$"]["GET"] = listTokens;
    server.resource["^/api/tokens$"]["POST"] = createToken;
    server.resource["^/api/tokens/([\\w-]+)$"]["DELETE"] = deleteToken;
    server.resource["^/api/stream/network-stats$"]["POST"] = postNetworkStats;
    server.resource["^/api/stream/bitrate$"]["GET"] = getBitrate;
    server.resource["^/api/stream/latency$"]["GET"] = getStreamLatency;
    server.resource["^/api/stream/telemetry$"]["GET"] = getTelemetry;
    server.resource["^/api/sessions$"]["GET"] = getSessions;
    server.resource["^/api/errors$"]["GET"] = getErrors;
    server.resource["^/api/reset-display-device-persistence$"]["POST"] = resetDisplayDevicePersistence;
    server.resource["^/api/restart$"]["POST"] = restart;
    server.resource["^/api/update$"]["GET"] = getUpdateStatus;
    server.resource["^/api/update/start$"]["POST"] = startUpdate;
    server.resource["^/api/update/apply$"]["POST"] = applyUpdate;
    server.resource["^/api/update/cancel$"]["POST"] = cancelUpdate;
    server.resource["^/api/vigembus/status$"]["GET"] = getViGEmBusStatus;
    server.resource["^/api/vigembus/install$"]["POST"] = installViGEmBus;

    // static/dynamic resources
    server.resource["^/images/sunshine.ico$"]["GET"] = getFaviconImage;
    server.resource["^/images/logo-sunshine-45.png$"]["GET"] = getSunshineLogoImage;
    server.resource["^/manifest.webmanifest$"]["GET"] = getManifest;
    server.resource["^/assets\\/.+$"]["GET"] = getAsset;

    server.config.reuse_address = true;
    server.config.address = net::get_bind_address(address_family);
    server.config.port = port_https;

    // Store bind address for logging, use "localhost" as fallback for wildcard addresses
    const auto bind_addr = server.config.address;
    const auto display_addr = config::sunshine.bind_address.empty() ? "localhost"sv : std::string_view {bind_addr};

    auto accept_and_run = [&](auto *server) {
      try {
        platf::set_thread_name("confighttp::tcp");
        server->start([&display_addr](const unsigned short port) {
          BOOST_LOG(info) << "Configuration UI available at [https://"sv << display_addr << ":" << port << "]";
        });
      } catch (boost::system::system_error &err) {
        // It's possible the exception gets thrown after calling server->stop() from a different thread
        if (shutdown_event->peek()) {
          return;
        }

        BOOST_LOG(fatal) << "Couldn't start Configuration HTTPS server on port ["sv << port_https << "]: "sv << err.what();
        shutdown_event->raise(true);
        return;
      }
    };
    std::thread tcp {accept_and_run, &server};

    // Wait for any event
    shutdown_event->view();

    server.stop();

    tcp.join();
  }
}  // namespace confighttp
