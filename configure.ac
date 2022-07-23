dnl nghttp2 - HTTP/2 C Library

dnl Copyright (c) 2012, 2013, 2014, 2015 Tatsuhiro Tsujikawa

dnl Permission is hereby granted, free of charge, to any person obtaining
dnl a copy of this software and associated documentation files (the
dnl "Software"), to deal in the Software without restriction, including
dnl without limitation the rights to use, copy, modify, merge, publish,
dnl distribute, sublicense, and/or sell copies of the Software, and to
dnl permit persons to whom the Software is furnished to do so, subject to
dnl the following conditions:

dnl The above copyright notice and this permission notice shall be
dnl included in all copies or substantial portions of the Software.

dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
dnl EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
dnl NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
dnl LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
dnl OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
dnl WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

dnl Do not change user variables!
dnl https://www.gnu.org/software/automake/manual/html_node/Flag-Variables-Ordering.html

AC_PREREQ(2.61)
AC_INIT([nghttp2-asio], [0.1.0-DEV], [maintainer@nghttp2-asio])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

LT_PREREQ([2.2.6])
LT_INIT()

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl See versioning rule:
dnl  https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
AC_SUBST(LT_CURRENT, 0)
AC_SUBST(LT_REVISION, 0)
AC_SUBST(LT_AGE, 0)

major=`echo $PACKAGE_VERSION |cut -d. -f1 | sed -e "s/[^0-9]//g"`
minor=`echo $PACKAGE_VERSION |cut -d. -f2 | sed -e "s/[^0-9]//g"`
patch=`echo $PACKAGE_VERSION |cut -d. -f3 | cut -d- -f1 | sed -e "s/[^0-9]//g"`

PACKAGE_VERSION_NUM=`printf "0x%02x%02x%02x" "$major" "$minor" "$patch"`

AC_SUBST(PACKAGE_VERSION_NUM)

dnl Checks for command-line options
AC_ARG_ENABLE([werror],
    [AS_HELP_STRING([--enable-werror],
                    [Turn on compile time warnings])],
    [werror=$enableval], [werror=no])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [Turn on debug output])],
    [debug=$enableval], [debug=no])

AC_ARG_ENABLE([threads],
    [AS_HELP_STRING([--disable-threads],
                    [Turn off threading in apps])],
    [threads=$enableval], [threads=yes])

dnl Define variables
AC_ARG_VAR([LIBTOOL_LDFLAGS],
           [libtool specific flags (e.g., -static-libtool-libs)])

dnl Checks for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

PKG_PROG_PKG_CONFIG([0.20])

if test "x$GCC" = "xyes" -o "x$CC" = "xclang" ; then
  AC_DEFINE([NGHTTP2_NORETURN], [__attribute__((noreturn))], [Hint to the compiler that a function never return])
else
  AC_DEFINE([NGHTTP2_NORETURN], , [Hint to the compiler that a function never return])
fi

save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=

AX_CXX_COMPILE_STDCXX([14], [noext], [optional])

CXX1XCXXFLAGS="$CXXFLAGS"
CXXFLAGS="$save_CXXFLAGS"
AC_SUBST([CXX1XCXXFLAGS])

AC_LANG_PUSH(C++)

save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $CXX1XCXXFLAGS"

# Check that std::future is available.
AC_MSG_CHECKING([whether std::future is available])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[
#include <vector>
#include <future>
]],
[[
std::vector<std::future<int>> v;
(void)v;
]])],
    [AC_DEFINE([HAVE_STD_FUTURE], [1],
               [Define to 1 if you have the `std::future`.])
     have_std_future=yes
     AC_MSG_RESULT([yes])],
    [have_std_future=no
     AC_MSG_RESULT([no])])

# Check that std::map::emplace is available for g++-4.7.
AC_MSG_CHECKING([whether std::map::emplace is available])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[
#include <map>
]],
[[
std::map<int, int>().emplace(1, 2);
]])],
    [AC_DEFINE([HAVE_STD_MAP_EMPLACE], [1],
               [Define to 1 if you have the `std::map::emplace`.])
     have_std_map_emplace=yes
     AC_MSG_RESULT([yes])],
    [have_std_map_emplace=no
     AC_MSG_RESULT([no])])

