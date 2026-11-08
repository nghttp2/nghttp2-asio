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
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SERVER_CONNECTION_H
#define ASIO_SERVER_CONNECTION_H

#include "nghttp2_config.h"

#include <array>
#include <chrono>
#include <memory>

#include <nghttp2/asio_http2_server.h>

#include "asio_server_http2_handler.h"
#include "asio_server_serve_mux.h"
#include "util.h"
#include "template.h"

namespace nghttp2 {

namespace asio_http2 {

namespace server {

// Returns a new reference to the peer certificate of a completed TLS
// handshake, or nullptr if the peer sent none or it failed verification.
inline X509 *verified_peer_certificate(boost::asio::ip::tcp::socket &) {
  return nullptr;
}

inline X509 *verified_peer_certificate(
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &socket) {
  SSL *ssl = socket.native_handle();
  if (SSL_get_verify_result(ssl) != X509_V_OK) {
    return nullptr;
  }
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(OPENSSL_IS_BORINGSSL)
  return SSL_get1_peer_certificate(ssl);
#else
  return SSL_get_peer_certificate(ssl);
#endif
}

template <typename socket_type>
class connection : public std::enable_shared_from_this<connection<socket_type>> {
public:
  connection(const connection &) = delete;
  connection &operator=(const connection &) = delete;

  template <typename... SocketArgs>
  explicit connection(serve_mux &mux,
                      std::chrono::nanoseconds tls_handshake_timeout,
                      std::chrono::nanoseconds read_timeout,
                      SocketArgs &&...args)
      : socket_(std::forward<SocketArgs>(args)...),
        mux_(mux),
        deadline_(socket_.lowest_layer().get_executor()),
        tls_handshake_timeout_(tls_handshake_timeout),
        read_timeout_(read_timeout),
        writing_(false),
        stopped_(false) {}

  void start() {
    boost::system::error_code ec;

    // The handler outlives this connection when a write signal is still
    // queued on the executor after stop(). A weak reference lets that late
    // write become a no-op; a raw `this` would call do_write() on a destroyed
    // connection, and a strong reference would form a cycle through handler_.
    handler_ = std::make_shared<http2_handler>(
        socket_.lowest_layer().get_executor(),
        socket_.lowest_layer().remote_endpoint(ec),
        [weak_self = std::weak_ptr<connection>(this->shared_from_this())]() {
          if (auto self = weak_self.lock()) {
            self->do_write();
          }
        },
        mux_);
    handler_->peer_certificate(verified_peer_certificate(socket_));
    if (handler_->start() != 0) {
      stop();
      return;
    }
    do_read();
  }

  socket_type &socket() { return socket_; }

  void start_tls_handshake_deadline() {
    deadline_.expires_after(tls_handshake_timeout_);
    deadline_.async_wait(
        [self = this->shared_from_this()](const boost::system::error_code &) {
          self->handle_deadline();
        });
  }

  void start_read_deadline() {
    deadline_.expires_after(read_timeout_);
    deadline_.async_wait(
        [self = this->shared_from_this()](const boost::system::error_code &) {
          self->handle_deadline();
        });
  }

  void handle_deadline() {
    if (stopped_) {
      return;
    }

    if (deadline_.expiry() <= boost::asio::steady_timer::clock_type::now()) {
      stop();
      return;
    }

    deadline_.async_wait(
        [self = this->shared_from_this()](const boost::system::error_code &) {
          self->handle_deadline();
        });
  }

  void do_read() {
    auto self = this->shared_from_this();

    deadline_.expires_after(read_timeout_);

    socket_.async_read_some(
        boost::asio::buffer(buffer_),
        [this, self](const boost::system::error_code &e,
                     std::size_t bytes_transferred) {
          if (e) {
            stop();
            return;
          }

          if (handler_->on_read(buffer_, bytes_transferred) != 0) {
            stop();
            return;
          }

          do_write();

          if (!writing_ && handler_->should_stop()) {
            stop();
            return;
          }

          do_read();
        });
  }

  void do_write() {
    auto self = this->shared_from_this();

    if (writing_) {
      return;
    }

    int rv;
    std::size_t nwrite;

    rv = handler_->on_write(outbuf_, nwrite);

    if (rv != 0) {
      stop();
      return;
    }

    if (nwrite == 0) {
      if (handler_->should_stop()) {
        stop();
      }
      return;
    }

    writing_ = true;

    deadline_.expires_after(read_timeout_);

    boost::asio::async_write(
        socket_, boost::asio::buffer(outbuf_, nwrite),
        [this, self](const boost::system::error_code &e, std::size_t) {
          if (e) {
            stop();
            return;
          }

          writing_ = false;

          do_write();
        });
  }

  void stop() {
    if (stopped_) {
      return;
    }

    stopped_ = true;
    boost::system::error_code ignored_ec;
    socket_.lowest_layer().close(ignored_ec);
    deadline_.cancel();
  }

private:
  socket_type socket_;

  serve_mux &mux_;

  std::shared_ptr<http2_handler> handler_;

  std::array<uint8_t, 8_k> buffer_;
  std::array<uint8_t, 64_k> outbuf_;

  boost::asio::steady_timer deadline_;
  std::chrono::nanoseconds tls_handshake_timeout_;
  std::chrono::nanoseconds read_timeout_;

  bool writing_;
  bool stopped_;
};

} // namespace server

} // namespace asio_http2

} // namespace nghttp2

#endif // ASIO_SERVER_CONNECTION_H
