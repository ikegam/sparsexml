#ifndef __SXMLParserPRIV__
#define __SXMLParserPRIV__

#include "sparsexml.h"

enum SXMLParserState {
  INITIAL,
  IN_HEADER,
  IN_TAG,
  IN_ATTRIBUTE_KEY,
  IN_ATTRIBUTE_VALUE,
  IN_CONTENT
};

struct __SXMLParser {
  enum SXMLParserState state;

  char buffer[SXMLElementLength];
  unsigned int bp;
  unsigned char header_parsed;

  unsigned char (*tag_func)(char *);
  unsigned char (*content_func)(char *);
  unsigned char (*attribute_value_func)(char *);
  unsigned char (*attribute_key_func)(char *);
};

unsigned char priv_sxml_change_parser_state(SXMLParser* parser, enum SXMLParserState state);

#endif
