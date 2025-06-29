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
  SXMLExplorerState prev_state; // Previous state for entity processing
  char entity_buffer[32];      // Temporary buffer for entity processing
  unsigned int entity_bp;      // Buffer pointer for entity processing
  unsigned int saved_bp;       // Saved buffer position before entity processing
  unsigned char enable_entity_processing; // Flag to enable/disable entity processing
  unsigned char enable_namespace_processing; // Flag to enable/disable namespace processing
  unsigned char enable_extended_entities; // Flag to enable/disable extended HTML entities
  unsigned char enable_numeric_entities; // Flag to enable/disable numeric character references

  unsigned char (*tag_func)(char *);
  unsigned char (*content_func)(char *);
  unsigned char (*attribute_value_func)(char *);
  unsigned char (*attribute_key_func)(char *);
  unsigned char (*comment_func)(char *);
};

unsigned char priv_sxml_change_explorer_state(SXMLExplorer* explorer, SXMLExplorerState state);
unsigned char priv_sxml_process_entity(SXMLExplorer* explorer, char* entity_buffer);
unsigned char priv_sxml_process_numeric_entity(SXMLExplorer* explorer, char* entity_buffer);
unsigned char priv_sxml_process_extended_entity(SXMLExplorer* explorer, char* entity_buffer);
void priv_sxml_process_namespace(char* tag_name, char** namespace_uri, char** local_name);

#endif
