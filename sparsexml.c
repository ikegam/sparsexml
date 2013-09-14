#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "sparsexml-priv.h"

unsigned char priv_sxml_change_explorer_state(SXMLExplorer* explorer, enum SXMLExplorerState state) {
  unsigned char ret = SXMLExplorerContinue;

  if (strlen(explorer->buffer) > 0) {
           if (explorer->state == IN_TAG && state == IN_CONTENT && explorer->tag_func != NULL) {
      ret = explorer->tag_func(explorer->buffer);
    } else if (explorer->state == IN_TAG && state == IN_TAG && explorer->tag_func != NULL) {
      ret = explorer->tag_func(explorer->buffer);
    } else if (explorer->state == IN_TAG && state == IN_ATTRIBUTE_KEY && explorer->tag_func != NULL) {
      ret = explorer->tag_func(explorer->buffer);
    } else if (explorer->state == IN_CONTENT && state == IN_TAG && explorer->content_func != NULL) {
      ret = explorer->content_func(explorer->buffer);
    } else if (explorer->state == IN_ATTRIBUTE_KEY && state == IN_ATTRIBUTE_VALUE && explorer->attribute_key_func != NULL) {
      ret = explorer->attribute_key_func(explorer->buffer);
    } else if (explorer->state == IN_ATTRIBUTE_VALUE && state == IN_TAG && explorer->attribute_value_func != NULL) {
      ret = explorer->attribute_value_func(explorer->buffer);
    }
  }

  explorer->bp = 0;
  explorer->buffer[0] = '\0';

  explorer->state = state;

  return ret;
}

SXMLExplorer* sxml_make_explorer(void) {
  SXMLExplorer* explorer;
  explorer = malloc(sizeof(SXMLExplorer));

  explorer->state = INITIAL;
  explorer->bp = 0;
  explorer->buffer[0] = '\0';
  explorer->header_parsed = 0;

  return explorer;
}

void sxml_destroy_explorer(SXMLExplorer *explorer) {
  free(explorer);
}

void sxml_register_func(SXMLExplorer* explorer, void* open, void* content, void* attribute_key, void* attribute_value) {
  explorer->tag_func = open;
  explorer->content_func = content;
  explorer->attribute_value_func = attribute_value;
  explorer->attribute_key_func = attribute_key;
}

unsigned char sxml_run_explorer(SXMLExplorer* explorer, char *xml) {

  unsigned char result = SXMLExplorerContinue;

  do {

#ifdef  __DEBUG1
    printf("State:%d Buffer:%s CharAddr: %p Char:%c, result %d, length %d\r\n", explorer->state, explorer->buffer, xml, *xml, result, len);
#endif

    switch (explorer->state) {
      case INITIAL:
        switch (*xml) {
          case '<':
            result = priv_sxml_change_explorer_state(explorer, INITIAL);
            continue;
          case '?':
            result = priv_sxml_change_explorer_state(explorer, IN_HEADER);
            continue;
        }
        break;
      case IN_HEADER:
        switch (*xml) {
          case '>':
            result = priv_sxml_change_explorer_state(explorer, IN_CONTENT);
            continue;
        }
        break;
      case IN_TAG:
        switch (*xml) {
          case '>':
            result =  priv_sxml_change_explorer_state(explorer, IN_CONTENT);
            continue;
          case ' ':
            result = priv_sxml_change_explorer_state(explorer, IN_ATTRIBUTE_KEY);
            continue;
        }
        break;
      case IN_ATTRIBUTE_KEY:
        switch (*xml) {
          case '>':
            result = priv_sxml_change_explorer_state(explorer, IN_CONTENT);
            continue;
          case '"':
            assert(explorer->bp > 1);
            explorer->bp--;
            explorer->buffer[explorer->bp] = '\0';
            result = priv_sxml_change_explorer_state(explorer, IN_ATTRIBUTE_VALUE);
            continue;
        }
      case IN_ATTRIBUTE_VALUE:
        switch (*xml) {
          case '"':
            result = priv_sxml_change_explorer_state(explorer, IN_TAG);
            continue;
        }
        break;
      case IN_CONTENT:
        switch (*xml) {
          case '<':
            result = priv_sxml_change_explorer_state(explorer, IN_TAG);
            continue;
        }
        break;
    }
    explorer->buffer[explorer->bp++] = *xml;
    explorer->buffer[explorer->bp] = '\0';

  } while ((*++xml != '\0') && (result == SXMLExplorerContinue));

  if (result == SXMLExplorerStop) {
    return SXMLExplorerInterrupted;
  }

  return SXMLExplorerComplete;

}

