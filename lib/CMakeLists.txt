add_subdirectory(includes)

file(GLOB c_sources *.c)
set_source_files_properties(${c_sources} PROPERTIES
  COMPILE_FLAGS "${WARNCFLAGS}")
file(GLOB cxx_sources *.cc)
set_source_files_properties(${cxx_sources} PROPERTIES
  COMPILE_FLAGS "${WARNCXXFLAGS} ${CXX1XCXXFLAGS}")

set(NGHTTP2_ASIO_SOURCES
  util.cc http2.cc
  timegm.c
  asio_common.cc
  asio_io_service_pool.cc
  asio_server_http2.cc
  asio_server_http2_impl.cc
  asio_server.cc
  asio_server_http2_handler.cc
  asio_server_request.cc
  asio_server_request_impl.cc
  asio_server_response.cc
  asio_server_response_impl.cc
  asio_server_stream.cc
  asio_server_serve_mux.cc
  asio_server_request_handler.cc
  asio_server_tls_context.cc
  asio_client_session.cc
  asio_client_session_impl.cc
  asio_client_session_tcp_impl.cc
  asio_client_session_tls_impl.cc
  asio_client_response.cc
  asio_client_response_impl.cc
  asio_client_request.cc
  asio_client_request_impl.cc
  asio_client_stream.cc
  asio_client_tls_context.cc
)

if (NOT ENABLE_SHARED_LIB AND NOT ENABLE_STATIC_LIB)
  if (BUILD_SHARED_LIBS)
    set(ENABLE_SHARED_LIB ON)
  else()
    set(ENABLE_STATIC_LIB ON)
  endif()
endif()

# Object lib used for both static and shared lib
add_library(nghttp2_asio_object OBJECT ${NGHTTP2_ASIO_SOURCES})
target_include_directories(nghttp2_asio_object
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/includes"
    "${CMAKE_CURRENT_SOURCE_DIR}/../third-party"
    ${LIBNGHTTP2_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
  INTERFACE
    "${CMAKE_CURRENT_BINARY_DIR}/../lib/includes"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/includes"
    "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)
target_link_libraries(nghttp2_asio_object
  PRIVATE
    ${LIBNGHTTP2_LIBRARIES}
    ${OPENSSL_LIBRARIES}
  PUBLIC
    ${Boost_LIBRARIES}
)
set_target_properties(nghttp2_asio_object
  PROPERTIES
    VERSION ${LT_VERSION}
    SOVERSION ${LT_SOVERSION}
)

# Public shared library
if(ENABLE_SHARED_LIB)
  set_property(TARGET nghttp2_asio_object PROPERTY POSITION_INDEPENDENT_CODE 1)
  add_library(nghttp2_asio SHARED)
  target_link_libraries(nghttp2_asio PUBLIC nghttp2_asio_object $<TARGET_OBJECTS:url-parser>)
  install(TARGETS nghttp2_asio)
endif()

# Static library
if(ENABLE_STATIC_LIB)
  add_library(nghttp2_asio_static STATIC)
  target_link_libraries(nghttp2_asio_static PUBLIC nghttp2_asio_object $<TARGET_OBJECTS:url-parser>)
  set_target_properties(nghttp2_asio_static PROPERTIES ARCHIVE_OUTPUT_NAME nghttp2_asio${STATIC_LIB_SUFFIX})
  install(TARGETS nghttp2_asio_static)
endif()

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libnghttp2_asio.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
