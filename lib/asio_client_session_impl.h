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
#ifndef ASIO_CLIENT_SESSION_IMPL_H
#define ASIO_CLIENT_SESSION_IMPL_H

#include "nghttp2_config.h"

#include <array>
#include <chrono>

#include <nghttp2/asio_http2_client.h>

#include "template.h"

namespace nghttp2 {
namespace asio_http2 {
namespace client {

class stream;

using boost::asio::ip::tcp;

class session_impl : public std::enable_shared_from_this<session_impl> {
public:
  // ex is the executor every socket, timer, and resolver this session owns
  // is constructed with, and the only executor any of this session's
  // internal handlers are ever dispatched through. Passing a strand (or any
  // other serializing executor) here is what makes an rpcpio-style caller's
  // multi-threaded io_context safe to run a session on: nghttp2 sessions
  // are not thread-safe, so without this, two of the caller's io_context
  // threads could enter the same session concurrently.
  session_impl(const boost::asio::any_io_executor &ex,
               std::chrono::nanoseconds connect_timeout);
  virtual ~session_impl();

  void start_resolve(const std::string &host, const std::string &service);

  void connected(tcp::endpoint endpoint);
  void not_connected(const boost::system::error_code &ec);

  void on_connect(connect_cb cb);
  void on_error(error_cb cb);
  void on_goaway(goaway_cb cb);

  const connect_cb &on_connect() const;
  const error_cb &on_error() const;
  const goaway_cb &on_goaway() const;
  void call_on_goaway(uint32_t error_code, int32_t last_stream_id);

  int write_trailer(stream &strm, header_map h);

  void cancel(stream &strm, uint32_t error_code);
  void resume(stream &strm);

  std::unique_ptr<stream> create_stream();
  std::unique_ptr<stream> pop_stream(int32_t stream_id);
  stream *create_push_stream(int32_t stream_id);
  stream *find_stream(int32_t stream_id);

  const request *submit(boost::system::error_code &ec,
                        const std::string &method, const std::string &uri,
                        generator_cb cb, header_map h, priority_spec spec);

  virtual void start_connect(tcp::resolver::results_type results) = 0;
  virtual tcp::socket &socket() = 0;
  virtual void read_socket(
      std::function<void(const boost::system::error_code &ec, std::size_t n)>
          h) = 0;
  virtual void write_socket(
      std::function<void(const boost::system::error_code &ec, std::size_t n)>
          h) = 0;
  virtual void shutdown_socket() = 0;

  void shutdown();

  boost::asio::io_context &io_context();
  const boost::asio::any_io_executor &executor() const;

  void signal_write();

  void enter_callback();
  void leave_callback();

  void do_read();
  void do_write();

  void read_timeout(std::chrono::nanoseconds t);

  void stop();
  bool stopped() const;

protected:
  std::array<uint8_t, 8_k> rb_;
  std::array<uint8_t, 64_k> wb_;
  std::size_t wblen_;

private:
  bool should_stop() const;
  bool setup_session();
  void call_error_cb(const boost::system::error_code &ec);
  void handle_deadline();
  void start_ping();
  void handle_ping(const boost::system::error_code &ec);

  boost::asio::any_io_executor executor_;
  tcp::resolver resolver_;

  std::map<int32_t, std::unique_ptr<stream>> streams_;

  connect_cb connect_cb_;
  error_cb error_cb_;
  goaway_cb goaway_cb_;

  boost::asio::steady_timer deadline_;
  std::chrono::nanoseconds connect_timeout_;
  std::chrono::nanoseconds read_timeout_;

  boost::asio::steady_timer ping_;

  nghttp2_session *session_;

  const uint8_t *data_pending_;
  std::size_t data_pendinglen_;

  bool writing_;
  bool inside_callback_;
  bool stopped_;
};

} // namespace client
} // namespace asio_http2
} // namespace nghttp2

#endif // ASIO_CLIENT_SESSION_IMPL_H
