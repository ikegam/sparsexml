#ifndef __SXMLExplorerPRIV__
#define __SXMLExplorerPRIV__

#include "sparsexml.h"

struct __SXMLExplorer {
  SXMLExplorerState state;

  char buffer[SXMLElementLength];
  unsigned int bp;
  unsigned char header_parsed;
  unsigned int comment_depth;  // For nested comment detection
  unsigned int cdata_pos;      // Position counter for CDATA detection

  unsigned char (*tag_func)(char *);
  unsigned char (*content_func)(char *);
  unsigned char (*attribute_value_func)(char *);
  unsigned char (*attribute_key_func)(char *);
  unsigned char (*comment_func)(char *);
};

unsigned char priv_sxml_change_explorer_state(SXMLExplorer* explorer, SXMLExplorerState state);

#endif
