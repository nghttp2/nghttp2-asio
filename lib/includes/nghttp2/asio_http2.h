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
#ifndef ASIO_HTTP2_H
#define ASIO_HTTP2_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <map>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <nghttp2/nghttp2.h>

namespace boost {
  namespace asio {
    #if BOOST_VERSION >= 108700
      using io_service = boost::asio::io_context;
      using const_buffers_1 = boost::asio::const_buffer;
      using mutable_buffers_1 = boost::asio::mutable_buffer;
      using io_context_work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
      /// Cast a non-modifiable buffer to a specified pointer to POD type.
      template <typename PointerToPodType>
      inline PointerToPodType buffer_cast(const mutable_buffer& b) noexcept
      {
        return static_cast<PointerToPodType>(b.data());
      }

      /// Cast a non-modifiable buffer to a specified pointer to POD type.
      template <typename PointerToPodType>
      inline PointerToPodType buffer_cast(const const_buffer& b) noexcept
      {
        return static_cast<PointerToPodType>(b.data());
      }

      template <typename Protocol, typename Executor, typename Iterator,
          BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
            Iterator)) IteratorConnectToken = default_completion_token_t<Executor>>
      inline auto async_connect(basic_socket<Protocol, Executor>& s, Iterator begin,
          IteratorConnectToken&& token = default_completion_token_t<Executor>(),
          constraint_t<
            !is_endpoint_sequence<Iterator>::value
          > = 0,
          constraint_t<
            !is_same<Iterator, decay_t<IteratorConnectToken>>::value
          > = 0,
          constraint_t<
            !is_connect_condition<IteratorConnectToken, Iterator>::value
          > = 0)
        -> decltype(
          async_initiate<IteratorConnectToken,
            void (boost::system::error_code, Iterator)>(
              declval<detail::initiate_async_iterator_connect<Protocol, Executor>>(),
              token, begin, Iterator(),
              declval<detail::default_connect_condition>()))
      {
        return async_initiate<IteratorConnectToken,
          void (boost::system::error_code, Iterator)>(
            detail::initiate_async_iterator_connect<Protocol, Executor>(s),
            token, begin, Iterator(), detail::default_connect_condition());
      }
      #define NGHTTP2_ASIO_IO_CONTEXT_WORK_PARAM(IO_SERVICE) (IO_SERVICE)->get_executor()
    #else
      using io_context_work = boost::asio::io_service::work;
      #define NGHTTP2_ASIO_IO_CONTEXT_WORK_PARAM(IO_SERVICE) *(IO_SERVICE)
    #endif
  }
}

namespace nghttp2 {

namespace asio_http2 {

struct header_value {
  // header field value
  std::string value;
  // true if the header field value is sensitive information, such as
  // authorization information or short length secret cookies.  If
  // true, those header fields are not indexed by HPACK (but still
  // huffman-encoded), which results in lesser compression.
  bool sensitive;
};

// header fields.  The header field name must be lower-cased.
using header_map = std::multimap<std::string, header_value>;

const boost::system::error_category &nghttp2_category() noexcept;

struct uri_ref {
  std::string scheme;
  std::string host;
  // form after percent-encoding decoded
  std::string path;
  // original path, percent-encoded
  std::string raw_path;
  // original query, percent-encoded
  std::string raw_query;
  std::string fragment;
};

// Callback function when data is arrived.  EOF is indicated by
// passing 0 to the second parameter.
typedef std::function<void(const uint8_t *, std::size_t)> data_cb;
typedef std::function<void(void)> void_cb;
typedef std::function<void(const boost::system::error_code &ec)> error_cb;
// Callback function when request and response are finished.  The
// parameter indicates the cause of closure.
typedef std::function<void(uint32_t)> close_cb;

// Callback function to generate response body.  This function has the
// same semantics with nghttp2_data_source_read_callback.  Just source
// and user_data parameters are removed.
//
// Basically, write at most |len| bytes to |data| and returns the
// number of bytes written.  If there is no data left to send, set
// NGHTTP2_DATA_FLAG_EOF to *data_flags (e.g., *data_flags |=
// NGHTTP2_DATA_FLAG_EOF).  If there is still data to send but they
// are not available right now, return NGHTTP2_ERR_DEFERRED.  In case
// of the error and request/response must be closed, return
// NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE.
typedef std::function<ssize_t(uint8_t *buf, std::size_t len,
                              uint32_t *data_flags)>
    generator_cb;

// Convenient function to create function to read file denoted by
// |path|.  This can be passed to response::end().
generator_cb file_generator(const std::string &path);

// Like file_generator(const std::string&), but it takes opened file
// descriptor.  The passed descriptor will be closed when returned
// function object is destroyed.
generator_cb file_generator_from_fd(int fd);

// Validates path so that it does not contain directory traversal
// vector.  Returns true if path is safe.  The |path| must start with
// "/" otherwise returns false.  This function should be called after
// percent-decode was performed.
bool check_path(const std::string &path);

// Performs percent-decode against string |s|.
std::string percent_decode(const std::string &s);

// Returns HTTP date representation of current posix time |t|.
std::string http_date(int64_t t);

// Parses |uri| and extract scheme, host and service.  The service is
// port component of URI (e.g., "8443") if available, otherwise it is
// scheme (e.g., "https").
boost::system::error_code host_service_from_uri(boost::system::error_code &ec,
                                                std::string &scheme,
                                                std::string &host,
                                                std::string &service,
                                                const std::string &uri);

enum nghttp2_asio_error {
  NGHTTP2_ASIO_ERR_NO_ERROR = 0,
  NGHTTP2_ASIO_ERR_TLS_NO_APP_PROTO_NEGOTIATED = 1,
};

} // namespace asio_http2

} // namespace nghttp2

namespace boost {

namespace system {

template <> struct is_error_code_enum<nghttp2_error> {
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <> struct is_error_code_enum<nghttp2::asio_http2::nghttp2_asio_error> {
  BOOST_STATIC_CONSTANT(bool, value = true);
};

} // namespace system

} // namespace boost

#endif // ASIO_HTTP2_H
