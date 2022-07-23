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
#include "http2.h"
#include "util.h"

namespace nghttp2 {

namespace http2 {

StringRef get_reason_phrase(unsigned int status_code) {
  switch (status_code) {
  case 100:
    return StringRef::from_lit("Continue");
  case 101:
    return StringRef::from_lit("Switching Protocols");
  case 103:
    return StringRef::from_lit("Early Hints");
  case 200:
    return StringRef::from_lit("OK");
  case 201:
    return StringRef::from_lit("Created");
  case 202:
    return StringRef::from_lit("Accepted");
  case 203:
    return StringRef::from_lit("Non-Authoritative Information");
  case 204:
    return StringRef::from_lit("No Content");
  case 205:
    return StringRef::from_lit("Reset Content");
  case 206:
    return StringRef::from_lit("Partial Content");
  case 300:
    return StringRef::from_lit("Multiple Choices");
  case 301:
    return StringRef::from_lit("Moved Permanently");
  case 302:
    return StringRef::from_lit("Found");
  case 303:
    return StringRef::from_lit("See Other");
  case 304:
    return StringRef::from_lit("Not Modified");
  case 305:
    return StringRef::from_lit("Use Proxy");
  // case 306: return StringRef::from_lit("(Unused)");
  case 307:
    return StringRef::from_lit("Temporary Redirect");
  case 308:
    return StringRef::from_lit("Permanent Redirect");
  case 400:
    return StringRef::from_lit("Bad Request");
  case 401:
    return StringRef::from_lit("Unauthorized");
  case 402:
    return StringRef::from_lit("Payment Required");
  case 403:
    return StringRef::from_lit("Forbidden");
  case 404:
    return StringRef::from_lit("Not Found");
  case 405:
    return StringRef::from_lit("Method Not Allowed");
  case 406:
    return StringRef::from_lit("Not Acceptable");
  case 407:
    return StringRef::from_lit("Proxy Authentication Required");
  case 408:
    return StringRef::from_lit("Request Timeout");
  case 409:
    return StringRef::from_lit("Conflict");
  case 410:
    return StringRef::from_lit("Gone");
  case 411:
    return StringRef::from_lit("Length Required");
  case 412:
    return StringRef::from_lit("Precondition Failed");
  case 413:
    return StringRef::from_lit("Payload Too Large");
  case 414:
    return StringRef::from_lit("URI Too Long");
  case 415:
    return StringRef::from_lit("Unsupported Media Type");
  case 416:
    return StringRef::from_lit("Requested Range Not Satisfiable");
  case 417:
    return StringRef::from_lit("Expectation Failed");
  case 421:
    return StringRef::from_lit("Misdirected Request");
  case 425:
    // https://tools.ietf.org/html/rfc8470
    return StringRef::from_lit("Too Early");
  case 426:
    return StringRef::from_lit("Upgrade Required");
  case 428:
    return StringRef::from_lit("Precondition Required");
  case 429:
    return StringRef::from_lit("Too Many Requests");
  case 431:
    return StringRef::from_lit("Request Header Fields Too Large");
  case 451:
    return StringRef::from_lit("Unavailable For Legal Reasons");
  case 500:
    return StringRef::from_lit("Internal Server Error");
  case 501:
    return StringRef::from_lit("Not Implemented");
  case 502:
    return StringRef::from_lit("Bad Gateway");
  case 503:
    return StringRef::from_lit("Service Unavailable");
  case 504:
    return StringRef::from_lit("Gateway Timeout");
  case 505:
    return StringRef::from_lit("HTTP Version Not Supported");
  case 511:
    return StringRef::from_lit("Network Authentication Required");
  default:
    return StringRef{};
  }
}

StringRef stringify_status(BlockAllocator &balloc, unsigned int status_code) {
  switch (status_code) {
  case 100:
    return StringRef::from_lit("100");
  case 101:
    return StringRef::from_lit("101");
  case 103:
    return StringRef::from_lit("103");
  case 200:
    return StringRef::from_lit("200");
  case 201:
    return StringRef::from_lit("201");
  case 202:
    return StringRef::from_lit("202");
  case 203:
    return StringRef::from_lit("203");
  case 204:
    return StringRef::from_lit("204");
  case 205:
    return StringRef::from_lit("205");
  case 206:
    return StringRef::from_lit("206");
  case 300:
    return StringRef::from_lit("300");
  case 301:
    return StringRef::from_lit("301");
  case 302:
    return StringRef::from_lit("302");
  case 303:
    return StringRef::from_lit("303");
  case 304:
    return StringRef::from_lit("304");
  case 305:
    return StringRef::from_lit("305");
  // case 306: return StringRef::from_lit("306");
  case 307:
    return StringRef::from_lit("307");
  case 308:
    return StringRef::from_lit("308");
  case 400:
    return StringRef::from_lit("400");
  case 401:
    return StringRef::from_lit("401");
  case 402:
    return StringRef::from_lit("402");
  case 403:
    return StringRef::from_lit("403");
  case 404:
    return StringRef::from_lit("404");
  case 405:
    return StringRef::from_lit("405");
  case 406:
    return StringRef::from_lit("406");
  case 407:
    return StringRef::from_lit("407");
  case 408:
    return StringRef::from_lit("408");
  case 409:
    return StringRef::from_lit("409");
  case 410:
    return StringRef::from_lit("410");
  case 411:
    return StringRef::from_lit("411");
  case 412:
    return StringRef::from_lit("412");
  case 413:
    return StringRef::from_lit("413");
  case 414:
    return StringRef::from_lit("414");
  case 415:
    return StringRef::from_lit("415");
  case 416:
    return StringRef::from_lit("416");
  case 417:
    return StringRef::from_lit("417");
  case 421:
    return StringRef::from_lit("421");
  case 426:
    return StringRef::from_lit("426");
  case 428:
    return StringRef::from_lit("428");
  case 429:
    return StringRef::from_lit("429");
  case 431:
    return StringRef::from_lit("431");
  case 451:
    return StringRef::from_lit("451");
  case 500:
    return StringRef::from_lit("500");
  case 501:
    return StringRef::from_lit("501");
  case 502:
    return StringRef::from_lit("502");
  case 503:
    return StringRef::from_lit("503");
  case 504:
    return StringRef::from_lit("504");
  case 505:
    return StringRef::from_lit("505");
  case 511:
    return StringRef::from_lit("511");
  default:
    return util::make_string_ref_uint(balloc, status_code);
  }
}

void copy_url_component(std::string &dest, const http_parser_url *u, int field,
                        const char *url) {
  if (u->field_set & (1 << field)) {
    dest.assign(url + u->field_data[field].off, u->field_data[field].len);
  }
}

namespace {
nghttp2_nv make_nv_internal(const std::string &name, const std::string &value,
                            bool no_index, uint8_t nv_flags) {
  uint8_t flags;

  flags =
      nv_flags | (no_index ? NGHTTP2_NV_FLAG_NO_INDEX : NGHTTP2_NV_FLAG_NONE);

  return {(uint8_t *)name.c_str(), (uint8_t *)value.c_str(), name.size(),
          value.size(), flags};
}
} // namespace

namespace {
nghttp2_nv make_nv_internal(const StringRef &name, const StringRef &value,
                            bool no_index, uint8_t nv_flags) {
  uint8_t flags;

  flags =
      nv_flags | (no_index ? NGHTTP2_NV_FLAG_NO_INDEX : NGHTTP2_NV_FLAG_NONE);

  return {(uint8_t *)name.c_str(), (uint8_t *)value.c_str(), name.size(),
          value.size(), flags};
}
} // namespace

nghttp2_nv make_nv(const std::string &name, const std::string &value,
                   bool no_index) {
  return make_nv_internal(name, value, no_index, NGHTTP2_NV_FLAG_NONE);
}

nghttp2_nv make_nv(const StringRef &name, const StringRef &value,
                   bool no_index) {
  return make_nv_internal(name, value, no_index, NGHTTP2_NV_FLAG_NONE);
}

// This function was generated by genheaderfunc.py.  Inspired by h2o
// header lookup.  https://github.com/h2o/h2o
int lookup_token(const uint8_t *name, size_t namelen) {
  switch (namelen) {
  case 2:
    switch (name[1]) {
    case 'e':
      if (util::streq_l("t", name, 1)) {
        return HD_TE;
      }
      break;
    }
    break;
  case 3:
    switch (name[2]) {
    case 'a':
      if (util::streq_l("vi", name, 2)) {
        return HD_VIA;
      }
      break;
    }
    break;
  case 4:
    switch (name[3]) {
    case 'e':
      if (util::streq_l("dat", name, 3)) {
        return HD_DATE;
      }
      break;
    case 'k':
      if (util::streq_l("lin", name, 3)) {
        return HD_LINK;
      }
      break;
    case 't':
      if (util::streq_l("hos", name, 3)) {
        return HD_HOST;
      }
      break;
    }
    break;
  case 5:
    switch (name[4]) {
    case 'h':
      if (util::streq_l(":pat", name, 4)) {
        return HD__PATH;
      }
      break;
    case 't':
      if (util::streq_l(":hos", name, 4)) {
        return HD__HOST;
      }
      break;
    }
    break;
  case 6:
    switch (name[5]) {
    case 'e':
      if (util::streq_l("cooki", name, 5)) {
        return HD_COOKIE;
      }
      break;
    case 'r':
      if (util::streq_l("serve", name, 5)) {
        return HD_SERVER;
      }
      break;
    case 't':
      if (util::streq_l("expec", name, 5)) {
        return HD_EXPECT;
      }
      break;
    }
    break;
  case 7:
    switch (name[6]) {
    case 'c':
      if (util::streq_l("alt-sv", name, 6)) {
        return HD_ALT_SVC;
      }
      break;
    case 'd':
      if (util::streq_l(":metho", name, 6)) {
        return HD__METHOD;
      }
      break;
    case 'e':
      if (util::streq_l(":schem", name, 6)) {
        return HD__SCHEME;
      }
      if (util::streq_l("upgrad", name, 6)) {
        return HD_UPGRADE;
      }
      break;
    case 'r':
      if (util::streq_l("traile", name, 6)) {
        return HD_TRAILER;
      }
      break;
    case 's':
      if (util::streq_l(":statu", name, 6)) {
        return HD__STATUS;
      }
      break;
    }
    break;
  case 8:
    switch (name[7]) {
    case 'n':
      if (util::streq_l("locatio", name, 7)) {
        return HD_LOCATION;
      }
      break;
    }
    break;
  case 9:
    switch (name[8]) {
    case 'd':
      if (util::streq_l("forwarde", name, 8)) {
        return HD_FORWARDED;
      }
      break;
    case 'l':
      if (util::streq_l(":protoco", name, 8)) {
        return HD__PROTOCOL;
      }
      break;
    }
    break;
  case 10:
    switch (name[9]) {
    case 'a':
      if (util::streq_l("early-dat", name, 9)) {
        return HD_EARLY_DATA;
      }
      break;
    case 'e':
      if (util::streq_l("keep-aliv", name, 9)) {
        return HD_KEEP_ALIVE;
      }
      break;
    case 'n':
      if (util::streq_l("connectio", name, 9)) {
        return HD_CONNECTION;
      }
      break;
    case 't':
      if (util::streq_l("user-agen", name, 9)) {
        return HD_USER_AGENT;
      }
      break;
    case 'y':
      if (util::streq_l(":authorit", name, 9)) {
        return HD__AUTHORITY;
      }
      break;
    }
    break;
  case 12:
    switch (name[11]) {
    case 'e':
      if (util::streq_l("content-typ", name, 11)) {
        return HD_CONTENT_TYPE;
      }
      break;
    }
    break;
  case 13:
    switch (name[12]) {
    case 'l':
      if (util::streq_l("cache-contro", name, 12)) {
        return HD_CACHE_CONTROL;
      }
      break;
    }
    break;
  case 14:
    switch (name[13]) {
    case 'h':
      if (util::streq_l("content-lengt", name, 13)) {
        return HD_CONTENT_LENGTH;
      }
      break;
    case 's':
      if (util::streq_l("http2-setting", name, 13)) {
        return HD_HTTP2_SETTINGS;
      }
      break;
    }
    break;
  case 15:
    switch (name[14]) {
    case 'e':
      if (util::streq_l("accept-languag", name, 14)) {
        return HD_ACCEPT_LANGUAGE;
      }
      break;
    case 'g':
      if (util::streq_l("accept-encodin", name, 14)) {
        return HD_ACCEPT_ENCODING;
      }
      break;
    case 'r':
      if (util::streq_l("x-forwarded-fo", name, 14)) {
        return HD_X_FORWARDED_FOR;
      }
      break;
    }
    break;
  case 16:
    switch (name[15]) {
    case 'n':
      if (util::streq_l("proxy-connectio", name, 15)) {
        return HD_PROXY_CONNECTION;
      }
      break;
    }
    break;
  case 17:
    switch (name[16]) {
    case 'e':
      if (util::streq_l("if-modified-sinc", name, 16)) {
        return HD_IF_MODIFIED_SINCE;
      }
      break;
    case 'g':
      if (util::streq_l("transfer-encodin", name, 16)) {
        return HD_TRANSFER_ENCODING;
      }
      break;
    case 'o':
      if (util::streq_l("x-forwarded-prot", name, 16)) {
        return HD_X_FORWARDED_PROTO;
      }
      break;
    case 'y':
      if (util::streq_l("sec-websocket-ke", name, 16)) {
        return HD_SEC_WEBSOCKET_KEY;
      }
      break;
    }
    break;
  case 20:
    switch (name[19]) {
    case 't':
      if (util::streq_l("sec-websocket-accep", name, 19)) {
        return HD_SEC_WEBSOCKET_ACCEPT;
      }
      break;
    }
    break;
  }
  return -1;
}

std::string path_join(const StringRef &base_path, const StringRef &base_query,
                      const StringRef &rel_path, const StringRef &rel_query) {
  BlockAllocator balloc(1024, 1024);

  return path_join(balloc, base_path, base_query, rel_path, rel_query).str();
}

bool expect_response_body(int status_code) {
  return status_code == 101 ||
         (status_code / 100 != 1 && status_code != 304 && status_code != 204);
}

bool expect_response_body(const std::string &method, int status_code) {
  return method != "HEAD" && expect_response_body(status_code);
}

namespace {
template <typename InputIt> InputIt eat_file(InputIt first, InputIt last) {
  if (first == last) {
    *first++ = '/';
    return first;
  }

  if (*(last - 1) == '/') {
    return last;
  }

  auto p = last;
  for (; p != first && *(p - 1) != '/'; --p)
    ;
  if (p == first) {
    // this should not happened in normal case, where we expect path
    // starts with '/'
    *first++ = '/';
    return first;
  }

  return p;
}
} // namespace

namespace {
template <typename InputIt> InputIt eat_dir(InputIt first, InputIt last) {
  auto p = eat_file(first, last);

  --p;

  assert(*p == '/');

  return eat_file(first, p);
}
} // namespace

StringRef path_join(BlockAllocator &balloc, const StringRef &base_path,
                    const StringRef &base_query, const StringRef &rel_path,
                    const StringRef &rel_query) {
  auto res = make_byte_ref(
      balloc, std::max(static_cast<size_t>(1), base_path.size()) +
                  rel_path.size() + 1 +
                  std::max(base_query.size(), rel_query.size()) + 1);
  auto p = res.base;

  if (rel_path.empty()) {
    if (base_path.empty()) {
      *p++ = '/';
    } else {
      p = std::copy(std::begin(base_path), std::end(base_path), p);
    }
    if (rel_query.empty()) {
      if (!base_query.empty()) {
        *p++ = '?';
        p = std::copy(std::begin(base_query), std::end(base_query), p);
      }
      *p = '\0';
      return StringRef{res.base, p};
    }
    *p++ = '?';
    p = std::copy(std::begin(rel_query), std::end(rel_query), p);
    *p = '\0';
    return StringRef{res.base, p};
  }

  auto first = std::begin(rel_path);
  auto last = std::end(rel_path);

  if (rel_path[0] == '/') {
    *p++ = '/';
    ++first;
    for (; first != last && *first == '/'; ++first)
      ;
  } else if (base_path.empty()) {
    *p++ = '/';
  } else {
    p = std::copy(std::begin(base_path), std::end(base_path), p);
  }

  for (; first != last;) {
    if (*first == '.') {
      if (first + 1 == last) {
        break;
      }
      if (*(first + 1) == '/') {
        first += 2;
        continue;
      }
      if (*(first + 1) == '.') {
        if (first + 2 == last) {
          p = eat_dir(res.base, p);
          break;
        }
        if (*(first + 2) == '/') {
          p = eat_dir(res.base, p);
          first += 3;
          continue;
        }
      }
    }
    if (*(p - 1) != '/') {
      p = eat_file(res.base, p);
    }
    auto slash = std::find(first, last, '/');
    if (slash == last) {
      p = std::copy(first, last, p);
      break;
    }
    p = std::copy(first, slash + 1, p);
    first = slash + 1;
    for (; first != last && *first == '/'; ++first)
      ;
  }
  if (!rel_query.empty()) {
    *p++ = '?';
    p = std::copy(std::begin(rel_query), std::end(rel_query), p);
  }
  *p = '\0';
  return StringRef{res.base, p};
}

} // namespace http2

} // namespace nghttp2
