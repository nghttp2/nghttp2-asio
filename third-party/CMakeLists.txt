set(LIBURL_PARSER_SOURCES
  url-parser/url_parser.c
)
add_library(url-parser OBJECT ${LIBURL_PARSER_SOURCES})
set_target_properties(url-parser PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
