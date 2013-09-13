#ifndef __SXMLExplorerPRIV__
#define __SXMLExplorerPRIV__

#include "sparsexml.h"

enum SXMLExplorerState {
  INITIAL,
  IN_HEADER,
  IN_TAG,
  IN_ATTRIBUTE_KEY,
  IN_ATTRIBUTE_VALUE,
  IN_CONTENT
};

struct __SXMLExplorer {
  enum SXMLExplorerState state;

  char buffer[SXMLElementLength];
  unsigned int bp;
  unsigned char header_parsed;

  unsigned char (*tag_func)(char *);
  unsigned char (*content_func)(char *);
  unsigned char (*attribute_value_func)(char *);
  unsigned char (*attribute_key_func)(char *);
};

unsigned char priv_sxml_change_explorer_state(SXMLExplorer* explorer, enum SXMLExplorerState state);

#endif
