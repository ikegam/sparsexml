#ifndef __SXMLExplorerPRIV__
#define __SXMLExplorerPRIV__

#include "sparsexml.h"

#define SXML_KIND_XML 1
#define SXML_KIND_EXI 2

// Part shared by every kind of explorer. Each explorer type starts with it,
// so a pointer to any explorer can be cast to SXMLExplorer* and back.
struct __SXMLExplorer {
  unsigned char kind;          // SXML_KIND_*
  unsigned char heap_allocated; // Created by sxml_make_*explorer()

  char buffer[SXMLElementLength]; // Text handed to the callbacks (always this buffer)
  unsigned int bp;
  unsigned char enable_entity_processing; // Flag to enable/disable entity processing
  unsigned char enable_namespace_processing; // Flag to enable/disable namespace processing
  unsigned char enable_extended_entities; // Flag to enable/disable extended HTML entities
  unsigned char enable_numeric_entities; // Flag to enable/disable numeric character references

  SXMLCallback tag_func;
  SXMLCallback content_func;
  SXMLCallback attribute_value_func;
  SXMLCallback attribute_key_func;
  SXMLCallback comment_func;
  void* user_data;
};

// XML explorer: the state machine of sxml_run_explorer()
typedef struct {
  SXMLExplorer base;

  SXMLExplorerState state;
  SXMLExplorerState prev_state; // Previous state for entity processing
  char entity_buffer[32];      // Temporary buffer for entity processing
  unsigned int entity_bp;      // Buffer pointer for entity processing
  char tag_name[SXMLNameLength]; // Last start tag name (for <empty/> elements)
  char last[2];                // Last two characters seen in the current state
  char quote;                  // Quote character of the current attribute value
  unsigned char self_closing;  // '/' seen inside a start tag
  unsigned int doctype_depth;  // Nesting of [ ] in DOCTYPE internal subset
} SXMLXmlExplorer;

// The EXI explorer (SXMLExiExplorer) is defined in sparsexml.c

unsigned char priv_sxml_change_explorer_state(SXMLXmlExplorer* explorer, SXMLExplorerState state);
unsigned char priv_sxml_process_entity(SXMLExplorer* explorer, char* entity_buffer);
unsigned char priv_sxml_process_numeric_entity(SXMLExplorer* explorer, char* entity_buffer);
unsigned char priv_sxml_process_extended_entity(SXMLExplorer* explorer, char* entity_buffer);
void priv_sxml_process_namespace(char* tag_name, char** namespace_uri, char** local_name);
unsigned char priv_append_char(SXMLExplorer* explorer, char c);
unsigned char priv_append_string(SXMLExplorer* explorer, const char* str);

#endif
