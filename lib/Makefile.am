# nghttp2 - HTTP/2 C Library

# Copyright (c) 2012 Tatsuhiro Tsujikawa

# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:

# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
SUBDIRS = includes

EXTRA_DIST = \
	CMakeLists.txt

AM_CFLAGS = $(WARNCFLAGS)
AM_CXXFLAGS = $(WARNCXXFLAGS) $(CXX1XCXXFLAGS)
AM_CPPFLAGS = \
	-DPKGDATADIR='"$(pkgdatadir)"' \
	-DPKGLIBDIR='"$(pkglibdir)"' \
	-I$(top_srcdir)/lib/includes \
	-I$(top_builddir)/lib/includes \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/third-party \
	@LIBNGHTTP2_CFLAGS@ \
	@OPENSSL_CFLAGS@ \
	@EXTRA_DEFS@ \
	@DEFS@
AM_LDFLAGS = @LIBTOOL_LDFLAGS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnghttp2_asio.pc
DISTCLEANFILES = $(pkgconfig_DATA)

lib_LTLIBRARIES = libnghttp2_asio.la

libnghttp2_asio_la_SOURCES = \
	nghttp2_config.h \
	template.h \
	network.h \
	allocator.h \
	util.cc util.h http2.cc http2.h \
	tls.h \
	timegm.c timegm.h \
	asio_common.cc asio_common.h \
	asio_io_service_pool.cc asio_io_service_pool.h \
	asio_server_http2.cc \
	asio_server_http2_impl.cc asio_server_http2_impl.h \
	asio_server.cc asio_server.h \
	asio_server_http2_handler.cc asio_server_http2_handler.h \
	asio_server_connection.h \
	asio_server_request.cc \
	asio_server_request_impl.cc asio_server_request_impl.h \
	asio_server_response.cc \
	asio_server_response_impl.cc asio_server_response_impl.h \
	asio_server_stream.cc asio_server_stream.h \
	asio_server_serve_mux.cc asio_server_serve_mux.h \
	asio_server_request_handler.cc asio_server_request_handler.h \
	asio_server_tls_context.cc asio_server_tls_context.h \
	asio_client_session.cc \
	asio_client_session_impl.cc asio_client_session_impl.h \
	asio_client_session_tcp_impl.cc asio_client_session_tcp_impl.h \
	asio_client_session_tls_impl.cc asio_client_session_tls_impl.h \
	asio_client_response.cc \
	asio_client_response_impl.cc asio_client_response_impl.h \
	asio_client_request.cc \
	asio_client_request_impl.cc asio_client_request_impl.h \
	asio_client_stream.cc asio_client_stream.h \
	asio_client_tls_context.cc asio_client_tls_context.h

libnghttp2_asio_la_CPPFLAGS = ${AM_CPPFLAGS} ${BOOST_CPPFLAGS}
libnghttp2_asio_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libnghttp2_asio_la_LIBADD = \
	$(top_builddir)/third-party/liburl-parser.la \
	@LIBNGHTTP2_LIBS@ \
	@OPENSSL_LIBS@ \
	${BOOST_LDFLAGS} \
	${BOOST_ASIO_LIB} \
	${BOOST_THREAD_LIB} \
	${BOOST_SYSTEM_LIB}
