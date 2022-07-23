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
#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "nghttp2_config.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>
#include <functional>
#include <typeinfo>
#include <algorithm>
#include <ostream>
#include <utility>

namespace nghttp2 {

// std::forward is constexpr since C++14
template <typename... T>
constexpr std::array<
    typename std::decay<typename std::common_type<T...>::type>::type,
    sizeof...(T)>
make_array(T &&...t) {
  return std::array<
      typename std::decay<typename std::common_type<T...>::type>::type,
      sizeof...(T)>{{std::forward<T>(t)...}};
}

template <typename T, size_t N> constexpr size_t str_size(T (&)[N]) {
  return N - 1;
}

// inspired by <http://blog.korfuri.fr/post/go-defer-in-cpp/>, but our
// template can take functions returning other than void.
template <typename F, typename... T> struct Defer {
  Defer(F &&f, T &&...t)
      : f(std::bind(std::forward<F>(f), std::forward<T>(t)...)) {}
  Defer(Defer &&o) noexcept : f(std::move(o.f)) {}
  ~Defer() { f(); }

  using ResultType = typename std::result_of<typename std::decay<F>::type(
      typename std::decay<T>::type...)>::type;
  std::function<ResultType()> f;
};

template <typename F, typename... T> Defer<F, T...> defer(F &&f, T &&...t) {
  return Defer<F, T...>(std::forward<F>(f), std::forward<T>(t)...);
}

// User-defined literals for K, M, and G (powers of 1024)

constexpr unsigned long long operator"" _k(unsigned long long k) {
  return k * 1024;
}

constexpr unsigned long long operator"" _m(unsigned long long m) {
  return m * 1024 * 1024;
}

constexpr unsigned long long operator"" _g(unsigned long long g) {
  return g * 1024 * 1024 * 1024;
}

// StringRef is a reference to a string owned by something else.  So
// it behaves like simple string, but it does not own pointer.  When
// it is default constructed, it has empty string.  You can freely
// copy or move around this struct, but never free its pointer.  str()
// function can be used to export the content as std::string.
class StringRef {
public:
  using traits_type = std::char_traits<char>;
  using value_type = traits_type::char_type;
  using allocator_type = std::allocator<char>;
  using size_type = std::allocator_traits<allocator_type>::size_type;
  using difference_type =
      std::allocator_traits<allocator_type>::difference_type;
  using const_reference = const value_type &;
  using const_pointer = const value_type *;
  using const_iterator = const_pointer;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr StringRef() : base(""), len(0) {}
  explicit StringRef(const std::string &s) : base(s.c_str()), len(s.size()) {}
  explicit StringRef(const char *s) : base(s), len(strlen(s)) {}
  constexpr StringRef(const char *s, size_t n) : base(s), len(n) {}
  template <typename CharT>
  constexpr StringRef(const CharT *s, size_t n)
      : base(reinterpret_cast<const char *>(s)), len(n) {}
  template <typename InputIt>
  StringRef(InputIt first, InputIt last)
      : base(reinterpret_cast<const char *>(&*first)),
        len(std::distance(first, last)) {}
  template <typename InputIt>
  StringRef(InputIt *first, InputIt *last)
      : base(reinterpret_cast<const char *>(first)),
        len(std::distance(first, last)) {}
  template <typename CharT, size_t N>
  constexpr static StringRef from_lit(const CharT (&s)[N]) {
    return StringRef{s, N - 1};
  }
  static StringRef from_maybe_nullptr(const char *s) {
    if (s == nullptr) {
      return StringRef();
    }

    return StringRef(s);
  }

  constexpr const_iterator begin() const { return base; };
  constexpr const_iterator cbegin() const { return base; };

  constexpr const_iterator end() const { return base + len; };
  constexpr const_iterator cend() const { return base + len; };

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{base + len};
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator{base + len};
  }

  const_reverse_iterator rend() const { return const_reverse_iterator{base}; }
  const_reverse_iterator crend() const { return const_reverse_iterator{base}; }

  constexpr const char *c_str() const { return base; }
  constexpr size_type size() const { return len; }
  constexpr bool empty() const { return len == 0; }
  constexpr const_reference operator[](size_type pos) const {
    return *(base + pos);
  }

  std::string str() const { return std::string(base, len); }
  const uint8_t *byte() const {
    return reinterpret_cast<const uint8_t *>(base);
  }

private:
  const char *base;
  size_type len;
};

inline bool operator==(const StringRef &lhs, const StringRef &rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

inline bool operator==(const StringRef &lhs, const std::string &rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

inline bool operator==(const std::string &lhs, const StringRef &rhs) {
  return rhs == lhs;
}

inline bool operator==(const StringRef &lhs, const char *rhs) {
  return lhs.size() == strlen(rhs) &&
         std::equal(std::begin(lhs), std::end(lhs), rhs);
}

inline bool operator==(const char *lhs, const StringRef &rhs) {
  return rhs == lhs;
}

inline bool operator!=(const StringRef &lhs, const StringRef &rhs) {
  return !(lhs == rhs);
}

inline bool operator!=(const StringRef &lhs, const std::string &rhs) {
  return !(lhs == rhs);
}

inline bool operator!=(const std::string &lhs, const StringRef &rhs) {
  return !(rhs == lhs);
}

inline bool operator!=(const StringRef &lhs, const char *rhs) {
  return !(lhs == rhs);
}

inline bool operator!=(const char *lhs, const StringRef &rhs) {
  return !(rhs == lhs);
}

inline bool operator<(const StringRef &lhs, const StringRef &rhs) {
  return std::lexicographical_compare(std::begin(lhs), std::end(lhs),
                                      std::begin(rhs), std::end(rhs));
}

inline std::ostream &operator<<(std::ostream &o, const StringRef &s) {
  return o.write(s.c_str(), s.size());
}

inline std::string &operator+=(std::string &lhs, const StringRef &rhs) {
  lhs.append(rhs.c_str(), rhs.size());
  return lhs;
}

} // namespace nghttp2

namespace std {
template <> struct hash<nghttp2::StringRef> {
  std::size_t operator()(const nghttp2::StringRef &s) const noexcept {
    // 32 bit FNV-1a:
    // https://tools.ietf.org/html/draft-eastlake-fnv-16#section-6.1.1
    uint32_t h = 2166136261u;
    for (auto c : s) {
      h ^= static_cast<uint8_t>(c);
      h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }
    return h;
  }
};
} // namespace std

#endif // TEMPLATE_H
