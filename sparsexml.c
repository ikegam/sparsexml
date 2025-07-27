#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "sparsexml-priv.h"

unsigned char priv_sxml_change_explorer_state(SXMLExplorer* explorer, SXMLExplorerState state) {
  unsigned char ret = SXMLExplorerContinue;

  if (strlen(explorer->buffer) > 0) {
    if (explorer->state == IN_TAG && (state == IN_CONTENT || state == IN_TAG || state == IN_ATTRIBUTE_KEY) && explorer->tag_func != NULL) {
      if (explorer->enable_namespace_processing) {
        char* namespace_uri = NULL;
        char* local_name = NULL;
        char tag_copy[SXMLElementLength];
        strcpy(tag_copy, explorer->buffer);
        priv_sxml_process_namespace(tag_copy, &namespace_uri, &local_name);
        // For simplicity, we pass the local name for now
        ret = explorer->tag_func(local_name ? local_name : explorer->buffer);
      } else {
        ret = explorer->tag_func(explorer->buffer);
      }
    } else if (explorer->state == IN_CONTENT && state == IN_TAG && explorer->content_func != NULL) {
      ret = explorer->content_func(explorer->buffer);
    } else if (explorer->state == IN_ATTRIBUTE_KEY && state == IN_ATTRIBUTE_VALUE && explorer->attribute_key_func != NULL) {
      ret = explorer->attribute_key_func(explorer->buffer);
    } else if (explorer->state == IN_ATTRIBUTE_VALUE && state == IN_TAG && explorer->attribute_value_func != NULL) {
      ret = explorer->attribute_value_func(explorer->buffer);
    } else if (explorer->state == IN_COMMENT && state == IN_CONTENT && explorer->comment_func != NULL) {
      ret = explorer->comment_func(explorer->buffer);
    } else if (explorer->state == IN_CDATA && state == IN_CONTENT && explorer->content_func != NULL) {
      ret = explorer->content_func(explorer->buffer);
    }
  }

  explorer->bp = 0;
  explorer->buffer[0] = '\0';

  explorer->state = state;

  return ret;
}

SXMLExplorerState sxml_check_explorer_state(SXMLExplorer* ex) {
  return ex->state;
}

