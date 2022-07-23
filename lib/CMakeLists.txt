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

add_library(nghttp2_asio SHARED
  ${NGHTTP2_ASIO_SOURCES}
  $<TARGET_OBJECTS:url-parser>
)
target_include_directories(nghttp2_asio PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/includes"
  "${CMAKE_CURRENT_SOURCE_DIR}/../third-party"

  ${LIBNGHTTP2_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)
target_include_directories(nghttp2_asio INTERFACE
  "${CMAKE_CURRENT_BINARY_DIR}/../lib/includes"
  "${CMAKE_CURRENT_SOURCE_DIR}/../lib/includes"
  "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)
target_link_libraries(nghttp2_asio
  ${LIBNGHTTP2_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${Boost_LIBRARIES}
)
set_target_properties(nghttp2_asio PROPERTIES
  VERSION ${LT_VERSION} SOVERSION ${LT_SOVERSION})

install(TARGETS nghttp2_asio
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libnghttp2_asio.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
