/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2014 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
// We wrote this code based on the original code which has the
// following license:
//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "asio_server.h"

#include "asio_server_connection.h"
#include "asio_common.h"
#include "util.h"

namespace nghttp2 {
namespace asio_http2 {
namespace server {

server::server(std::size_t io_context_pool_size,
               std::chrono::nanoseconds tls_handshake_timeout,
               std::chrono::nanoseconds read_timeout)
    : uses_external_io_context_(false),
      io_context_pool_(std::make_unique<io_service_pool>(io_context_pool_size)),
      external_io_context_(nullptr),
      tls_handshake_timeout_(tls_handshake_timeout),
      read_timeout_(read_timeout) {}

server::server(boost::asio::io_context &io_context,
               std::chrono::nanoseconds tls_handshake_timeout,
               std::chrono::nanoseconds read_timeout)
    : uses_external_io_context_(true),
      io_context_pool_(nullptr),
      external_io_context_(&io_context),
      tls_handshake_timeout_(tls_handshake_timeout),
      read_timeout_(read_timeout) {
  external_io_contexts_.push_back(
      std::shared_ptr<boost::asio::io_context>(&io_context, [](auto *) {}));
}

boost::asio::io_context &server::get_io_context() {
  if (uses_external_io_context_) {
    return *external_io_context_;
  }
  return io_context_pool_->get_io_context();
}

void server::track_connection(std::function<void()> stop_fn) {
  std::lock_guard<std::mutex> lock(connection_closers_mutex_);
  connection_closers_.push_back(std::move(stop_fn));
}

void server::close_connections() {
  std::vector<std::function<void()>> closers;
  {
    std::lock_guard<std::mutex> lock(connection_closers_mutex_);
    closers.swap(connection_closers_);
  }
  for (auto &fn : closers) {
    fn();
  }
}

boost::system::error_code
server::listen_and_serve(boost::system::error_code &ec,
                         boost::asio::ssl::context *tls_context,
                         const std::string &address, const std::string &port,
                         int backlog, serve_mux &mux, bool asynchronous) {
  ec.clear();

  if (bind_and_listen(ec, address, port, backlog)) {
    return ec;
  }

  for (auto &acceptor : acceptors_) {
    if (tls_context) {
      start_accept(*tls_context, acceptor, mux);
    } else {
      start_accept(acceptor, mux);
    }
  }

  if (uses_external_io_context_) {
    if (!asynchronous) {
      external_io_context_->run();
    }
  } else {
    io_context_pool_->run(asynchronous);
  }

  return ec;
}

boost::system::error_code server::bind_and_listen(boost::system::error_code &ec,
                                                  const std::string &address,
                                                  const std::string &port,
                                                  int backlog) {
  tcp::resolver resolver(get_io_context());
  auto results = resolver.resolve(address, port, ec);
  if (ec) {
    return ec;
  }

  for (const auto &result : results) {
    tcp::endpoint endpoint = result.endpoint();
    auto acceptor = tcp::acceptor(get_io_context());

    if (acceptor.open(endpoint.protocol(), ec)) {
      continue;
    }

    acceptor.set_option(tcp::acceptor::reuse_address(true));

    if (acceptor.bind(endpoint, ec)) {
      continue;
    }

    if (acceptor.listen(
            backlog == -1 ? boost::asio::socket_base::max_listen_connections
                          : backlog,
            ec)) {
      continue;
    }

    acceptors_.push_back(std::move(acceptor));
  }

  if (acceptors_.empty()) {
    return ec;
  }

  // ec could have some errors since we may have failed to bind some
  // interfaces.
  ec.clear();

  return ec;
}

void server::start_accept(boost::asio::ssl::context &tls_context,
                          tcp::acceptor &acceptor, serve_mux &mux) {

  {
    std::lock_guard<std::mutex> lock(acceptors_mutex_);
    if (!acceptor.is_open()) {
      return;
    }
  }

  // Each connection gets its own strand over get_io_context()'s executor.
  // With an external, caller-supplied io_context (the common rpcpio-style
  // case), that io_context may be driven by more than one thread; without
  // this, this connection's own socket read/write/handshake completions --
  // and every handler downstream of them -- could land on two different
  // threads at once, and nghttp2 sessions are not thread-safe against that.
  // Different connections still get different strands, so they still run
  // fully in parallel across those threads.
  boost::asio::strand<boost::asio::any_io_executor> conn_strand(
      get_io_context().get_executor());
  auto new_connection = std::make_shared<connection<ssl_socket>>(
      mux, tls_handshake_timeout_, read_timeout_, conn_strand,
      tls_context);

  // close_connections() (called from server::stop(), i.e. from whatever
  // thread calls Shutdown()) runs these closers directly; that thread is not
  // necessarily one that this connection's own strand ever runs on. Route
  // the actual stop() through conn_strand so it can never race this
  // connection's own in-flight read/write/handshake handlers.
  track_connection([w = std::weak_ptr<connection<ssl_socket>>(new_connection),
                    conn_strand]() {
    boost::asio::dispatch(conn_strand, [w]() {
      if (auto c = w.lock()) {
        c->stop();
      }
    });
  });

  // async_accept's completion handler is, by default, dispatched via the
  // acceptor's own executor -- not conn_strand, even though the socket it
  // operates on was constructed with conn_strand -- because it is the
  // acceptor's operation, and there is exactly one acceptor shared by every
  // connection. Touching the brand-new connection (set_option, starting the
  // handshake) from there is exactly the kind of access this whole change
  // is meant to confine, so bind_executor it onto conn_strand explicitly.
  // The trailing start_accept() re-arm call is unrelated to this
  // connection but harmless to also run on conn_strand once.
  std::lock_guard<std::mutex> accept_lock(acceptors_mutex_);
  if (!acceptor.is_open()) {
    return;
  }
  acceptor.async_accept(
      new_connection->socket().lowest_layer(),
      boost::asio::bind_executor(
          conn_strand,
          [this, &tls_context, &acceptor, &mux,
           new_connection](const boost::system::error_code &e) {
            if (!e) {
              boost::system::error_code option_ec;
              new_connection->socket().lowest_layer().set_option(
                  tcp::no_delay(true), option_ec);
              if (option_ec) {
                new_connection->stop();
                start_accept(tls_context, acceptor, mux);
                return;
              }
              new_connection->start_tls_handshake_deadline();
              new_connection->socket().async_handshake(
                  boost::asio::ssl::stream_base::server,
                  [new_connection](const boost::system::error_code &e) {
                    if (e) {
                      new_connection->stop();
                      return;
                    }

                    if (!tls_h2_negotiated(new_connection->socket())) {
                      new_connection->stop();
                      return;
                    }

                    new_connection->start();
                  });
            }

            start_accept(tls_context, acceptor, mux);
          }));
}

void server::start_accept(tcp::acceptor &acceptor, serve_mux &mux) {

  {
    std::lock_guard<std::mutex> lock(acceptors_mutex_);
    if (!acceptor.is_open()) {
      return;
    }
  }

  // See the TLS overload above for why this is a per-connection strand
  // rather than get_io_context() directly.
  boost::asio::strand<boost::asio::any_io_executor> conn_strand(
      get_io_context().get_executor());
  auto new_connection = std::make_shared<connection<tcp::socket>>(
      mux, tls_handshake_timeout_, read_timeout_, conn_strand);

  // See the TLS overload above for why this dispatches through conn_strand.
  track_connection([w = std::weak_ptr<connection<tcp::socket>>(new_connection),
                    conn_strand]() {
    boost::asio::dispatch(conn_strand, [w]() {
      if (auto c = w.lock()) {
        c->stop();
      }
    });
  });

  // See the TLS overload above for why this binds conn_strand explicitly.
  std::lock_guard<std::mutex> accept_lock(acceptors_mutex_);
  if (!acceptor.is_open()) {
    return;
  }
  acceptor.async_accept(
      new_connection->socket(),
      boost::asio::bind_executor(
          conn_strand,
          [this, &acceptor, &mux,
           new_connection](const boost::system::error_code &e) {
            if (!e) {
              boost::system::error_code option_ec;
              new_connection->socket().set_option(tcp::no_delay(true),
                                                  option_ec);
              if (option_ec) {
                new_connection->stop();
                start_accept(acceptor, mux);
                return;
              }
              new_connection->start_read_deadline();
              new_connection->start();
            }
            start_accept(acceptor, mux);
          }));
}

void server::stop_listening() {
  std::lock_guard<std::mutex> lock(acceptors_mutex_);
  for (auto &acceptor : acceptors_) {
    acceptor.close();
  }
}

void server::stop() {
  stop_listening();
  close_connections();
  if (!uses_external_io_context_) {
    io_context_pool_->stop();
  }
}

void server::join() {
  if (uses_external_io_context_) {
    return;
  }
  io_context_pool_->join();
}

const std::vector<std::shared_ptr<boost::asio::io_context>> &
server::io_contexts() const {
  if (uses_external_io_context_) {
    return external_io_contexts_;
  }
  return io_context_pool_->io_contexts();
}

const std::vector<int> server::ports() const {
  auto ports = std::vector<int>(acceptors_.size());
  auto index = 0;
  for (const auto &acceptor : acceptors_) {
    ports[index++] = acceptor.local_endpoint().port();
  }
  return ports;
}

} // namespace server
} // namespace asio_http2
} // namespace nghttp2
