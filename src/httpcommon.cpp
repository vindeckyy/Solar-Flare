/**
 * @file src/httpcommon.cpp
 * @brief Definitions for common HTTP.
 */
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

// standard includes
#include <filesystem>
#include <utility>

// lib includes
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/context_base.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <curl/curl.h>
#include <Simple-Web-Server/server_http.hpp>
#include <Simple-Web-Server/server_https.hpp>

// local includes
#include "config.h"
#include "crypto.h"
#include "file_handler.h"
#include "httpcommon.h"
#include "logging.h"
#include "network.h"
#include "nvhttp.h"
#include "platform/common.h"
#include "process.h"
#include "rtsp.h"
#include "utility.h"
#include "uuid.h"

namespace http {
  using namespace std::literals;
  namespace fs = std::filesystem;
  namespace pt = boost::property_tree;

  int reload_user_creds(const std::string &file);
  bool user_creds_exist(const std::string &file);

  std::string unique_id;
  net::net_e origin_web_ui_allowed;

  int init() {
    bool clean_slate = config::sunshine.flags[config::flag::FRESH_STATE];
    origin_web_ui_allowed = net::from_enum_string(config::nvhttp.origin_web_ui_allowed);

    // Persist cert/pkey in appdata/credentials/ so they survive reboots.
    // Previously these lived under temp_directory_path()/Sunshine/, which
    // is wiped by systemd-tmpfiles on reboot and by some package-manager
    // hooks. After a reboot Sunshine would start with no cert on disk
    // and SSL_CTX_use_certificate_chain_file() would silently fail; every
    // HTTPS handshake aborted at server hello and Moonlight could not pair.
    auto creds_dir = platf::appdata() / "credentials"sv;
    if (clean_slate || config::nvhttp.cert.empty() || config::nvhttp.pkey.empty()) {
      // ponytail: adopt any existing pkey/cert pair on disk before generating
      // a new UUID. Without this, every restart produced a new cert because
      // the user has no pkey=/cert= in sunshine.conf (defaults to empty),
      // so the unique_id was random per start. Each new cert invalidates
      // every paired client -- they re-pair. Pick the most recently written
      // pair if multiple exist (multi-instance legacy), or generate a fresh
      // UUID on a true cold install.
      std::error_code ec;
      std::vector<fs::path> existing_pkies;
      for (auto &e : fs::directory_iterator(creds_dir, ec)) {
        if (e.is_regular_file() && e.path().filename().string().starts_with("pkey-"sv)) {
          existing_pkies.push_back(e.path());
        }
      }
      if (!existing_pkies.empty()) {
        // ponytail: pick the pair whose pkey has the highest mtime. mtime
        // is the cheapest sort key that's robust to the multi-instance
        // case (one of the older pairs is whichever instance wrote last).
        auto newest = std::max_element(existing_pkies.begin(), existing_pkies.end(),
          [](const fs::path &a, const fs::path &b) {
            return fs::last_write_time(a) < fs::last_write_time(b);
          });
        auto pkey_path = *newest;
        auto cert_path = pkey_path;
        cert_path.replace_filename(std::string("cert-") + pkey_path.filename().string().substr(5));
        // ponytail: on a hot-restart loop, we may have written pkey but the
        // cert write got interrupted. Skip the pair if the cert is missing
        // -- fall through to fresh generation rather than failing to start.
        if (fs::exists(cert_path)) {
          if (existing_pkies.size() > 1) {
            BOOST_LOG(warning) << "Multiple cert/pkey pairs found in "sv << creds_dir
                               << "; using newest pair "sv << pkey_path.filename().string()
                               << ". Pass --fresh-state or remove the other pairs to clean up."sv;
          }
          config::nvhttp.pkey = pkey_path.string();
          config::nvhttp.cert = cert_path.string();
          unique_id = pkey_path.filename().string().substr(5);
        } else {
          BOOST_LOG(warning) << "Found orphan pkey "sv << pkey_path.filename().string()
                             << " without matching cert; generating fresh credentials."sv;
          unique_id = uuid_util::uuid_t::generate().string();
          config::nvhttp.cert = (creds_dir / ("cert-"s + unique_id)).string();
          config::nvhttp.pkey = (creds_dir / ("pkey-"s + unique_id)).string();
        }
      } else {
        // Generate a unique suffix so multiple Sunshine instances on the
        // same machine (different ports) don't trample each other.
        unique_id = uuid_util::uuid_t::generate().string();
        config::nvhttp.cert = (creds_dir / ("cert-"s + unique_id)).string();
        config::nvhttp.pkey = (creds_dir / ("pkey-"s + unique_id)).string();
      }
    }

    // Generate cert/key if either file is missing. Missing-after-reboot is
    // the common case this protects against; missing-on-fresh-install is
    // the original case.
    if ((!fs::exists(config::nvhttp.pkey) || !fs::exists(config::nvhttp.cert)) &&
        create_creds(config::nvhttp.pkey, config::nvhttp.cert)) {
      return -1;
    }

    // If the path still doesn't point at a real file (e.g. the user
    // hand-set `cert` in sunshine.conf to a directory), refuse to start
    // with a clear error rather than silently serving TLS handshakes
    // that always fail at server hello.
    if (fs::is_directory(config::nvhttp.cert) || !fs::is_regular_file(config::nvhttp.cert)) {
      BOOST_LOG(fatal) << "nvhttp.cert is not a regular file: "sv << config::nvhttp.cert;
      BOOST_LOG(fatal) << "Set `cert` in sunshine.conf to a writable file path, or remove the cert/pkey keys to let Sunshine generate fresh ones under "sv << creds_dir;
      return -1;
    }
    if (fs::is_directory(config::nvhttp.pkey) || !fs::is_regular_file(config::nvhttp.pkey)) {
      BOOST_LOG(fatal) << "nvhttp.pkey is not a regular file: "sv << config::nvhttp.pkey;
      return -1;
    }
    if (!user_creds_exist(config::sunshine.credentials_file)) {
      BOOST_LOG(info) << "Open the Web UI to set your new username and password and getting started";
    } else if (reload_user_creds(config::sunshine.credentials_file)) {
      return -1;
    }
    return 0;
  }

