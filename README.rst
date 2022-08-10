libnghttp2_asio: High level HTTP/2 C++ library
----------------------------------------------

libnghttp2_asio is C++ library built on top of libnghttp2 and provides
high level abstraction API to build HTTP/2 applications.  It depends
on the Boost::ASIO library and OpenSSL.  libnghttp2_asio
provides both client and server APIs.

The server API is designed to build an HTTP/2 server very easily to utilize
modern C++ anonymous functions and closures.  The bare minimum example of
an HTTP/2 server looks like this:

.. code-block:: cpp

    #include <iostream>

    #include <nghttp2/asio_http2_server.h>

    using namespace nghttp2::asio_http2;
    using namespace nghttp2::asio_http2::server;

    int main(int argc, char *argv[]) {
      boost::system::error_code ec;
      http2 server;

      server.handle("/", [](const request &req, const response &res) {
        res.write_head(200);
        res.end("hello, world\n");
      });

      if (server.listen_and_serve(ec, "localhost", "3000")) {
        std::cerr << "error: " << ec.message() << std::endl;
      }
    }

Here is sample code to use the client API:

.. code-block:: cpp

    #include <iostream>

    #include <nghttp2/asio_http2_client.h>

    using boost::asio::ip::tcp;

    using namespace nghttp2::asio_http2;
    using namespace nghttp2::asio_http2::client;

    int main(int argc, char *argv[]) {
      boost::system::error_code ec;
      boost::asio::io_service io_service;

      // connect to localhost:3000
      session sess(io_service, "localhost", "3000");

      sess.on_connect([&sess](tcp::resolver::iterator endpoint_it) {
        boost::system::error_code ec;

        auto req = sess.submit(ec, "GET", "http://localhost:3000/");

        req->on_response([](const response &res) {
          // print status code and response header fields.
          std::cerr << "HTTP/2 " << res.status_code() << std::endl;
          for (auto &kv : res.header()) {
            std::cerr << kv.first << ": " << kv.second.value << "\n";
          }
          std::cerr << std::endl;

          res.on_data([](const uint8_t *data, std::size_t len) {
            std::cerr.write(reinterpret_cast<const char *>(data), len);
            std::cerr << std::endl;
          });
        });

        req->on_close([&sess](uint32_t error_code) {
          // shutdown session after first request was done.
          sess.shutdown();
        });
      });

      sess.on_error([](const boost::system::error_code &ec) {
        std::cerr << "error: " << ec.message() << std::endl;
      });

      io_service.run();
    }