# Check that std::atomic_* overloads for std::shared_ptr are
# available.
AC_MSG_CHECKING([whether std::atomic_* overloads for std::shared_ptr are available])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[
#include <memory>
]],
[[
auto a = std::make_shared<int>(1000000007);
auto p = std::atomic_load(&a);
++*p;
std::atomic_store(&a, p);
]])],
    [AC_DEFINE([HAVE_ATOMIC_STD_SHARED_PTR], [1],
               [Define to 1 if you have the std::atomic_* overloads for std::shared_ptr.])
     have_atomic_std_shared_ptr=yes
     AC_MSG_RESULT([yes])],
    [have_atomic_std_shared_ptr=no
     AC_MSG_RESULT([no])])

# Check that thread_local storage specifier is available
AC_MSG_CHECKING([whether thread_local storage class specifier is available.])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
,
[[
thread_local int a = 0;
(void)a;
]])],
    [AC_DEFINE([HAVE_THREAD_LOCAL], [1],
               [Define to 1 if you have thread_local storage specifier.])
     have_thread_local=yes
     AC_MSG_RESULT([yes])],
    [have_Thread_local=no
     AC_MSG_RESULT([no])])

CXXFLAGS=$save_CXXFLAGS

AC_LANG_POP()

# Checks for libraries.

# Additional libraries required for tests.
TESTLDADD=

# Additional libraries required for programs under src directory.
APPLDFLAGS=

case "$host_os" in
  *android*)
    android_build=yes
    # android does not need -pthread, but needs following 3 libs for C++
    APPLDFLAGS="$APPLDFLAGS -lstdc++ -latomic -lsupc++"
    ;;
  *)
    PTHREAD_LDFLAGS="-pthread"
    APPLDFLAGS="$APPLDFLAGS $PTHREAD_LDFLAGS"
    ;;
esac

case "$host_os" in
  *solaris*)
    APPLDFLAGS="$APPLDFLAGS -lsocket -lnsl"
    ;;
esac

case "${build}" in
  *-apple-darwin*)
    EXTRA_DEFS="-D__APPLE_USE_RFC_3542"
    AC_SUBST([EXTRA_DEFS])
    ;;
esac

# dl: openssl requires libdl when it is statically linked.
case "${host_os}" in
  *bsd*)
    # dlopen is in libc on *BSD
    ;;
  *)
    save_LIBS=$LIBS
    AC_SEARCH_LIBS([dlopen], [dl], [APPLDFLAGS="-ldl $APPLDFLAGS"], [], [])
    LIBS=$save_LIBS
    ;;
esac

# nghttp2
have_libnghttp2=no
if test "x${request_libnghttp2}" != "xno"; then
  PKG_CHECK_MODULES([LIBNGHTTP2], [libnghttp2 >= 1.43.0], [have_libnghttp2=yes],
                    [have_libnghttp2=no])
  if test "x${have_libnghttp2}" = "xno"; then
    AC_MSG_NOTICE($LIBNGHTTP2_PKG_ERRORS)
  fi
fi

if test "x${have_libnghttp2}" != "xyes"; then
  AC_MSG_ERROR([libnghttp2 is required but not found])
fi

# openssl
have_openssl=no
PKG_CHECK_MODULES([OPENSSL], [openssl >= 1.0.1],
                  [have_openssl=yes], [have_openssl=no])
if test "x${have_openssl}" = "xno"; then
  AC_MSG_NOTICE($OPENSSL_PKG_ERRORS)
fi

if test "x${have_openssl}" != "xyes"; then
  AC_MSG_ERROR([openssl is requested but not found])
fi

# Check Boost Asio library
have_asio_lib=no

AX_BOOST_BASE([1.54.0], [have_boost_base=yes], [have_boost_base=no])

if test "x${have_boost_base}" = "xyes"; then
  AX_BOOST_ASIO()
  AX_BOOST_SYSTEM()
  AX_BOOST_THREAD()

  if test "x${ax_cv_boost_asio}" = "xyes" &&
     test "x${ax_cv_boost_system}" = "xyes" &&
     test "x${ax_cv_boost_thread}" = "xyes"; then
    have_asio_lib=yes
  fi
fi

if test "x${have_asio_lib}" != "xyes"; then
  AC_MSG_ERROR([boost asio library is required but not found])
fi

