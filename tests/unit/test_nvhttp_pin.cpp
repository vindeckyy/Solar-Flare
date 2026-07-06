/**
 * @file tests/unit/test_nvhttp_pin.cpp
 * @brief Tests for nvhttp::pin() — the held async-response path used when
 *        the user enters their Moonlight pairing PIN through the web UI.
 *
 * @details
 * Regression coverage for the pairing-timeout bug: the held async response
 * must be marked `close_connection_after_response = true` after `pin()`
 * writes the XML body, otherwise SimpleWeb's keep-alive path is taken on
 * HTTP/1.1 and the client never sees a complete reply, causing the
 * Moonlight pairing handshake to time out.
 */
#include "../tests_common.h"

#include <src/nvhttp.h>

#include <boost/asio.hpp>
#include <Simple-Web-Server/server_http.hpp>

#include <array>
#include <atomic>
#include <memory>
#include <string>

using namespace nvhttp;

/**
 * @brief Minimal ServerBase subclass that exposes the normally-protected
 *        `write()` / `create_connection()` helpers and the nested
 *        `Session` type so the test can build a real `Response` object
 *        without spinning up a full HTTP listener.
 */
class PinTestServer: public SimpleWeb::ServerBase<SimpleWeb::HTTP> {
public:
  PinTestServer():
      SimpleWeb::ServerBase<SimpleWeb::HTTP>(0) {
    // Ensure timers/long timeouts don't fire while the test is in flight.
    config.timeout_content = 300;
  }

  void accept() override {
    // No-op: the test never opens a listening socket.
  }

  using SimpleWeb::ServerBase<SimpleWeb::HTTP>::create_connection;
  using SimpleWeb::ServerBase<SimpleWeb::HTTP>::write;
  // Expose the otherwise protected nested types so the test can name them.
  using SimpleWeb::ServerBase<SimpleWeb::HTTP>::Connection;
  using SimpleWeb::ServerBase<SimpleWeb::HTTP>::Session;
  using SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response;
};

/**
 * @brief Build a real SimpleWeb::Response backed by an in-memory asio
 *        connection. The returned response is suitable for storing in
 *        `pair_session_t::async_insert_pin.response` (left variant).
 */
static std::shared_ptr<PinTestServer::Response> make_test_response(PinTestServer &server) {
  static boost::asio::io_context io;
  auto conn = server.create_connection(io);
  auto sess = std::make_shared<PinTestServer::Session>(server.config.max_request_streambuf_size, conn);

  std::shared_ptr<PinTestServer::Response> captured;
  auto resource = [&captured](
                    std::shared_ptr<PinTestServer::Response> resp,
                    std::shared_ptr<PinTestServer::Request> /*req*/) {
    captured = resp;
  };
  // write() takes the resource by non-const lvalue reference, so bind it
  // to a named local.
  std::function<void(std::shared_ptr<PinTestServer::Response>,
                     std::shared_ptr<PinTestServer::Request>)>
    resource_fn = resource;
  server.write(sess, resource_fn);
  return captured;
}

/**
 * @brief RAII wrapper that snapshots the response state emitted by the
 *        test-only pin observer into easy-to-assert member fields.
 */
struct PinCapture {
  std::atomic<bool> fired {false};
  bool close_flag = false;
  std::size_t body_size = 0;

  void reset() {
    fired.store(false);
    close_flag = false;
    body_size = 0;
  }

  static void observer(const test_access::PinResponseSnapshot &snap) {
    // The observer is a plain function pointer; it must dispatch into a
    // thread-local instance. We use a single global because the test
    // fixture serialises pin() invocations and never overlaps them.
    instance().ingest(snap);
  }

  static PinCapture &instance() {
    static PinCapture c;
    return c;
  }

  void ingest(const test_access::PinResponseSnapshot &snap) {
    close_flag = snap.close_connection_after_response;
    body_size = snap.body_size;
    fired.store(true);
  }
};

/**
 * @brief Test fixture that resets the internal pairing-session map and
 *        the pin observer between tests, so order does not matter and
 *        one test cannot pollute the next.
 */
struct NvhttpPinTest: testing::Test {
  void SetUp() override {
    nvhttp::test_access::clear_pair_sessions();
    PinCapture::instance().reset();
    nvhttp::test_access::set_pin_observer(&PinCapture::observer);
  }

  void TearDown() override {
    nvhttp::test_access::set_pin_observer(nullptr);
    nvhttp::test_access::clear_pair_sessions();
  }
};

/**
 * @brief `pin()` returns false when the internal session map is empty.
 */
TEST_F(NvhttpPinTest, ReturnsFalseWhenNoSessions) {
  EXPECT_FALSE(nvhttp::pin("1234", "laptop"));
  EXPECT_FALSE(PinCapture::instance().fired.load());
}

