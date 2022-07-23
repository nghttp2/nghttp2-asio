# nghttp2 - HTTP/2 C Library
#
# Copyright (c) 2012, 2013, 2014, 2015 Tatsuhiro Tsujikawa
# Copyright (c) 2016 Peter Wu <peter@lekensteyn.nl>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.0)
# XXX using 1.8.90 instead of 1.9.0-DEV
project(nghttp2-asio VERSION 0.0.90)

# See versioning rule:
#  https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
set(LT_CURRENT  0)
set(LT_REVISION 0)
set(LT_AGE      0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(Version)

math(EXPR LT_SOVERSION "${LT_CURRENT} - ${LT_AGE}")
set(LT_VERSION "${LT_SOVERSION}.${LT_AGE}.${LT_REVISION}")
set(PACKAGE_VERSION     "${PROJECT_VERSION}")
HexVersion(PACKAGE_VERSION_NUM ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the build type" FORCE)

  # Include "None" as option to disable any additional (optimization) flags,
  # relying on just CMAKE_C_FLAGS and CMAKE_CXX_FLAGS (which are empty by
  # default). These strings are presented in cmake-gui.
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "None" "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(GNUInstallDirs)

# Auto-detection of features that can be toggled
find_package(OpenSSL 1.0.1)
find_package(Libnghttp2 1.43.0)

include(CMakeOptions.txt)

# Do not disable assertions based on CMAKE_BUILD_TYPE.
foreach(_build_type "Release" "MinSizeRel" "RelWithDebInfo")
  foreach(_lang C CXX)
    string(TOUPPER "CMAKE_${_lang}_FLAGS_${_build_type}" _var)
    string(REGEX REPLACE "(^| )[/-]D *NDEBUG($| )" " " ${_var} "${${_var}}")
  endforeach()
endforeach()

if(CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(HINT_NORETURN       "__attribute__((noreturn))")
else()
  set(HINT_NORETURN)
endif()

include(ExtractValidFlags)
foreach(_cxx1x_flag -std=c++14)
  extract_valid_cxx_flags(_cxx1x_flag_supported ${_cxx1x_flag})
  if(_cxx1x_flag_supported)
    set(CXX1XCXXFLAGS ${_cxx1x_flag})
    break()
  endif()
endforeach()

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)
cmake_push_check_state()
set(CMAKE_REQUIRED_DEFINITIONS "${CXX1XCXXFLAGS}")
# Check that std::future is available.
check_cxx_source_compiles("
#include <vector>
#include <future>
int main() { std::vector<std::future<int>> v; }" HAVE_STD_FUTURE)
# Check that std::map::emplace is available for g++-4.7.
check_cxx_source_compiles("
#include <map>
int main() { std::map<int, int>().emplace(1, 2); }" HAVE_STD_MAP_EMPLACE)
cmake_pop_check_state()


# Checks for libraries.
# Additional libraries required for programs under src directory.
set(APP_LIBRARIES)

set(CMAKE_THREAD_PREFER_PTHREAD 1)
find_package(Threads)
if(CMAKE_USE_PTHREADS_INIT)
  list(APPEND APP_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
endif()
# XXX android and C++, is this still needed in cmake?
# case "$host" in
#   *android*)
#     android_build=yes
#     # android does not need -pthread, but needs following 3 libs for C++
#     APPLDFLAGS="$APPLDFLAGS -lstdc++ -latomic -lsupc++"

# dl: openssl requires libdl when it is statically linked.
# XXX shouldn't ${CMAKE_DL_LIBS} be appended to OPENSSL_LIBRARIES instead of
# APP_LIBRARIES if it is really specific to OpenSSL?

# openssl
include(CheckSymbolExists)
set(HAVE_OPENSSL    ${OPENSSL_FOUND})
if(OPENSSL_FOUND)
  set(OPENSSL_INCLUDE_DIRS  ${OPENSSL_INCLUDE_DIR})
  cmake_push_check_state()
  set(CMAKE_REQUIRED_INCLUDES   "${OPENSSL_INCLUDE_DIR}")
  set(CMAKE_REQUIRED_LIBRARIES  "${OPENSSL_LIBRARIES}")
  if(WIN32)
    set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}" "ws2_32" "bcrypt")
  endif()
  cmake_pop_check_state()
else()
  set(OPENSSL_INCLUDE_DIRS  "")
  set(OPENSSL_LIBRARIES     "")
endif()

find_package(Boost 1.54.0 REQUIRED system thread)

# Checks for header files.
include(CheckIncludeFile)
check_include_file("arpa/inet.h"    HAVE_ARPA_INET_H)
check_include_file("fcntl.h"        HAVE_FCNTL_H)
check_include_file("inttypes.h"     HAVE_INTTYPES_H)
check_include_file("limits.h"       HAVE_LIMITS_H)
check_include_file("netdb.h"        HAVE_NETDB_H)
check_include_file("netinet/in.h"   HAVE_NETINET_IN_H)
check_include_file("pwd.h"          HAVE_PWD_H)
check_include_file("sys/socket.h"   HAVE_SYS_SOCKET_H)
check_include_file("sys/time.h"     HAVE_SYS_TIME_H)
check_include_file("syslog.h"       HAVE_SYSLOG_H)
check_include_file("time.h"         HAVE_TIME_H)
check_include_file("unistd.h"       HAVE_UNISTD_H)

include(CheckTypeSize)
# Checks for typedefs, structures, and compiler characteristics.
# AC_TYPE_SIZE_T
check_type_size("ssize_t" SIZEOF_SSIZE_T)
if(SIZEOF_SSIZE_T STREQUAL "")
  # ssize_t is a signed type in POSIX storing at least -1.
  # Set it to "int" to match the behavior of AC_TYPE_SSIZE_T (autotools).
  set(ssize_t int)
endif()
# AC_TYPE_UINT8_T
# AC_TYPE_UINT16_T
# AC_TYPE_UINT32_T
# AC_TYPE_UINT64_T
# AC_TYPE_INT8_T
# AC_TYPE_INT16_T
# AC_TYPE_INT32_T
# AC_TYPE_INT64_T
# AC_TYPE_OFF_T
# AC_TYPE_PID_T
# AC_TYPE_UID_T
# XXX To support inline for crappy compilers, see https://cmake.org/Wiki/CMakeTestInline
# AC_C_INLINE
# XXX is AC_SYS_LARGEFILE still needed for modern systems?
# add_definitions(-D_FILE_OFFSET_BITS=64)

include(CheckStructHasMember)
check_struct_has_member("struct tm" tm_gmtoff time.h HAVE_STRUCT_TM_TM_GMTOFF)

# Check size of pointer to decide we need 8 bytes alignment adjustment.
check_type_size("int *"   SIZEOF_INT_P)
check_type_size("time_t"  SIZEOF_TIME_T)

# Checks for library functions.
include(CheckFunctionExists)
check_function_exists(_Exit     HAVE__EXIT)
check_function_exists(accept4   HAVE_ACCEPT4)
check_function_exists(mkostemp  HAVE_MKOSTEMP)

include(CheckSymbolExists)
# XXX does this correctly detect initgroups (un)availability on cygwin?
check_symbol_exists(initgroups grp.h HAVE_DECL_INITGROUPS)
if(NOT HAVE_DECL_INITGROUPS AND HAVE_UNISTD_H)
  # FreeBSD declares initgroups() in unistd.h
  check_symbol_exists(initgroups unistd.h HAVE_DECL_INITGROUPS2)
  if(HAVE_DECL_INITGROUPS2)
    set(HAVE_DECL_INITGROUPS 1)
  endif()
endif()

set(WARNCFLAGS)
set(WARNCXXFLAGS)
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  if(ENABLE_WERROR)
    set(WARNCFLAGS    /WX)
    set(WARNCXXFLAGS  /WX)
  endif()
else()
  if(ENABLE_WERROR)
    extract_valid_c_flags(WARNCFLAGS    -Werror)
    extract_valid_c_flags(WARNCXXFLAGS  -Werror)
  endif()

  # For C compiler
  extract_valid_c_flags(WARNCFLAGS
    -Wall
    -Wextra
    -Wmissing-prototypes
    -Wstrict-prototypes
    -Wmissing-declarations
    -Wpointer-arith
    -Wdeclaration-after-statement
    -Wformat-security
    -Wwrite-strings
    -Wshadow
    -Winline
    -Wnested-externs
    -Wfloat-equal
    -Wundef
    -Wendif-labels
    -Wempty-body
    -Wcast-align
    -Wclobbered
    -Wvla
    -Wpragmas
    -Wunreachable-code
    -Waddress
    -Wattributes
    -Wdiv-by-zero
    -Wshorten-64-to-32

    -Wconversion
    -Wextended-offsetof
    -Wformat-nonliteral
    -Wlanguage-extension-token
    -Wmissing-field-initializers
    -Wmissing-noreturn
    -Wmissing-variable-declarations
    # Not used because we cannot change public structs
    # -Wpadded
    -Wsign-conversion
    # Not used because this basically disallows default case
    # -Wswitch-enum
    -Wunreachable-code-break
    -Wunused-macros
    -Wunused-parameter
    -Wredundant-decls
    # Only work with Clang for the moment
    -Wheader-guard
    # This is required because we pass format string as "const char*.
    -Wno-format-nonliteral
  )

  extract_valid_cxx_flags(WARNCXXFLAGS
    # For C++ compiler
    -Wall
    -Wformat-security
  )
endif()

if(ENABLE_STATIC_CRT)
  foreach(lang C CXX)
    foreach(suffix "" _DEBUG _MINSIZEREL _RELEASE _RELWITHDEBINFO)
      set(var "CMAKE_${lang}_FLAGS${suffix}")
      string(REPLACE "/MD" "/MT" ${var} "${${var}}")
    endforeach()
  endforeach()
endif()

if(ENABLE_DEBUG)
  set(DEBUGBUILD 1)
endif()

# Some platform does not have working std::future.  We disable
# threading for those platforms.
if(NOT ENABLE_THREADS OR NOT HAVE_STD_FUTURE)
  set(NOTHREADS 1)
endif()

add_definitions(-DHAVE_CONFIG_H)
configure_file(cmakeconfig.h.in config.h)
# autotools-compatible names
# Sphinx expects relative paths in the .rst files. Use the fact that the files
# below are all one directory level deep.
file(RELATIVE_PATH top_srcdir   "${CMAKE_CURRENT_BINARY_DIR}/dir" "${CMAKE_CURRENT_SOURCE_DIR}")
file(RELATIVE_PATH top_builddir "${CMAKE_CURRENT_BINARY_DIR}/dir" "${CMAKE_CURRENT_BINARY_DIR}")
set(abs_top_srcdir  "${CMAKE_CURRENT_SOURCE_DIR}")
set(abs_top_builddir "${CMAKE_CURRENT_BINARY_DIR}")
# libnghttp2.pc (pkg-config file)
set(prefix          "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix     "${CMAKE_INSTALL_PREFIX}")
set(libdir          "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir      "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(VERSION         "${PACKAGE_VERSION}")
# For init scripts and systemd service file (in contrib/)
set(bindir          "${CMAKE_INSTALL_FULL_BINDIR}")
set(sbindir         "${CMAKE_INSTALL_FULL_SBINDIR}")
foreach(name
   lib/libnghttp2_asio.pc
)
  configure_file("${name}.in" "${name}" @ONLY)
endforeach()

if(APPLE)
  add_definitions(-D__APPLE_USE_RFC_3542)
endif()

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}" # for config.h
)
# For use in src/CMakeLists.txt
set(PKGDATADIR "${CMAKE_INSTALL_FULL_DATADIR}/${CMAKE_PROJECT_NAME}")
set(PKGLIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}/${CMAKE_PROJECT_NAME}")

install(FILES README.rst DESTINATION "${CMAKE_INSTALL_DOCDIR}")

add_subdirectory(lib)
#add_subdirectory(lib/includes)
add_subdirectory(third-party)
#add_subdirectory(src/includes)
#add_subdirectory(tests/testdata)

string(TOUPPER "${CMAKE_BUILD_TYPE}" _build_type)
message(STATUS "summary of build options:

    Package version: ${VERSION}
    Library version: ${LT_CURRENT}:${LT_REVISION}:${LT_AGE}
    Install prefix:  ${CMAKE_INSTALL_PREFIX}
    Target system:   ${CMAKE_SYSTEM_NAME}
    Compiler:
      Build type:     ${CMAKE_BUILD_TYPE}
      C compiler:     ${CMAKE_C_COMPILER}
      CFLAGS:         ${CMAKE_C_FLAGS_${_build_type}} ${CMAKE_C_FLAGS}
      C++ compiler:   ${CMAKE_CXX_COMPILER}
      CXXFLAGS:       ${CMAKE_CXX_FLAGS_${_build_type}} ${CMAKE_CXX_FLAGS}
      WARNCFLAGS:     ${WARNCFLAGS}
      CXX1XCXXFLAGS:  ${CXX1XCXXFLAGS}
    Libs:
      OpenSSL:        ${HAVE_OPENSSL} (LIBS='${OPENSSL_LIBRARIES}')
      Libnghttp2:     ${HAVE_LIBNGHTTP2} (LIBS='${LIBNGHTTP2_LIBRARIES}')
      Boost::System:  ${Boost_SYSTEM_LIBRARY}
      Boost::Thread:  ${Boost_THREAD_LIBRARY}
")
