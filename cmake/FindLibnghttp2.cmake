# - Try to find libnghttp2
# Once done this will define
#  LIBNGHTTP2_FOUND        - System has libnghttp2
#  LIBNGHTTP2_INCLUDE_DIRS - The libnghttp2 include directories
#  LIBNGHTTP2_LIBRARIES    - The libraries needed to use libnghttp2

find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBNGHTTP2 QUIET libnghttp2)

find_path(LIBNGHTTP2_INCLUDE_DIR
  NAMES nghttp2/nghttp2.h
  HINTS ${PC_LIBNGHTTP2_INCLUDE_DIRS}
)
find_library(LIBNGHTTP2_LIBRARY
  NAMES nghttp2
  HINTS ${PC_LIBNGHTTP2_LIBRARY_DIRS}
)

if(LIBNGHTTP2_INCLUDE_DIR)
  set(_version_regex "^#define[ \t]+NGHTTP2_VERSION[ \t]+\"([^\"]+)\".*")
  file(STRINGS "${LIBNGHTTP2_INCLUDE_DIR}/nghttp2/nghttp2ver.h"
    LIBNGHTTP2_VERSION REGEX "${_version_regex}")
  string(REGEX REPLACE "${_version_regex}" "\\1"
    LIBNGHTTP2_VERSION "${LIBNGHTTP2_VERSION}")
  unset(_version_regex)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBNGHTTP2_FOUND
# to TRUE if all listed variables are TRUE and the requested version
# matches.
find_package_handle_standard_args(Libnghttp2 REQUIRED_VARS
                                  LIBNGHTTP2_LIBRARY LIBNGHTTP2_INCLUDE_DIR
                                  VERSION_VAR LIBNGHTTP2_VERSION)

if(LIBNGHTTP2_FOUND)
  set(LIBNGHTTP2_LIBRARIES     ${LIBNGHTTP2_LIBRARY})
  set(LIBNGHTTP2_INCLUDE_DIRS  ${LIBNGHTTP2_INCLUDE_DIR})
endif()

mark_as_advanced(LIBNGHTTP2_INCLUDE_DIR LIBNGHTTP2_LIBRARY)
