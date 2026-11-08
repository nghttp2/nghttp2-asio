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
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SERVER_H
#define ASIO_SERVER_H

#include "nghttp2_config.h"

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <memory>

#include <nghttp2/asio_http2_server.h>

#include "asio_io_service_pool.h"

namespace nghttp2 {

namespace asio_http2 {

namespace server {

class serve_mux;

using boost::asio::ip::tcp;

using ssl_socket = boost::asio::ssl::stream<tcp::socket>;

class server {
public:
  server(const server &) = delete;
  server &operator=(const server &) = delete;

  explicit server(std::size_t io_context_pool_size,
                  std::chrono::nanoseconds tls_handshake_timeout,
                  std::chrono::nanoseconds read_timeout);

  explicit server(boost::asio::io_context &io_context,
                  std::chrono::nanoseconds tls_handshake_timeout,
                  std::chrono::nanoseconds read_timeout);

  boost::system::error_code
  listen_and_serve(boost::system::error_code &ec,
                   boost::asio::ssl::context *tls_context,
                   const std::string &address, const std::string &port,
                   int backlog, serve_mux &mux, bool asynchronous = false);
  void join();
  void stop_listening();
  void stop();

  const std::vector<std::shared_ptr<boost::asio::io_context>> &
  io_contexts() const;

  const std::vector<int> ports() const;

private:
  void start_accept(tcp::acceptor &acceptor, serve_mux &mux);
  void start_accept(boost::asio::ssl::context &tls_context,
                    tcp::acceptor &acceptor, serve_mux &mux);

  boost::system::error_code bind_and_listen(boost::system::error_code &ec,
                                            const std::string &address,
                                            const std::string &port,
                                            int backlog);

  boost::asio::io_context &get_io_context();
  void track_connection(std::function<void()> stop_fn);
  void close_connections();

  bool uses_external_io_context_;
  std::unique_ptr<io_service_pool> io_context_pool_;
  boost::asio::io_context *external_io_context_;
  std::vector<std::shared_ptr<boost::asio::io_context>> external_io_contexts_;
  std::mutex connection_closers_mutex_;
  std::vector<std::function<void()>> connection_closers_;
  // Serializes is_open(), async_accept() and close() on each acceptor. Accept
  // completions re-arm from per-connection strands while stop_listening()
  // runs on whichever thread calls stop(); an acceptor object is not safe to
  // use from two threads at once.
  std::mutex acceptors_mutex_;
  std::vector<tcp::acceptor> acceptors_;
  std::unique_ptr<boost::asio::ssl::context> ssl_ctx_;

  std::chrono::nanoseconds tls_handshake_timeout_;
  std::chrono::nanoseconds read_timeout_;
};

} // namespace server

} // namespace asio_http2

} // namespace nghttp2

#endif // ASIO_SERVER_H