SXMLExplorer* sxml_make_explorer(void) {
  SXMLExplorer* explorer;
  explorer = malloc(sizeof(SXMLExplorer));

  explorer->state = INITIAL;
  explorer->bp = 0;
  explorer->buffer[0] = '\0';
  explorer->header_parsed = 0;
  explorer->comment_depth = 0;
  explorer->cdata_pos = 0;
  explorer->comment_func = NULL;
  explorer->prev_state = INITIAL;
  explorer->entity_bp = 0;
  explorer->entity_buffer[0] = '\0';
  explorer->saved_bp = 0;
  explorer->enable_entity_processing = 0;
  explorer->enable_namespace_processing = 0;
  explorer->enable_extended_entities = 0;
  explorer->enable_numeric_entities = 0;

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

void sxml_register_comment_func(SXMLExplorer* explorer, void* comment) {
  explorer->comment_func = comment;
}

void sxml_enable_entity_processing(SXMLExplorer* explorer, unsigned char enable) {
  explorer->enable_entity_processing = enable;
}

void sxml_enable_namespace_processing(SXMLExplorer* explorer, unsigned char enable) {
  explorer->enable_namespace_processing = enable;
}

void sxml_enable_extended_entities(SXMLExplorer* explorer, unsigned char enable) {
  explorer->enable_extended_entities = enable;
}

void sxml_enable_numeric_entities(SXMLExplorer* explorer, unsigned char enable) {
  explorer->enable_numeric_entities = enable;
}

unsigned char priv_sxml_process_entity(SXMLExplorer* explorer, char* entity_buffer) {
  char replacement = '\0';
  
  // First try standard XML entities
  if (strcmp(entity_buffer, "lt") == 0) {
    replacement = '<';
  } else if (strcmp(entity_buffer, "gt") == 0) {
    replacement = '>';
  } else if (strcmp(entity_buffer, "amp") == 0) {
    replacement = '&';
  } else if (strcmp(entity_buffer, "quot") == 0) {
    replacement = '"';
  } else if (strcmp(entity_buffer, "apos") == 0) {
    replacement = '\'';
  } else {
    // Try numeric entities if enabled
    if (explorer->enable_numeric_entities && (entity_buffer[0] == '#')) {
      return priv_sxml_process_numeric_entity(explorer, entity_buffer);
    }
    // Try extended entities if enabled
    if (explorer->enable_extended_entities) {
      return priv_sxml_process_extended_entity(explorer, entity_buffer);
    }
    return SXMLExplorerErrorInvalidEntity;
  }
  
  if (explorer->bp < SXMLElementLength - 1) {
    explorer->buffer[explorer->bp++] = replacement;
    explorer->buffer[explorer->bp] = '\0';
  } else {
    return SXMLExplorerErrorBufferOverflow;
  }
  
  return SXMLExplorerContinue;
}

unsigned char priv_sxml_process_numeric_entity(SXMLExplorer* explorer, char* entity_buffer) {
  unsigned int codepoint = 0;
  char* end_ptr;
  
  if (entity_buffer[1] == 'x' || entity_buffer[1] == 'X') {
    // Hexadecimal: &#xAB; or &#XAB;
    codepoint = strtoul(entity_buffer + 2, &end_ptr, 16);
  } else {
    // Decimal: &#123;
    codepoint = strtoul(entity_buffer + 1, &end_ptr, 10);
  }
  
  // Basic ASCII range check for embedded systems
  if (codepoint == 0 || codepoint > 127) {
    return SXMLExplorerErrorInvalidEntity;
  }
  
  if (explorer->bp < SXMLElementLength - 1) {
    explorer->buffer[explorer->bp++] = (char)codepoint;
    explorer->buffer[explorer->bp] = '\0';
  } else {
    return SXMLExplorerErrorBufferOverflow;
  }
  
  return SXMLExplorerContinue;
}

unsigned char priv_sxml_process_extended_entity(SXMLExplorer* explorer, char* entity_buffer) {
  // Common HTML entities (keeping it minimal for embedded systems)
  char replacement = '\0';
  
  if (strcmp(entity_buffer, "nbsp") == 0) {
    replacement = ' ';  // Non-breaking space -> regular space
  } else if (strcmp(entity_buffer, "copy") == 0) {
    // For embedded systems, replace with simple text
    const char* copyright_text = "(c)";
    int len = strlen(copyright_text);
    if (explorer->bp + len < SXMLElementLength) {
      strcpy(explorer->buffer + explorer->bp, copyright_text);
      explorer->bp += len;
      return SXMLExplorerContinue;
    } else {
      return SXMLExplorerErrorBufferOverflow;
    }
  } else if (strcmp(entity_buffer, "reg") == 0) {
    const char* reg_text = "(R)";
    int len = strlen(reg_text);
    if (explorer->bp + len < SXMLElementLength) {
      strcpy(explorer->buffer + explorer->bp, reg_text);
      explorer->bp += len;
      return SXMLExplorerContinue;
    } else {
      return SXMLExplorerErrorBufferOverflow;
    }
  } else if (strcmp(entity_buffer, "trade") == 0) {
    const char* trade_text = "(TM)";
    int len = strlen(trade_text);
    if (explorer->bp + len < SXMLElementLength) {
      strcpy(explorer->buffer + explorer->bp, trade_text);
      explorer->bp += len;
      return SXMLExplorerContinue;
    } else {
      return SXMLExplorerErrorBufferOverflow;
    }
  } else if (strcmp(entity_buffer, "euro") == 0) {
    replacement = 'E';  // Euro symbol -> E for ASCII compatibility
  } else if (strcmp(entity_buffer, "pound") == 0) {
    replacement = '#';  // Pound symbol -> # for ASCII compatibility
  } else {
    return SXMLExplorerErrorInvalidEntity;
  }
  
  if (replacement != '\0') {
    if (explorer->bp < SXMLElementLength - 1) {
      explorer->buffer[explorer->bp++] = replacement;
      explorer->buffer[explorer->bp] = '\0';
    } else {
      return SXMLExplorerErrorBufferOverflow;
    }
  }
  
  return SXMLExplorerContinue;
}

void priv_sxml_process_namespace(char* tag_name, char** namespace_uri, char** local_name) {
  char* colon_pos = strchr(tag_name, ':');
  if (colon_pos != NULL) {
    *colon_pos = '\0';  // Split the string
    *namespace_uri = tag_name;
    *local_name = colon_pos + 1;
  } else {
    *namespace_uri = NULL;
    *local_name = tag_name;
  }
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
          default:
            result = priv_sxml_change_explorer_state(explorer, IN_TAG);
        }
        break;
      case IN_DECLARATION:
        switch (*xml) {
          case '>':
            if (explorer->buffer[strlen(explorer->buffer)-1] != '?') {
              break;
            }
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
          case '?':
            if (explorer->buffer[strlen(explorer->buffer)-1] != '<') {
              break;
            }
            result =  priv_sxml_change_explorer_state(explorer, IN_DECLARATION);
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
        break;
      case IN_ATTRIBUTE_VALUE:
        switch (*xml) {
          case '"':
            result = priv_sxml_change_explorer_state(explorer, IN_TAG);
            continue;
          case '&':
            if (explorer->enable_entity_processing) {
              explorer->prev_state = explorer->state;
              explorer->state = IN_ENTITY;
              explorer->entity_bp = 0;
              explorer->entity_buffer[0] = '\0';
              continue;
            }
            break;
        }
        break;
      case IN_CONTENT:
        switch (*xml) {
          case '<':
            // Check for comment start: <!--
            if (*(xml+1) == '!' && *(xml+2) == '-' && *(xml+3) == '-') {
              result = priv_sxml_change_explorer_state(explorer, IN_COMMENT);
              xml += 3; // Skip '!--'
              continue;
            }
            // Check for CDATA start: <![CDATA[
            if (*(xml+1) == '!' && *(xml+2) == '[' && 
                strncmp(xml+3, "CDATA[", 6) == 0) {
              result = priv_sxml_change_explorer_state(explorer, IN_CDATA);
              xml += 8; // Skip '![CDATA['
              continue;
            }
            // Check for DOCTYPE start: <!DOCTYPE
            if (*(xml+1) == '!' && strncmp(xml+2, "DOCTYPE", 7) == 0) {
              result = priv_sxml_change_explorer_state(explorer, IN_DOCTYPE);
              xml += 8; // Skip '!DOCTYPE'
              continue;
            }
            result = priv_sxml_change_explorer_state(explorer, IN_TAG);
            continue;
          case '&':
            if (explorer->enable_entity_processing) {
              explorer->prev_state = explorer->state;
              explorer->state = IN_ENTITY;
              explorer->entity_bp = 0;
              explorer->entity_buffer[0] = '\0';
              continue;
            }
            break;
        }
        break;
      case IN_COMMENT:
        // Look for comment end: -->
        if (*xml == '-' && *(xml+1) == '-' && *(xml+2) == '>') {
          result = priv_sxml_change_explorer_state(explorer, IN_CONTENT);
          xml += 2; // Skip '-->', the '>' will be processed in next iteration
          continue;
        }
        break;
      case IN_CDATA:
        // Look for CDATA end: ]]>
        if (*xml == ']' && *(xml+1) == ']' && *(xml+2) == '>') {
          result = priv_sxml_change_explorer_state(explorer, IN_CONTENT);
          xml += 2; // Skip ']]>', the '>' will be processed in next iteration
          continue;
        }
        break;
      case IN_ENTITY:
        if (*xml == ';') {
          // Process entity reference
          if (explorer->entity_bp < sizeof(explorer->entity_buffer) - 1) {
            explorer->entity_buffer[explorer->entity_bp] = '\0';
            result = priv_sxml_process_entity(explorer, explorer->entity_buffer);
            if (result != SXMLExplorerContinue) {
              return result;
            }
            explorer->state = explorer->prev_state;
            continue;
          } else {
            return SXMLExplorerErrorInvalidEntity;
          }
        } else {
          // Collect entity characters in entity buffer
          if (explorer->entity_bp < sizeof(explorer->entity_buffer) - 1) {
            explorer->entity_buffer[explorer->entity_bp++] = *xml;
          } else {
            return SXMLExplorerErrorInvalidEntity;
          }
        }
        break;
      case IN_DOCTYPE:
        if (*xml == '>') {
          result = priv_sxml_change_explorer_state(explorer, IN_CONTENT);
          continue;
        }
        break;
    }
    // Add character to buffer unless we're in an entity state
    if (explorer->state == IN_ENTITY) {
      // Entity characters are handled in the IN_ENTITY case above
    } else {
      if (explorer->bp < SXMLElementLength - 1) {
        explorer->buffer[explorer->bp++] = *xml;
        explorer->buffer[explorer->bp] = '\0';
      } else {
        // Buffer overflow prevention - truncate
        explorer->buffer[SXMLElementLength - 1] = '\0';
      }
    }

  } while ((*++xml != '\0') && (result == SXMLExplorerContinue));

  if (result == SXMLExplorerStop) {
    return SXMLExplorerInterrupted;
  }

  return SXMLExplorerComplete;

}

