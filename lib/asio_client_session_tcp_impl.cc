/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2015 Tatsuhiro Tsujikawa
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
#include "asio_client_session_tcp_impl.h"

namespace nghttp2 {
namespace asio_http2 {
namespace client {

session_tcp_impl::session_tcp_impl(
    const boost::asio::any_io_executor &ex, const std::string &host,
    const std::string &service, std::chrono::nanoseconds connect_timeout)
    : session_impl(ex, connect_timeout), socket_(ex) {}

session_tcp_impl::session_tcp_impl(
    const boost::asio::any_io_executor &ex,
    const boost::asio::ip::tcp::endpoint &local_endpoint,
    const std::string &host, const std::string &service,
    std::chrono::nanoseconds connect_timeout)
    : session_impl(ex, connect_timeout), socket_(ex) {
  socket_.open(local_endpoint.protocol());
  boost::asio::socket_base::reuse_address option(true);
  socket_.set_option(option);
  socket_.bind(local_endpoint);
}

session_tcp_impl::~session_tcp_impl() {}

void session_tcp_impl::start_connect(tcp::resolver::results_type results) {
  auto self = shared_from_this();
  boost::asio::async_connect(
      socket_, results,
      [self](const boost::system::error_code &ec, const tcp::endpoint &ep) {
        if (self->stopped()) {
          return;
        }

        if (ec) {
          self->not_connected(ec);
          return;
        }

        self->connected(ep);
      });
}

tcp::socket &session_tcp_impl::socket() { return socket_; }

void session_tcp_impl::read_socket(
    std::function<void(const boost::system::error_code &ec, std::size_t n)> h) {
  socket_.async_read_some(boost::asio::buffer(rb_), h);
}

void session_tcp_impl::write_socket(
    std::function<void(const boost::system::error_code &ec, std::size_t n)> h) {
  boost::asio::async_write(socket_, boost::asio::buffer(wb_, wblen_), h);
}

void session_tcp_impl::shutdown_socket() {
  boost::system::error_code ignored_ec;
  socket_.close(ignored_ec);
}

} // namespace client
} // namespace asio_http2
} // namespace nghttp2
