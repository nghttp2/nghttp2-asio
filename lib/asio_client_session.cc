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
#include "nghttp2_config.h"

#include <nghttp2/asio_http2_client.h>

#include "asio_client_session_tcp_impl.h"
#include "asio_client_session_tls_impl.h"
#include "asio_common.h"
#include "template.h"

namespace nghttp2 {
namespace asio_http2 {
namespace client {

using boost::asio::ip::tcp;

// The io_context-based constructors are convenience wrappers around the
// executor-based ones below; see the header comment for why the executor
// ones are what actually matter for thread-safety.
session::session(boost::asio::io_context &io_context, const std::string &host,
                 const std::string &service)
    : session(io_context.get_executor(), host, service) {}

session::session(boost::asio::io_context &io_context,
                 const boost::asio::ip::tcp::endpoint &local_endpoint,
                 const std::string &host, const std::string &service)
    : session(io_context.get_executor(), local_endpoint, host, service) {}

session::session(boost::asio::io_context &io_context, const std::string &host,
                 const std::string &service,
                 std::chrono::nanoseconds connect_timeout)
    : session(io_context.get_executor(), host, service, connect_timeout) {}

session::session(boost::asio::io_context &io_context,
                 const boost::asio::ip::tcp::endpoint &local_endpoint,
                 const std::string &host, const std::string &service,
                 std::chrono::nanoseconds connect_timeout)
    : session(io_context.get_executor(), local_endpoint, host, service,
              connect_timeout) {}

session::session(boost::asio::io_context &io_context,
                 boost::asio::ssl::context &tls_ctx, const std::string &host,
                 const std::string &service)
    : session(io_context.get_executor(), tls_ctx, host, service) {}

session::session(boost::asio::io_context &io_context,
                 boost::asio::ssl::context &tls_ctx, const std::string &host,
                 const std::string &service,
                 std::chrono::nanoseconds connect_timeout)
    : session(io_context.get_executor(), tls_ctx, host, service,
              connect_timeout) {}

session::session(const boost::asio::any_io_executor &ex,
                 const std::string &host, const std::string &service)
    : impl_(std::make_shared<session_tcp_impl>(
          ex, host, service, std::chrono::seconds(60))) {
  impl_->start_resolve(host, service);
}

session::session(const boost::asio::any_io_executor &ex,
                 const boost::asio::ip::tcp::endpoint &local_endpoint,
                 const std::string &host, const std::string &service)
    : impl_(std::make_shared<session_tcp_impl>(
          ex, local_endpoint, host, service, std::chrono::seconds(60))) {
  impl_->start_resolve(host, service);
}

session::session(const boost::asio::any_io_executor &ex,
                 const std::string &host, const std::string &service,
                 std::chrono::nanoseconds connect_timeout)
    : impl_(std::make_shared<session_tcp_impl>(ex, host, service,
                                               connect_timeout)) {
  impl_->start_resolve(host, service);
}

session::session(const boost::asio::any_io_executor &ex,
                 const boost::asio::ip::tcp::endpoint &local_endpoint,
                 const std::string &host, const std::string &service,
                 std::chrono::nanoseconds connect_timeout)
    : impl_(std::make_shared<session_tcp_impl>(ex, local_endpoint, host,
                                               service, connect_timeout)) {
  impl_->start_resolve(host, service);
}

session::session(const boost::asio::any_io_executor &ex,
                 boost::asio::ssl::context &tls_ctx, const std::string &host,
                 const std::string &service)
    : impl_(std::make_shared<session_tls_impl>(
          ex, tls_ctx, host, service, std::chrono::seconds(60))) {
  impl_->start_resolve(host, service);
}

session::session(const boost::asio::any_io_executor &ex,
                 boost::asio::ssl::context &tls_ctx, const std::string &host,
                 const std::string &service,
                 std::chrono::nanoseconds connect_timeout)
    : impl_(std::make_shared<session_tls_impl>(ex, tls_ctx, host, service,
                                               connect_timeout)) {
  impl_->start_resolve(host, service);
}

session::~session() {}

session::session(session &&other) noexcept : impl_(std::move(other.impl_)) {}

session &session::operator=(session &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  impl_ = std::move(other.impl_);
  return *this;
}

void session::on_connect(connect_cb cb) const {
  impl_->on_connect(std::move(cb));
}

void session::on_error(error_cb cb) const { impl_->on_error(std::move(cb)); }

void session::on_goaway(goaway_cb cb) const {
  impl_->on_goaway(std::move(cb));
}

void session::shutdown() const { impl_->shutdown(); }

boost::asio::io_context &session::io_context() const {
  return impl_->io_context();
}

const request *session::submit(boost::system::error_code &ec,
                               const std::string &method,
                               const std::string &uri, header_map h,
                               priority_spec prio) const {
  return impl_->submit(ec, method, uri, generator_cb(), std::move(h),
                       std::move(prio));
}

const request *session::submit(boost::system::error_code &ec,
                               const std::string &method,
                               const std::string &uri, std::string data,
                               header_map h, priority_spec prio) const {
  return impl_->submit(ec, method, uri, string_generator(std::move(data)),
                       std::move(h), std::move(prio));
}

const request *session::submit(boost::system::error_code &ec,
                               const std::string &method,
                               const std::string &uri, generator_cb cb,
                               header_map h, priority_spec prio) const {
  return impl_->submit(ec, method, uri, std::move(cb), std::move(h),
                       std::move(prio));
}

void session::read_timeout(std::chrono::nanoseconds t) {
  impl_->read_timeout(t);
}

priority_spec::priority_spec(const int32_t stream_id, const int32_t weight,
                             const bool exclusive)
    : valid_(true) {
  nghttp2_priority_spec_init(&spec_, stream_id, weight, exclusive);
}

const nghttp2_priority_spec *priority_spec::get() const {
  if (!valid_) {
    return nullptr;
  }

  return &spec_;
}

bool priority_spec::valid() const { return valid_; }

} // namespace client
} // namespace asio_http2
} // namespace nghttp2
