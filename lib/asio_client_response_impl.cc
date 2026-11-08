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
#include "asio_client_response_impl.h"

#include "template.h"

namespace nghttp2 {
namespace asio_http2 {
namespace client {

response_impl::response_impl()
    : content_length_(-1),
      header_buffer_size_(0),
      trailer_buffer_size_(0),
      status_code_(0),
      body_eof_delivered_(false),
      trailers_delivered_(false) {}

void response_impl::on_data(data_cb cb) { data_cb_ = std::move(cb); }

void response_impl::on_trailers(trailers_cb cb) {
  trailers_cb_ = std::move(cb);
}

void response_impl::call_on_trailers(const header_map &trailers) {
  if (trailers_delivered_ || !trailers_cb_) {
    return;
  }
  trailers_delivered_ = true;
  trailers_cb_(trailers);
}

void response_impl::call_on_data(const uint8_t *data, std::size_t len) {
  if (data_cb_) {
    data_cb_(data, len);
  }
}

void response_impl::deliver_body_eof() {
  if (body_eof_delivered_) {
    return;
  }
  body_eof_delivered_ = true;
  call_on_data(nullptr, 0);
}

void response_impl::status_code(int sc) { status_code_ = sc; }

int response_impl::status_code() const { return status_code_; }

void response_impl::content_length(int64_t n) { content_length_ = n; }

int64_t response_impl::content_length() const { return content_length_; }

header_map &response_impl::header() { return header_; }

const header_map &response_impl::header() const { return header_; }

header_map &response_impl::trailer() { return trailer_; }

const header_map &response_impl::trailer() const { return trailer_; }

size_t response_impl::header_buffer_size() const { return header_buffer_size_; }

void response_impl::update_header_buffer_size(size_t len) {
  header_buffer_size_ += len;
}

size_t response_impl::trailer_buffer_size() const {
  return trailer_buffer_size_;
}

void response_impl::update_trailer_buffer_size(size_t len) {
  trailer_buffer_size_ += len;
}

} // namespace client
} // namespace asio_http2
} // namespace nghttp2