  int save_user_creds(const std::string &file, const std::string &username, const std::string &password, bool run_our_mouth) {
    // ponytail: M-4 minimum password length. Cheap reject at the write side;
    // bcrypt-style hashing would be the stronger answer but ships a new dep.
    constexpr std::size_t MIN_PASSWORD_LEN = 12;
    if (password.size() < MIN_PASSWORD_LEN) {
      BOOST_LOG(error) << "Password must be at least "sv << MIN_PASSWORD_LEN << " characters"sv;
      return -1;
    }
    pt::ptree outputTree;

    if (fs::exists(file)) {
      try {
        pt::read_json(file, outputTree);
      } catch (std::exception &e) {
        BOOST_LOG(error) << "Couldn't read user credentials: "sv << e.what();
        return -1;
      }
    }

    auto salt = crypto::rand_alphabet(16);
    outputTree.put("username", username);
    outputTree.put("salt", salt);
    outputTree.put("password", util::hex(crypto::hash(password + salt)).to_string());
    try {
      pt::write_json(file, outputTree);
    } catch (std::exception &e) {
      BOOST_LOG(error) << "error writing to the credentials file, perhaps try this again as an administrator? Details: "sv << e.what();
      return -1;
    }

    BOOST_LOG(info) << "New credentials have been created"sv;
    return 0;
  }

  bool user_creds_exist(const std::string &file) {
    if (!fs::exists(file)) {
      return false;
    }

    pt::ptree inputTree;
    try {
      pt::read_json(file, inputTree);
      return inputTree.find("username") != inputTree.not_found() &&
             inputTree.find("password") != inputTree.not_found() &&
             inputTree.find("salt") != inputTree.not_found();
    } catch (std::exception &e) {
      BOOST_LOG(error) << "validating user credentials: "sv << e.what();
    }

    return false;
  }

