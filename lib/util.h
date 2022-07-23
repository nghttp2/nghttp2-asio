/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2012 Tatsuhiro Tsujikawa
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
#ifndef UTIL_H
#define UTIL_H

#include "nghttp2_config.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif // HAVE_UNISTD_H
#include <getopt.h>
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif // HAVE_NETDB_H

#include <cmath>
#include <cstring>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <memory>
#include <chrono>
#include <map>
#include <random>

#include "url-parser/url_parser.h"

#include "template.h"
#include "network.h"
#include "allocator.h"

namespace nghttp2 {

constexpr auto NGHTTP2_H2_ALPN = StringRef::from_lit("\x2h2");
constexpr auto NGHTTP2_H2 = StringRef::from_lit("h2");

// The additional HTTP/2 protocol ALPN protocol identifier we also
// supports for our applications to make smooth migration into final
// h2 ALPN ID.
constexpr auto NGHTTP2_H2_16_ALPN = StringRef::from_lit("\x5h2-16");
constexpr auto NGHTTP2_H2_16 = StringRef::from_lit("h2-16");

constexpr auto NGHTTP2_H2_14_ALPN = StringRef::from_lit("\x5h2-14");
constexpr auto NGHTTP2_H2_14 = StringRef::from_lit("h2-14");

constexpr size_t NGHTTP2_MAX_UINT64_DIGITS = str_size("18446744073709551615");

namespace util {

inline bool is_alpha(const char c) {
  return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

inline bool is_digit(const char c) { return '0' <= c && c <= '9'; }

inline bool is_hex_digit(const char c) {
  return is_digit(c) || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f');
}

bool in_rfc3986_unreserved_chars(const char c);

bool in_rfc3986_sub_delims(const char c);

// Returns integer corresponding to hex notation |c|.  If
// is_hex_digit(c) is false, it returns 256.
uint32_t hex_to_uint(char c);

// percent-encode path component of URI |s|.
std::string percent_encode_path(const std::string &s);

template <typename InputIt>
std::string percent_decode(InputIt first, InputIt last) {
  std::string result;
  result.resize(last - first);
  auto p = std::begin(result);
  for (; first != last; ++first) {
    if (*first != '%') {
      *p++ = *first;
      continue;
    }

    if (first + 1 != last && first + 2 != last && is_hex_digit(*(first + 1)) &&
        is_hex_digit(*(first + 2))) {
      *p++ = (hex_to_uint(*(first + 1)) << 4) + hex_to_uint(*(first + 2));
      first += 2;
      continue;
    }

    *p++ = *first;
  }
  result.resize(p - std::begin(result));
  return result;
}

// Returns given time |t| from epoch in HTTP Date format (e.g., Mon,
// 10 Oct 2016 10:25:58 GMT).
std::string http_date(time_t t);
// Writes given time |t| from epoch in HTTP Date format into the
// buffer pointed by |res|.  The buffer must be at least 29 bytes
// long.  This function returns the one beyond the last position.
char *http_date(char *res, time_t t);

template <typename InputIterator1, typename InputIterator2>
bool starts_with(InputIterator1 first1, InputIterator1 last1,
                 InputIterator2 first2, InputIterator2 last2) {
  if (last1 - first1 < last2 - first2) {
    return false;
  }
  return std::equal(first2, last2, first1);
}

template <typename S, typename T> bool starts_with(const S &a, const T &b) {
  return starts_with(a.begin(), a.end(), b.begin(), b.end());
}

template <typename InputIterator1, typename InputIterator2>
bool ends_with(InputIterator1 first1, InputIterator1 last1,
               InputIterator2 first2, InputIterator2 last2) {
  if (last1 - first1 < last2 - first2) {
    return false;
  }
  return std::equal(first2, last2, last1 - (last2 - first2));
}

template <typename T, typename S> bool ends_with(const T &a, const S &b) {
  return ends_with(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T, typename CharT, size_t N>
bool ends_with_l(const T &a, const CharT (&b)[N]) {
  return ends_with(a.begin(), a.end(), b, b + N - 1);
}

template <typename InputIt1, typename InputIt2>
bool streq(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (std::distance(first1, last1) != std::distance(first2, last2)) {
    return false;
  }
  return std::equal(first1, last1, first2);
}

template <typename T, typename S> bool streq(const T &a, const S &b) {
  return streq(a.begin(), a.end(), b.begin(), b.end());
}

template <typename CharT, typename InputIt, size_t N>
bool streq_l(const CharT (&a)[N], InputIt b, size_t blen) {
  return streq(a, a + (N - 1), b, b + blen);
}

template <typename CharT, size_t N, typename T>
bool streq_l(const CharT (&a)[N], const T &b) {
  return streq(a, a + (N - 1), b.begin(), b.end());
}

// Returns string representation of |n| with 2 fractional digits.
std::string dtos(double n);

template <typename T> std::string utos(T n) {
  std::string res;
  if (n == 0) {
    res = "0";
    return res;
  }
  size_t nlen = 0;
  for (auto t = n; t; t /= 10, ++nlen)
    ;
  res.resize(nlen);
  for (; n; n /= 10) {
    res[--nlen] = (n % 10) + '0';
  }
  return res;
}

template <typename T, typename OutputIt> OutputIt utos(OutputIt dst, T n) {
  if (n == 0) {
    *dst++ = '0';
    return dst;
  }
  size_t nlen = 0;
  for (auto t = n; t; t /= 10, ++nlen)
    ;
  auto p = dst + nlen;
  auto res = p;
  for (; n; n /= 10) {
    *--p = (n % 10) + '0';
  }
  return res;
}

template <typename T>
StringRef make_string_ref_uint(BlockAllocator &balloc, T n) {
  auto iov = make_byte_ref(balloc, NGHTTP2_MAX_UINT64_DIGITS + 1);
  auto p = iov.base;
  p = util::utos(p, n);
  *p = '\0';
  return StringRef{iov.base, p};
}

bool numeric_host(const char *hostname);

bool numeric_host(const char *hostname, int family);

// Validates path so that it does not contain directory traversal
// vector.  Returns true if path is safe.  The |path| must start with
// "/" otherwise returns false.  This function should be called after
// percent-decode was performed.
bool check_path(const std::string &path);

// Returns true if ALPN ID |proto| is supported HTTP/2 protocol
// identifier.
bool check_h2_is_selected(const StringRef &proto);

// Selects h2 protocol ALPN ID if one of supported h2 versions are
// present in |in| of length inlen.  Returns true if h2 version is
// selected.
bool select_h2(const unsigned char **out, unsigned char *outlen,
               const unsigned char *in, unsigned int inlen);

// Returns default ALPN protocol list, which only contains supported
// HTTP/2 protocol identifier.
std::vector<unsigned char> get_default_alpn();

// Returns true if |host| is IPv6 numeric address (e.g., ::1)
bool ipv6_numeric_addr(const char *host);

// The following overload does not require |s| is NULL terminated.
int64_t parse_uint(const uint8_t *s, size_t len);

} // namespace util

} // namespace nghttp2

#endif // UTIL_H
