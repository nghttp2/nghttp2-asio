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
// io_service_pool.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "asio_io_service_pool.h"

namespace nghttp2 {

namespace asio_http2 {

io_service_pool::io_service_pool(std::size_t pool_size)
    : next_io_context_(0) {
  if (pool_size == 0) {
    throw std::runtime_error("io_service_pool size is 0");
  }

  for (std::size_t i = 0; i < pool_size; ++i) {
    auto ctx = std::make_shared<boost::asio::io_context>();
    work_.emplace_back(boost::asio::make_work_guard(*ctx));
    io_contexts_.push_back(std::move(ctx));
  }
}

void io_service_pool::run(bool asynchronous) {
  for (auto &ctx : io_contexts_) {
    futures_.push_back(std::async(std::launch::async, [ctx]() -> std::size_t {
      return ctx->run();
    }));
  }

  if (!asynchronous) {
    join();
  }
}

void io_service_pool::join() {
  for (auto &fut : futures_) {
    fut.get();
  }
}

void io_service_pool::force_stop() {
  for (auto &ctx : io_contexts_) {
    ctx->stop();
  }
}

void io_service_pool::stop() {
  work_.clear();
}

boost::asio::io_context &io_service_pool::get_io_context() {
  auto &ctx = *io_contexts_[next_io_context_];
  ++next_io_context_;
  if (next_io_context_ == io_contexts_.size()) {
    next_io_context_ = 0;
  }
  return ctx;
}

const std::vector<std::shared_ptr<boost::asio::io_context>> &
io_service_pool::io_contexts() const {
  return io_contexts_;
}

} // namespace asio_http2

} // namespace nghttp2