  int reload_user_creds(const std::string &file) {
    pt::ptree inputTree;
    try {
      pt::read_json(file, inputTree);
      config::sunshine.username = inputTree.get<std::string>("username");
      config::sunshine.password = inputTree.get<std::string>("password");
      config::sunshine.salt = inputTree.get<std::string>("salt");
    } catch (std::exception &e) {
      BOOST_LOG(error) << "loading user credentials: "sv << e.what();
      return -1;
    }
    return 0;
  }

  int create_creds(const std::string &pkey, const std::string &cert) {
    fs::path pkey_path = pkey;
    fs::path cert_path = cert;

    auto creds = crypto::gen_creds("Sunshine Gamestream Host"sv, 2048);

    auto pkey_dir = pkey_path;
    auto cert_dir = cert_path;
    pkey_dir.remove_filename();
    cert_dir.remove_filename();

    std::error_code err_code {};
    fs::create_directories(pkey_dir, err_code);
    if (err_code) {
      BOOST_LOG(error) << "Couldn't create directory ["sv << pkey_dir << "] :"sv << err_code.message();
      return -1;
    }

    fs::create_directories(cert_dir, err_code);
    if (err_code) {
      BOOST_LOG(error) << "Couldn't create directory ["sv << cert_dir << "] :"sv << err_code.message();
      return -1;
    }

    if (file_handler::write_file(pkey.c_str(), creds.pkey)) {
      BOOST_LOG(error) << "Couldn't open ["sv << config::nvhttp.pkey << ']';
      return -1;
    }

    if (file_handler::write_file(cert.c_str(), creds.x509)) {
      BOOST_LOG(error) << "Couldn't open ["sv << config::nvhttp.cert << ']';
      return -1;
    }

    fs::permissions(pkey_path, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, err_code);

    if (err_code) {
      BOOST_LOG(error) << "Couldn't change permissions of ["sv << config::nvhttp.pkey << "] :"sv << err_code.message();
      return -1;
    }

    fs::permissions(cert_path, fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read | fs::perms::owner_write, fs::perm_options::replace, err_code);

    if (err_code) {
      BOOST_LOG(error) << "Couldn't change permissions of ["sv << config::nvhttp.cert << "] :"sv << err_code.message();
      return -1;
    }

    return 0;
  }

  bool download_file(const std::string &url, const std::string &file, long ssl_version) {
    // sonar complains about weak ssl and tls versions; however sonar cannot detect the fix
    CURL *curl = curl_easy_init();  // NOSONAR
    if (!curl) {
      BOOST_LOG(error) << "Couldn't create CURL instance";
      return false;
    }

    if (std::string file_dir = file_handler::get_parent_directory(file); !file_handler::make_directory(file_dir)) {
      BOOST_LOG(error) << "Couldn't create directory ["sv << file_dir << ']';
      curl_easy_cleanup(curl);
      return false;
    }

    FILE *fp = fopen(file.c_str(), "wb");
    if (!fp) {
      BOOST_LOG(error) << "Couldn't open ["sv << file << ']';
      curl_easy_cleanup(curl);
      return false;
    }

    curl_easy_setopt(curl, CURLOPT_SSLVERSION, ssl_version);  // NOSONAR
    // ponytail: L-3 -- require TLS verification, HTTPS-only, and a hard timeout
    // for outbound fetches. Admin endpoint, but host check happens upstream
    // so we belt-and-suspenders here.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
      BOOST_LOG(error) << "Couldn't download ["sv << url << ", code:" << result << ']';
    }

    curl_easy_cleanup(curl);
    fclose(fp);
    return result == CURLE_OK;
  }

  std::string url_escape(const std::string &url) {
    char *string = curl_easy_escape(nullptr, url.c_str(), static_cast<int>(url.length()));
    std::string result(string);
    curl_free(string);
    return result;
  }

  std::string url_get_host(const std::string &url) {
    CURLU *curlu = curl_url();
    curl_url_set(curlu, CURLUPART_URL, url.c_str(), static_cast<unsigned int>(url.length()));
    char *host;
    if (curl_url_get(curlu, CURLUPART_HOST, &host, 0) != CURLUE_OK) {
      curl_url_cleanup(curlu);
      return "";
    }
    std::string result(host);
    curl_free(host);
    curl_url_cleanup(curlu);
    return result;
  }
}  // namespace http