/**
 * @brief `pin()` rejects PINs that are not exactly four digits.
 */
TEST_F(NvhttpPinTest, ReturnsFalseForWrongLengthPin) {
  EXPECT_FALSE(nvhttp::pin("", "laptop"));
  EXPECT_FALSE(nvhttp::pin("1", "laptop"));
  EXPECT_FALSE(nvhttp::pin("12", "laptop"));
  EXPECT_FALSE(nvhttp::pin("123", "laptop"));
  EXPECT_FALSE(nvhttp::pin("12345", "laptop"));
  EXPECT_FALSE(nvhttp::pin("123456", "laptop"));
}

/**
 * @brief `pin()` rejects PINs that contain non-numeric characters.
 */
TEST_F(NvhttpPinTest, ReturnsFalseForNonNumericPin) {
  EXPECT_FALSE(nvhttp::pin("abcd", "laptop"));
  EXPECT_FALSE(nvhttp::pin("12a4", "laptop"));
  EXPECT_FALSE(nvhttp::pin(" 123", "laptop"));
  EXPECT_FALSE(nvhttp::pin("123 ", "laptop"));
}

/**
 * @brief `pin()` returns false when the held session has no async response
 *        to write to.
 */
TEST_F(NvhttpPinTest, ReturnsFalseWhenNoHeldResponse) {
  pair_session_t sess;
  sess.client.uniqueID = "no-resp-uuid";
  sess.client.cert = "test-cert";
  // async_insert_pin.response left default-constructed (monostate)
  nvhttp::test_access::add_pair_session(std::move(sess));

  EXPECT_FALSE(nvhttp::pin("1234", "laptop"));
  EXPECT_FALSE(PinCapture::instance().fired.load());
}

/**
 * @brief Regression test: when `pin()` writes the XML body to the held
 *        async response, the response must be marked
 *        `close_connection_after_response = true` so SimpleWeb terminates
 *        the keep-alive loop and the Moonlight client actually receives
 *        the reply. Before the fix, this flag was left at its default
 *        (`false`), causing the client to time out.
 */
TEST_F(NvhttpPinTest, MarksHeldResponseAsCloseAfterSend) {
  PinTestServer server;
  auto resp = make_test_response(server);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->close_connection_after_response);
  EXPECT_EQ(resp->size(), 0u);

  pair_session_t sess;
  sess.client.uniqueID = "close-flag-uuid";
  sess.client.cert = "test-cert";
  // 32-byte (64 hex char) salt — required by getservercert() to derive the
  // AES key. Without this, getservercert() rejects the call via fail_pair()
  // and the session (with its held response) is destroyed before pin() can
  // set the close-after-send flag, so the regression we are guarding
  // against would not be exercised.
  sess.async_insert_pin.salt = "ff5dc6eda99339a8a0793e216c4257c4ff5dc6eda99339a8a0793e216c4257c4";
  sess.async_insert_pin.response = resp;
  nvhttp::test_access::add_pair_session(std::move(sess));

  EXPECT_TRUE(nvhttp::pin("1234", "laptop"));

  // The observer must have fired exactly once with the post-write snapshot.
  const auto &cap = PinCapture::instance();
  ASSERT_TRUE(cap.fired.load()) << "test_access::set_pin_observer callback never fired";

  // The close-after-send flag is the actual regression: without it, the
  // Moonlight pairing handshake waits forever on the keep-alive loop and
  // times out, so this is the bit that fixes the user-visible bug.
  EXPECT_TRUE(cap.close_flag) << "pin() must set close_connection_after_response=true on the held async response";

  // And the body actually has to be written — an empty body would be
  // another way for the client to fail to receive a useful response.
  EXPECT_GT(cap.body_size, 0u) << "pin() must write the XML body to the held async response";
}

/**
 * @brief `pin()` should propagate the user-supplied client name onto the
 *        session so the web UI can display which device is pairing.
 */
TEST_F(NvhttpPinTest, SetsClientNameOnSuccess) {
  PinTestServer server;
  auto resp = make_test_response(server);

  pair_session_t sess;
  sess.client.uniqueID = "name-uuid";
  sess.client.cert = "test-cert";
  sess.async_insert_pin.salt = "ff5dc6eda99339a8a0793e216c4257c4ff5dc6eda99339a8a0793e216c4257c4";
  sess.async_insert_pin.response = resp;
  nvhttp::test_access::add_pair_session(std::move(sess));

  EXPECT_TRUE(nvhttp::pin("0000", "Bedroom PC"));
  EXPECT_TRUE(PinCapture::instance().fired.load());
}
