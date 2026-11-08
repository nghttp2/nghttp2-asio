find_package(Threads REQUIRED)

add_executable(rpcpio_features_test rpcpio_features_test.cc)
target_link_libraries(rpcpio_features_test
  nghttp2_asio
  Threads::Threads
)
target_include_directories(rpcpio_features_test PRIVATE
  "${CMAKE_SOURCE_DIR}/lib/includes"
  "${CMAKE_BINARY_DIR}/lib/includes"
)

include(CTest)
add_test(NAME rpcpio_features_test COMMAND rpcpio_features_test)