unsigned char sxml_run_explorer_exi(SXMLExplorer* explorer, unsigned char* exi,
                                    unsigned int len) {
  unsigned int pos = 0;
  unsigned char result = SXMLExplorerContinue;

  while (pos < len && result == SXMLExplorerContinue) {
    unsigned char token = exi[pos++];

    switch (token) {
      case 0x01: { // Start element
        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char name_len = exi[pos++];
        if (pos + name_len > len || name_len >= SXMLElementLength)
          return SXMLExplorerErrorMalformedXML;
        char name[SXMLElementLength];
        memcpy(name, &exi[pos], name_len);
        name[name_len] = '\0';
        pos += name_len;
        if (explorer->tag_func)
          result = explorer->tag_func(name);
        break;
      }
      case 0x02: { // End element
        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char name_len = exi[pos++];
        if (pos + name_len > len || name_len >= SXMLElementLength - 1)
          return SXMLExplorerErrorMalformedXML;
        char name[SXMLElementLength];
        name[0] = '/';
        memcpy(name + 1, &exi[pos], name_len);
        name[name_len + 1] = '\0';
        pos += name_len;
        if (explorer->tag_func)
          result = explorer->tag_func(name);
        break;
      }
      case 0x03: { // Attribute key/value
        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char key_len = exi[pos++];
        if (pos + key_len > len || key_len >= SXMLElementLength)
          return SXMLExplorerErrorMalformedXML;
        char key[SXMLElementLength];
        memcpy(key, &exi[pos], key_len);
        key[key_len] = '\0';
        pos += key_len;

        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char val_len = exi[pos++];
        if (pos + val_len > len || val_len >= SXMLElementLength)
          return SXMLExplorerErrorMalformedXML;
        char value[SXMLElementLength];
        memcpy(value, &exi[pos], val_len);
        value[val_len] = '\0';
        pos += val_len;

        if (explorer->attribute_key_func)
          result = explorer->attribute_key_func(key);
        if (result != SXMLExplorerContinue) break;
        if (explorer->attribute_value_func)
          result = explorer->attribute_value_func(value);
        break;
      }
      case 0x04: { // Characters
        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char text_len = exi[pos++];
        if (pos + text_len > len || text_len >= SXMLElementLength)
          return SXMLExplorerErrorMalformedXML;
        char text[SXMLElementLength];
        memcpy(text, &exi[pos], text_len);
        text[text_len] = '\0';
        pos += text_len;
        if (explorer->content_func)
          result = explorer->content_func(text);
        break;
      }
      case 0x05: { // Comment
        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char com_len = exi[pos++];
        if (pos + com_len > len || com_len >= SXMLElementLength)
          return SXMLExplorerErrorMalformedXML;
        char comment[SXMLElementLength];
        memcpy(comment, &exi[pos], com_len);
        comment[com_len] = '\0';
        pos += com_len;
        if (explorer->comment_func)
          result = explorer->comment_func(comment);
        break;
      }
      case 0x06: { // Namespace-prefixed tag
        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char uri_len = exi[pos++];
        if (pos + uri_len > len || uri_len >= SXMLElementLength)
          return SXMLExplorerErrorMalformedXML;
        char uri[SXMLElementLength];
        memcpy(uri, &exi[pos], uri_len);
        uri[uri_len] = '\0';
        pos += uri_len;

        if (pos >= len) return SXMLExplorerErrorMalformedXML;
        unsigned char local_name_len = exi[pos++];
        if (pos + local_name_len > len || local_name_len >= SXMLElementLength)
          return SXMLExplorerErrorMalformedXML;
        char local_name[SXMLElementLength];
        memcpy(local_name, &exi[pos], local_name_len);
        local_name[local_name_len] = '\0';
        pos += local_name_len;

        if (uri_len + local_name_len + 1 < SXMLElementLength) {
          char full_name[SXMLElementLength];
          sprintf(full_name, "%s:%s", uri, local_name);
          if (explorer->tag_func)
            result = explorer->tag_func(full_name);
        } else {
          return SXMLExplorerErrorBufferOverflow;
        }
        break;
      }
      case 0xFF:
        return SXMLExplorerComplete;
      default:
        return SXMLExplorerErrorMalformedXML;
    }
  }

  return result == SXMLExplorerStop ? SXMLExplorerInterrupted : SXMLExplorerComplete;
}