# Checks for header files.
AC_HEADER_ASSERT
AC_CHECK_HEADERS([ \
  arpa/inet.h \
  fcntl.h \
  inttypes.h \
  limits.h \
  netdb.h \
  netinet/in.h \
  pwd.h \
  stddef.h \
  stdint.h \
  stdlib.h \
  string.h \
  sys/socket.h \
  sys/time.h \
  syslog.h \
  time.h \
  unistd.h \
])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_CHECK_TYPES([ptrdiff_t])
AC_C_BIGENDIAN
AC_C_INLINE
AC_SYS_LARGEFILE

AC_CHECK_MEMBER([struct tm.tm_gmtoff], [have_struct_tm_tm_gmtoff=yes],
                [have_struct_tm_tm_gmtoff=no], [[#include <time.h>]])

AC_CHECK_MEMBER([struct sockaddr_in.sin_len],
                [AC_DEFINE([HAVE_SOCKADDR_IN_SIN_LEN],[1],
                  [Define to 1 if struct sockaddr_in has sin_len member.])],
                [],
                [[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
]])

AC_CHECK_MEMBER([struct sockaddr_in6.sin6_len],
                [AC_DEFINE([HAVE_SOCKADDR_IN6_SIN6_LEN],[1],
                  [Define to 1 if struct sockaddr_in6 has sin6_len member.])],
                [],
                [[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
]])

if test "x$have_struct_tm_tm_gmtoff" = "xyes"; then
  AC_DEFINE([HAVE_STRUCT_TM_TM_GMTOFF], [1],
            [Define to 1 if you have `struct tm.tm_gmtoff` member.])
fi

# Check size of pointer to decide we need 8 bytes alignment
# adjustment.
AC_CHECK_SIZEOF([int *])

AC_CHECK_SIZEOF([time_t])

# Checks for library functions.

# Don't check malloc, since it does not play nicely with C++ stdlib
# AC_FUNC_MALLOC

AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
# Don't check realloc, since LeakSanitizer detects memory leak during check
# AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_STRNLEN

AC_CHECK_FUNCS([ \
  _Exit \
  accept4 \
  dup2 \
  getcwd \
  getpwnam \
  localtime_r \
  memchr \
  memmove \
  memset \
  mkostemp \
  socket \
  sqrt \
  strchr \
  strdup \
  strerror \
  strndup \
  strstr \
  strtol \
  strtoul \
  timegm \
])

# timerfd_create was added in linux kernel 2.6.25

AC_CHECK_FUNC([timerfd_create],
              [have_timerfd_create=yes], [have_timerfd_create=no])

# For cygwin: we can link initgroups, so AC_CHECK_FUNCS succeeds, but
# cygwin disables initgroups due to feature test macro magic with our
# configuration.  FreeBSD declares initgroups() in unistd.h.
AC_CHECK_DECLS([initgroups], [], [], [[
  #ifdef HAVE_UNISTD_H
  # include <unistd.h>
  #endif
  #include <grp.h>
]])

save_CFLAGS=$CFLAGS
save_CXXFLAGS=$CXXFLAGS

CFLAGS=
CXXFLAGS=

if test "x$werror" != "xno"; then
    # For C compiler
    AX_CHECK_COMPILE_FLAG([-Wall], [CFLAGS="$CFLAGS -Wall"])
    AX_CHECK_COMPILE_FLAG([-Wextra], [CFLAGS="$CFLAGS -Wextra"])
    AX_CHECK_COMPILE_FLAG([-Werror], [CFLAGS="$CFLAGS -Werror"])
    AX_CHECK_COMPILE_FLAG([-Wmissing-prototypes], [CFLAGS="$CFLAGS -Wmissing-prototypes"])
    AX_CHECK_COMPILE_FLAG([-Wstrict-prototypes], [CFLAGS="$CFLAGS -Wstrict-prototypes"])
    AX_CHECK_COMPILE_FLAG([-Wmissing-declarations], [CFLAGS="$CFLAGS -Wmissing-declarations"])
    AX_CHECK_COMPILE_FLAG([-Wpointer-arith], [CFLAGS="$CFLAGS -Wpointer-arith"])
    AX_CHECK_COMPILE_FLAG([-Wdeclaration-after-statement], [CFLAGS="$CFLAGS -Wdeclaration-after-statement"])
    AX_CHECK_COMPILE_FLAG([-Wformat-security], [CFLAGS="$CFLAGS -Wformat-security"])
    AX_CHECK_COMPILE_FLAG([-Wwrite-strings], [CFLAGS="$CFLAGS -Wwrite-strings"])
    AX_CHECK_COMPILE_FLAG([-Wshadow], [CFLAGS="$CFLAGS -Wshadow"])
    AX_CHECK_COMPILE_FLAG([-Winline], [CFLAGS="$CFLAGS -Winline"])
    AX_CHECK_COMPILE_FLAG([-Wnested-externs], [CFLAGS="$CFLAGS -Wnested-externs"])
    AX_CHECK_COMPILE_FLAG([-Wfloat-equal], [CFLAGS="$CFLAGS -Wfloat-equal"])
    AX_CHECK_COMPILE_FLAG([-Wundef], [CFLAGS="$CFLAGS -Wundef"])
    AX_CHECK_COMPILE_FLAG([-Wendif-labels], [CFLAGS="$CFLAGS -Wendif-labels"])
    AX_CHECK_COMPILE_FLAG([-Wempty-body], [CFLAGS="$CFLAGS -Wempty-body"])
    AX_CHECK_COMPILE_FLAG([-Wcast-align], [CFLAGS="$CFLAGS -Wcast-align"])
    AX_CHECK_COMPILE_FLAG([-Wclobbered], [CFLAGS="$CFLAGS -Wclobbered"])
    AX_CHECK_COMPILE_FLAG([-Wvla], [CFLAGS="$CFLAGS -Wvla"])
    AX_CHECK_COMPILE_FLAG([-Wpragmas], [CFLAGS="$CFLAGS -Wpragmas"])
    AX_CHECK_COMPILE_FLAG([-Wunreachable-code], [CFLAGS="$CFLAGS -Wunreachable-code"])
    AX_CHECK_COMPILE_FLAG([-Waddress], [CFLAGS="$CFLAGS -Waddress"])
    AX_CHECK_COMPILE_FLAG([-Wattributes], [CFLAGS="$CFLAGS -Wattributes"])
    AX_CHECK_COMPILE_FLAG([-Wdiv-by-zero], [CFLAGS="$CFLAGS -Wdiv-by-zero"])
    AX_CHECK_COMPILE_FLAG([-Wshorten-64-to-32], [CFLAGS="$CFLAGS -Wshorten-64-to-32"])

    AX_CHECK_COMPILE_FLAG([-Wconversion], [CFLAGS="$CFLAGS -Wconversion"])
    AX_CHECK_COMPILE_FLAG([-Wextended-offsetof], [CFLAGS="$CFLAGS -Wextended-offsetof"])
    AX_CHECK_COMPILE_FLAG([-Wformat-nonliteral], [CFLAGS="$CFLAGS -Wformat-nonliteral"])
    AX_CHECK_COMPILE_FLAG([-Wlanguage-extension-token], [CFLAGS="$CFLAGS -Wlanguage-extension-token"])
    AX_CHECK_COMPILE_FLAG([-Wmissing-field-initializers], [CFLAGS="$CFLAGS -Wmissing-field-initializers"])
    AX_CHECK_COMPILE_FLAG([-Wmissing-noreturn], [CFLAGS="$CFLAGS -Wmissing-noreturn"])
    AX_CHECK_COMPILE_FLAG([-Wmissing-variable-declarations], [CFLAGS="$CFLAGS -Wmissing-variable-declarations"])
    # Not used because we cannot change public structs
    # AX_CHECK_COMPILE_FLAG([-Wpadded], [CFLAGS="$CFLAGS -Wpadded"])
    AX_CHECK_COMPILE_FLAG([-Wsign-conversion], [CFLAGS="$CFLAGS -Wsign-conversion"])
    # Not used because this basically disallows default case
    # AX_CHECK_COMPILE_FLAG([-Wswitch-enum], [CFLAGS="$CFLAGS -Wswitch-enum"])
    AX_CHECK_COMPILE_FLAG([-Wunreachable-code-break], [CFLAGS="$CFLAGS -Wunreachable-code-break"])
    AX_CHECK_COMPILE_FLAG([-Wunused-macros], [CFLAGS="$CFLAGS -Wunused-macros"])
    AX_CHECK_COMPILE_FLAG([-Wunused-parameter], [CFLAGS="$CFLAGS -Wunused-parameter"])
    AX_CHECK_COMPILE_FLAG([-Wredundant-decls], [CFLAGS="$CFLAGS -Wredundant-decls"])
    # Only work with Clang for the moment
    AX_CHECK_COMPILE_FLAG([-Wheader-guard], [CFLAGS="$CFLAGS -Wheader-guard"])
    AX_CHECK_COMPILE_FLAG([-Wsometimes-uninitialized], [CFLAGS="$CFLAGS -Wsometimes-uninitialized"])

    # This is required because we pass format string as "const char*.
    AX_CHECK_COMPILE_FLAG([-Wno-format-nonliteral], [CFLAGS="$CFLAGS -Wno-format-nonliteral"])

    # For C++ compiler
    AC_LANG_PUSH(C++)
    AX_CHECK_COMPILE_FLAG([-Wall], [CXXFLAGS="$CXXFLAGS -Wall"])
    AX_CHECK_COMPILE_FLAG([-Werror], [CXXFLAGS="$CXXFLAGS -Werror"])
    AX_CHECK_COMPILE_FLAG([-Wformat-security], [CXXFLAGS="$CXXFLAGS -Wformat-security"])
    AX_CHECK_COMPILE_FLAG([-Wsometimes-uninitialized], [CXXFLAGS="$CXXFLAGS -Wsometimes-uninitialized"])
    # Disable noexcept-type warning of g++-7.  This is not harmful as
    # long as all source files are compiled with the same compiler.
    AX_CHECK_COMPILE_FLAG([-Wno-noexcept-type], [CXXFLAGS="$CXXFLAGS -Wno-noexcept-type"])
    AC_LANG_POP()
fi

WARNCFLAGS=$CFLAGS
WARNCXXFLAGS=$CXXFLAGS

CFLAGS=$save_CFLAGS
CXXFLAGS=$save_CXXFLAGS

AC_SUBST([WARNCFLAGS])
AC_SUBST([WARNCXXFLAGS])

EXTRACFLAG=
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [EXTRACFLAG="-fvisibility=hidden"])

AC_SUBST([EXTRACFLAG])

if test "x$debug" != "xno"; then
    AC_DEFINE([DEBUGBUILD], [1], [Define to 1 to enable debug output.])
fi

enable_threads=yes
# Some platform does not have working std::future.  We disable
# threading for those platforms.
if test "x$threads" != "xyes" ||
   test "x$have_std_future" != "xyes"; then
    enable_threads=no
    AC_DEFINE([NOTHREADS], [1], [Define to 1 if you want to disable threads.])
fi

# propagate $enable_static to tests/Makefile.am
AM_CONDITIONAL([ENABLE_STATIC], [test "x$enable_static" = "xyes"])

AC_SUBST([TESTLDADD])
AC_SUBST([APPLDFLAGS])

AC_CONFIG_FILES([
  Makefile
  lib/Makefile
  lib/libnghttp2_asio.pc
  lib/includes/Makefile
  third-party/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([summary of build options:

    Package version: ${VERSION}
    Library version: $LT_CURRENT:$LT_REVISION:$LT_AGE
    Install prefix:  ${prefix}
    System types:
      Build:         ${build}
      Host:          ${host}
      Target:        ${target}
    Compiler:
      C compiler:     ${CC}
      CFLAGS:         ${CFLAGS}
      LDFLAGS:        ${LDFLAGS}
      C++ compiler:   ${CXX}
      CXXFLAGS:       ${CXXFLAGS}
      CXXCPP:         ${CXXCPP}
      C preprocessor: ${CPP}
      CPPFLAGS:       ${CPPFLAGS}
      WARNCFLAGS:     ${WARNCFLAGS}
      WARNCXXFLAGS:   ${WARNCXXFLAGS}
      CXX1XCXXFLAGS:  ${CXX1XCXXFLAGS}
      EXTRACFLAG:     ${EXTRACFLAG}
      LIBS:           ${LIBS}
      DEFS:           ${DEFS}
      EXTRA_DEFS:     ${EXTRA_DEFS}
    Library:
      Shared:         ${enable_shared}
      Static:         ${enable_static}
    Libtool:
      LIBTOOL_LDFLAGS: ${LIBTOOL_LDFLAGS}
    Libs:
      OpenSSL:        ${have_openssl} (CFLAGS='${OPENSSL_CFLAGS}' LIBS='${OPENSSL_LIBS}')
      Boost CPPFLAGS: ${BOOST_CPPFLAGS}
      Boost LDFLAGS:  ${BOOST_LDFLAGS}
      Boost::ASIO:    ${BOOST_ASIO_LIB}
      Boost::System:  ${BOOST_SYSTEM_LIB}
      Boost::Thread:  ${BOOST_THREAD_LIB}
    Features:
      Threading:      ${enable_threads}
])
