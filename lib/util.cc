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
#include "util.h"

#ifdef HAVE_TIME_H
#  include <time.h>
#endif // HAVE_TIME_H
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif // HAVE_SYS_SOCKET_H
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif // HAVE_NETDB_H
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif // HAVE_FCNTL_H
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif // HAVE_NETINET_IN_H
#ifdef _WIN32
#  include <ws2tcpip.h>
#  include <boost/date_time/posix_time/posix_time.hpp>
#else // !_WIN32
#  include <netinet/tcp.h>
#endif // !_WIN32
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif // HAVE_ARPA_INET_H

#include <cmath>
#include <cerrno>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>

#include <nghttp2/nghttp2.h>

#include "timegm.h"

namespace nghttp2 {

namespace util {

#ifndef _WIN32
namespace {
int nghttp2_inet_pton(int af, const char *src, void *dst) {
  return inet_pton(af, src, dst);
}
} // namespace
#else // _WIN32
namespace {
// inet_pton-wrapper for Windows
int nghttp2_inet_pton(int af, const char *src, void *dst) {
#  if _WIN32_WINNT >= 0x0600
  return InetPtonA(af, src, dst);
#  else
  // the function takes a 'char*', so we need to make a copy
  char addr[INET6_ADDRSTRLEN + 1];
  strncpy(addr, src, sizeof(addr));
  addr[sizeof(addr) - 1] = 0;

  int size = sizeof(struct in6_addr);

  if (WSAStringToAddress(addr, af, nullptr, (LPSOCKADDR)dst, &size) == 0)
    return 1;
  return 0;
#  endif
}
} // namespace
#endif // _WIN32

namespace {
constexpr const char UPPER_XDIGITS[] = "0123456789ABCDEF";
} // namespace

bool in_rfc3986_unreserved_chars(const char c) {
  static constexpr char unreserved[] = {'-', '.', '_', '~'};
  return is_alpha(c) || is_digit(c) ||
         std::find(std::begin(unreserved), std::end(unreserved), c) !=
             std::end(unreserved);
}

bool in_rfc3986_sub_delims(const char c) {
  static constexpr char sub_delims[] = {'!', '$', '&', '\'', '(', ')',
                                        '*', '+', ',', ';',  '='};
  return std::find(std::begin(sub_delims), std::end(sub_delims), c) !=
         std::end(sub_delims);
}

std::string percent_encode_path(const std::string &s) {
  std::string dest;
  for (auto c : s) {
    if (in_rfc3986_unreserved_chars(c) || in_rfc3986_sub_delims(c) ||
        c == '/') {
      dest += c;
      continue;
    }

    dest += '%';
    dest += UPPER_XDIGITS[(c >> 4) & 0x0f];
    dest += UPPER_XDIGITS[(c & 0x0f)];
  }
  return dest;
}

uint32_t hex_to_uint(char c) {
  if (c <= '9') {
    return c - '0';
  }
  if (c <= 'Z') {
    return c - 'A' + 10;
  }
  if (c <= 'z') {
    return c - 'a' + 10;
  }
  return 256;
}

namespace {
template <typename Iterator>
Iterator cpydig(Iterator d, uint32_t n, size_t len) {
  auto p = d + len - 1;

  do {
    *p-- = (n % 10) + '0';
    n /= 10;
  } while (p >= d);

  return d + len;
}
} // namespace

namespace {
constexpr const char *MONTH[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr const char *DAY_OF_WEEK[] = {"Sun", "Mon", "Tue", "Wed",
                                       "Thu", "Fri", "Sat"};
} // namespace

std::string http_date(time_t t) {
  /* Sat, 27 Sep 2014 06:31:15 GMT */
  std::string res(29, 0);
  http_date(&res[0], t);
  return res;
}

char *http_date(char *res, time_t t) {
  struct tm tms;

  if (gmtime_r(&t, &tms) == nullptr) {
    return res;
  }

  auto p = res;

  auto s = DAY_OF_WEEK[tms.tm_wday];
  p = std::copy_n(s, 3, p);
  *p++ = ',';
  *p++ = ' ';
  p = cpydig(p, tms.tm_mday, 2);
  *p++ = ' ';
  s = MONTH[tms.tm_mon];
  p = std::copy_n(s, 3, p);
  *p++ = ' ';
  p = cpydig(p, tms.tm_year + 1900, 4);
  *p++ = ' ';
  p = cpydig(p, tms.tm_hour, 2);
  *p++ = ':';
  p = cpydig(p, tms.tm_min, 2);
  *p++ = ':';
  p = cpydig(p, tms.tm_sec, 2);
  s = " GMT";
  p = std::copy_n(s, 4, p);

  return p;
}

bool numeric_host(const char *hostname) {
  return numeric_host(hostname, AF_INET) || numeric_host(hostname, AF_INET6);
}

bool numeric_host(const char *hostname, int family) {
  int rv;
  std::array<uint8_t, sizeof(struct in6_addr)> dst;

  rv = nghttp2_inet_pton(family, hostname, dst.data());

  return rv == 1;
}

bool check_path(const std::string &path) {
  // We don't like '\' in path.
  return !path.empty() && path[0] == '/' &&
         path.find('\\') == std::string::npos &&
         path.find("/../") == std::string::npos &&
         path.find("/./") == std::string::npos &&
         !util::ends_with_l(path, "/..") && !util::ends_with_l(path, "/.");
}

bool check_h2_is_selected(const StringRef &proto) {
  return streq(NGHTTP2_H2, proto) || streq(NGHTTP2_H2_16, proto) ||
         streq(NGHTTP2_H2_14, proto);
}

namespace {
bool select_proto(const unsigned char **out, unsigned char *outlen,
                  const unsigned char *in, unsigned int inlen,
                  const StringRef &key) {
  for (auto p = in, end = in + inlen; p + key.size() <= end; p += *p + 1) {
    if (std::equal(std::begin(key), std::end(key), p)) {
      *out = p + 1;
      *outlen = *p;
      return true;
    }
  }
  return false;
}
} // namespace

bool select_h2(const unsigned char **out, unsigned char *outlen,
               const unsigned char *in, unsigned int inlen) {
  return select_proto(out, outlen, in, inlen, NGHTTP2_H2_ALPN) ||
         select_proto(out, outlen, in, inlen, NGHTTP2_H2_16_ALPN) ||
         select_proto(out, outlen, in, inlen, NGHTTP2_H2_14_ALPN);
}

std::vector<unsigned char> get_default_alpn() {
  auto res = std::vector<unsigned char>(NGHTTP2_H2_ALPN.size() +
                                        NGHTTP2_H2_16_ALPN.size() +
                                        NGHTTP2_H2_14_ALPN.size());
  auto p = std::begin(res);

  p = std::copy_n(std::begin(NGHTTP2_H2_ALPN), NGHTTP2_H2_ALPN.size(), p);
  p = std::copy_n(std::begin(NGHTTP2_H2_16_ALPN), NGHTTP2_H2_16_ALPN.size(), p);
  p = std::copy_n(std::begin(NGHTTP2_H2_14_ALPN), NGHTTP2_H2_14_ALPN.size(), p);

  return res;
}

bool ipv6_numeric_addr(const char *host) {
  uint8_t dst[16];
  return nghttp2_inet_pton(AF_INET6, host, dst) == 1;
}

namespace {
std::pair<int64_t, size_t> parse_uint_digits(const void *ss, size_t len) {
  const uint8_t *s = static_cast<const uint8_t *>(ss);
  int64_t n = 0;
  size_t i;
  if (len == 0) {
    return {-1, 0};
  }
  constexpr int64_t max = std::numeric_limits<int64_t>::max();
  for (i = 0; i < len; ++i) {
    if ('0' <= s[i] && s[i] <= '9') {
      if (n > max / 10) {
        return {-1, 0};
      }
      n *= 10;
      if (n > max - (s[i] - '0')) {
        return {-1, 0};
      }
      n += s[i] - '0';
      continue;
    }
    break;
  }
  if (i == 0) {
    return {-1, 0};
  }
  return {n, i};
}
} // namespace

int64_t parse_uint(const uint8_t *s, size_t len) {
  int64_t n;
  size_t i;
  std::tie(n, i) = parse_uint_digits(s, len);
  if (n == -1 || i != len) {
    return -1;
  }
  return n;
}

std::string dtos(double n) {
  auto m = llround(100. * n);
  auto f = utos(m % 100);
  return utos(m / 100) + "." + (f.size() == 1 ? "0" : "") + f;
}

} // namespace util

} // namespace nghttp2
