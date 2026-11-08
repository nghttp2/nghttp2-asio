#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <nghttp2/asio_http2.h>
#include <nghttp2/asio_http2_client.h>
#include <nghttp2/asio_http2_server.h>

using namespace nghttp2::asio_http2;
using namespace std::chrono_literals;

namespace {

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "CHECK failed: " #cond << " at " << __FILE__ << ":"         \
                << __LINE__ << std::endl;                                      \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

#define CHECK_EVENTS(events, name)                                             \
  do {                                                                         \
    if (std::find((events).begin(), (events).end(), (name)) ==                 \
        (events).end()) {                                                      \
      std::cerr << "Missing event '" << (name) << "'. Saw:";                   \
      for (const auto &ev : (events)) {                                        \
        std::cerr << " " << ev;                                                \
      }                                                                        \
      std::cerr << std::endl;                                                  \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

std::string request_uri(int port, const std::string &path) {
  return "http://127.0.0.1:" + std::to_string(port) + path;
}

void test_shutdown_before_connect() {
  boost::asio::io_context io_context;
  client::session sess(io_context, "127.0.0.1", "1");

  sess.shutdown();
  io_context.run();
}

void test_external_io_context_lifecycle() {
  boost::asio::io_context io_context;
  server::http2 server(io_context);

  std::atomic<bool> request_served{false};

  CHECK(server.handle("/", [&](const server::request &req,
                               const server::response &res) {
    CHECK(req.tls_peer_certificate() == nullptr);
    request_served = true;
    res.write_head(200);
    res.end("ok");
  }));

  boost::system::error_code ec;
  auto serve_ec =
      server.listen_and_serve(ec, "127.0.0.1", "0", /*asynchronous=*/true);
  CHECK(!serve_ec);
  CHECK(!ec);

  auto ports = server.ports();
  CHECK(!ports.empty());
  const int port = ports[0];

  std::thread io_thread([&]() { io_context.run(); });

  {
    boost::asio::io_context client_io;

    boost::system::error_code client_ec;
    client::session sess(client_io, "127.0.0.1", std::to_string(port));
    sess.read_timeout(5s);

    std::atomic<bool> connected{false};
    std::atomic<bool> response_done{false};
    sess.on_connect([&](boost::asio::ip::tcp::endpoint) { connected = true; });

    const auto connect_deadline = std::chrono::steady_clock::now() + 5s;
    while (!connected && std::chrono::steady_clock::now() < connect_deadline) {
      client_io.run_one();
    }
    CHECK(connected);

    const client::request *req = sess.submit(
        client_ec, "GET", request_uri(port, "/"), header_map{});
    CHECK(!client_ec);
    CHECK(req != nullptr);

    req->on_response([&](const client::response &res) {
      CHECK(res.status_code() == 200);
    });
    req->on_close([&](uint32_t) { response_done = true; });

    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!response_done && std::chrono::steady_clock::now() < deadline) {
      client_io.run_one();
    }
    CHECK(response_done);
  }

  CHECK(request_served);

  std::atomic<bool> posted{false};
  boost::asio::post(io_context, [&]() { posted = true; });
  const auto post_deadline = std::chrono::steady_clock::now() + 5s;
  while (!posted && std::chrono::steady_clock::now() < post_deadline) {
    std::this_thread::sleep_for(10ms);
  }
  CHECK(posted);

  server.stop();
  server.join();

  io_context.stop();
  io_thread.join();
}

void test_response_trailers_and_ordering() {
  boost::asio::io_context io_context;
  server::http2 server(io_context);

  CHECK(server.handle("/trailers", [&](const server::request &,
                                       const server::response &res) {
    res.write_head(200, {{"content-type", {"text/plain", false}}});
    auto sent_body = std::make_shared<bool>(false);
    res.end([&res, sent_body](uint8_t *buf, size_t len,
                              uint32_t *data_flags) -> ssize_t {
      if (!*sent_body) {
        *sent_body = true;
        const char payload[] = "body";
        std::copy_n(payload, sizeof(payload) - 1, buf);
        return sizeof(payload) - 1;
      }
      *data_flags |= NGHTTP2_DATA_FLAG_EOF;
      *data_flags |= NGHTTP2_DATA_FLAG_NO_END_STREAM;
      res.write_trailer({{"checksum", {"deadbeef", false}}});
      return 0;
    });
  }));

  CHECK(server.handle(
      "/trailers-only",
      [&](const server::request &, const server::response &res) {
        res.write_head(200);
        res.end([&res](uint8_t *, size_t, uint32_t *data_flags) -> ssize_t {
          *data_flags |= NGHTTP2_DATA_FLAG_EOF;
          *data_flags |= NGHTTP2_DATA_FLAG_NO_END_STREAM;
          res.write_trailer({{"result", {"trailers-only", false}}});
          return 0;
        });
      }));

  boost::system::error_code ec;
  CHECK(!server.listen_and_serve(ec, "127.0.0.1", "0", true));
  CHECK(!ec);
  const int port = server.ports()[0];

  std::thread io_thread([&]() { io_context.run(); });

  boost::asio::io_context client_io;

  auto run_case = [&](const std::string &path,
                      const std::string &expected_trailer_key,
                      const std::string &expected_trailer_value,
                      bool expect_body) {
    boost::system::error_code client_ec;
    client::session sess(client_io, "127.0.0.1", std::to_string(port));
    sess.read_timeout(5s);

    std::vector<std::string> events;
    std::mutex events_mutex;
    std::atomic<bool> connected{false};
    std::atomic<bool> done{false};

    sess.on_connect([&](boost::asio::ip::tcp::endpoint) { connected = true; });

    const auto connect_deadline = std::chrono::steady_clock::now() + 5s;
    while (!connected && std::chrono::steady_clock::now() < connect_deadline) {
      client_io.run_one();
    }
    CHECK(connected);

    const client::request *req = sess.submit(
        client_ec, "GET", request_uri(port, path), header_map{});
    CHECK(!client_ec);
    CHECK(req != nullptr);

    req->on_response([&](const client::response &res) {
      {
        std::lock_guard<std::mutex> lock(events_mutex);
        events.push_back("response");
      }
      res.on_data([&](const uint8_t *data, std::size_t len) {
        std::lock_guard<std::mutex> lock(events_mutex);
        if (len == 0) {
          events.push_back("body_eof");
        } else {
          events.push_back("body_data");
          CHECK(len == 4);
          CHECK(std::string(reinterpret_cast<const char *>(data), len) ==
                "body");
        }
      });
      res.on_trailers([&](const header_map &trailers) {
        std::lock_guard<std::mutex> lock(events_mutex);
        events.push_back("trailers");
        auto range = trailers.equal_range(expected_trailer_key);
        CHECK(range.first != range.second);
        CHECK(range.first->second.value == expected_trailer_value);
      });
    });
    req->on_close([&](uint32_t) { done = true; });

    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!done && std::chrono::steady_clock::now() < deadline) {
      client_io.run_one();
    }
    CHECK(done);

    std::lock_guard<std::mutex> lock(events_mutex);
    CHECK(events.size() >= 3);
    CHECK(events[0] == "response");

    auto event_index = [&](const std::string &name) -> std::ptrdiff_t {
      CHECK_EVENTS(events, name);
      auto it = std::find(events.begin(), events.end(), name);
      return std::distance(events.begin(), it);
    };

    const auto response_idx = event_index("response");
    const auto body_eof_idx = event_index("body_eof");
    const auto trailers_idx = event_index("trailers");
    CHECK(response_idx < body_eof_idx);
    CHECK(body_eof_idx < trailers_idx);

    if (expect_body) {
      const auto body_data_idx = event_index("body_data");
      CHECK(response_idx < body_data_idx);
      CHECK(body_data_idx < body_eof_idx);
    }
  };

  run_case("/trailers", "checksum", "deadbeef", true);
  run_case("/trailers-only", "result", "trailers-only", false);

  server.stop();
  io_context.stop();
  io_thread.join();
}

void test_on_goaway() {
  boost::asio::io_context io_context;
  server::http2 server(io_context);

  CHECK(server.handle("/", [&](const server::request &, const server::response &res) {
    res.write_head(200);
    res.end("ok");
  }));

  boost::system::error_code ec;
  CHECK(!server.listen_and_serve(ec, "127.0.0.1", "0", true));
  CHECK(!ec);
  const int port = server.ports()[0];

  std::thread io_thread([&]() { io_context.run(); });

  boost::asio::io_context client_io;
  client::session sess(client_io, "127.0.0.1", std::to_string(port));
  sess.read_timeout(5s);

  std::atomic<bool> connected{false};

  sess.on_connect([&](boost::asio::ip::tcp::endpoint) { connected = true; });
  sess.on_goaway([](uint32_t, int32_t) {});

  const auto connect_deadline = std::chrono::steady_clock::now() + 5s;
  while (!connected && std::chrono::steady_clock::now() < connect_deadline) {
    client_io.run_one();
  }
  CHECK(connected);

  sess.shutdown();

  const auto shutdown_deadline = std::chrono::steady_clock::now() + 5s;
  while (std::chrono::steady_clock::now() < shutdown_deadline) {
    client_io.run_one_for(20ms);
  }

  server.stop();
  io_context.stop();
  io_thread.join();
}

} // namespace

int main() {
  test_shutdown_before_connect();
  test_external_io_context_lifecycle();
  test_response_trailers_and_ordering();
  test_on_goaway();
  std::cout << "All rpcpio feature tests passed." << std::endl;
  return 0;
}
