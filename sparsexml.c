#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "sparsexml-priv.h"

// =============================================================================
// BASIC UTILITY FUNCTIONS
// =============================================================================

// Helper to append a single character to the buffer
unsigned char priv_append_char(SXMLExplorer* explorer, char c) {
  if (explorer->bp < SXMLElementLength - 1) {
    explorer->buffer[explorer->bp++] = c;
    explorer->buffer[explorer->bp] = '\0';
    return SXMLExplorerContinue;
  }
  return SXMLExplorerErrorBufferOverflow;
}

// Helper to append a string to the buffer
unsigned char priv_append_string(SXMLExplorer* explorer, const char* str) {
  unsigned int len = strlen(str);
  if (explorer->bp + len < SXMLElementLength) {
    memcpy(explorer->buffer + explorer->bp, str, len);
    explorer->bp += len;
    explorer->buffer[explorer->bp] = '\0';
    return SXMLExplorerContinue;
  }
  return SXMLExplorerErrorBufferOverflow;
}

// Copy a string into the buffer, truncating it if necessary
static void priv_set_buffer(SXMLExplorer* explorer, const char* prefix, const char* str) {
  explorer->bp = 0;
  explorer->buffer[0] = '\0';
  while (*prefix && explorer->bp < SXMLElementLength - 1) {
    explorer->buffer[explorer->bp++] = *prefix++;
  }
  while (*str && explorer->bp < SXMLElementLength - 1) {
    explorer->buffer[explorer->bp++] = *str++;
  }
  explorer->buffer[explorer->bp] = '\0';
}

// Encode a Unicode code point as UTF-8, returns the number of bytes (0 if invalid)
static unsigned int priv_encode_utf8(unsigned long cp, char* out) {
  if (cp < 0x80) {
    out[0] = (char)cp;
    return 1;
  } else if (cp < 0x800) {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  } else if (cp < 0x10000) {
    if (cp >= 0xD800 && cp <= 0xDFFF) return 0;
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  } else if (cp <= 0x10FFFF) {
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
  }
  return 0;
}

static int priv_is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// =============================================================================
// EXPLORER LIFECYCLE MANAGEMENT
// =============================================================================

unsigned int sxml_explorer_size(void) {
  return sizeof(SXMLExplorer);
}

SXMLExplorer* sxml_init_explorer(void* memory, unsigned int size) {
  SXMLExplorer* explorer = memory;

  if (explorer == NULL || size < sizeof(SXMLExplorer)) {
    return NULL;
  }
  memset(explorer, 0, sizeof(SXMLExplorer));
  sxml_reset_explorer(explorer);

  return explorer;
}

SXMLExplorer* sxml_make_explorer(void) {
  SXMLExplorer* explorer = sxml_init_explorer(malloc(sizeof(SXMLExplorer)), sizeof(SXMLExplorer));
  if (explorer != NULL) {
    explorer->heap_allocated = 1;
  }
  return explorer;
}

void sxml_destroy_explorer(SXMLExplorer *explorer) {
  if (explorer != NULL && explorer->heap_allocated) {
    free(explorer);
  }
}

void sxml_reset_explorer(SXMLExplorer* explorer) {
  explorer->state = INITIAL;
  explorer->prev_state = INITIAL;
  explorer->bp = 0;
  explorer->buffer[0] = '\0';
  explorer->entity_bp = 0;
  explorer->entity_buffer[0] = '\0';
  explorer->tag_name[0] = '\0';
  explorer->last[0] = explorer->last[1] = '\0';
  explorer->quote = '"';
  explorer->self_closing = 0;
  explorer->doctype_depth = 0;
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

void sxml_set_exi_options(SXMLExplorer* explorer, unsigned char options) {
  explorer->exi_options = options;
}

void sxml_set_exi_workspace(SXMLExplorer* explorer, void* workspace, unsigned int size) {
  explorer->exi_workspace = workspace;
  explorer->exi_workspace_size = workspace ? size : 0;
}

// =============================================================================
// XML PARSING: NAMESPACE PROCESSING
// =============================================================================

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

// =============================================================================
// XML PARSING: ENTITY PROCESSING
// =============================================================================

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

  return priv_append_char(explorer, replacement);
}

unsigned char priv_sxml_process_numeric_entity(SXMLExplorer* explorer, char* entity_buffer) {
  unsigned long codepoint = 0;
  char* digits;
  char* end_ptr;
  char utf8[5];
  unsigned int len;

  if (entity_buffer[1] == 'x' || entity_buffer[1] == 'X') {
    // Hexadecimal: &#xAB; or &#XAB;
    digits = entity_buffer + 2;
    codepoint = strtoul(digits, &end_ptr, 16);
  } else {
    // Decimal: &#123;
    digits = entity_buffer + 1;
    codepoint = strtoul(digits, &end_ptr, 10);
  }

  if (end_ptr == digits || *end_ptr != '\0' || codepoint == 0) {
    return SXMLExplorerErrorInvalidEntity;
  }

  // Non-ASCII characters are delivered as UTF-8
  len = priv_encode_utf8(codepoint, utf8);
  if (len == 0) {
    return SXMLExplorerErrorInvalidEntity;
  }
  utf8[len] = '\0';

  return priv_append_string(explorer, utf8);
}

unsigned char priv_sxml_process_extended_entity(SXMLExplorer* explorer, char* entity_buffer) {
  // Common HTML entities (keeping it minimal for embedded systems)
  char replacement = '\0';

  if (strcmp(entity_buffer, "nbsp") == 0) {
    replacement = ' ';  // Non-breaking space -> regular space
  } else if (strcmp(entity_buffer, "copy") == 0) {
    // For embedded systems, replace with simple text
    const char* copyright_text = "(c)";
    return priv_append_string(explorer, copyright_text);
  } else if (strcmp(entity_buffer, "reg") == 0) {
    const char* reg_text = "(R)";
    return priv_append_string(explorer, reg_text);
  } else if (strcmp(entity_buffer, "trade") == 0) {
    const char* trade_text = "(TM)";
    return priv_append_string(explorer, trade_text);
  } else if (strcmp(entity_buffer, "euro") == 0) {
    replacement = 'E';  // Euro symbol -> E for ASCII compatibility
  } else if (strcmp(entity_buffer, "pound") == 0) {
    replacement = '#';  // Pound symbol -> # for ASCII compatibility
  } else {
    return SXMLExplorerErrorInvalidEntity;
  }

  if (replacement != '\0') {
    return priv_append_char(explorer, replacement);
  }

  return SXMLExplorerContinue;
}

// =============================================================================
// XML PARSING: STATE MANAGEMENT
// =============================================================================

// Report the tag name in the buffer and remember it for <empty/> elements
static unsigned char priv_sxml_emit_tag(SXMLExplorer* explorer) {
  char* name = explorer->buffer;

  if (explorer->enable_namespace_processing) {
    char* colon_pos = strchr(name, ':');
    if (colon_pos != NULL) {
      if (name[0] == '/') {
        // "/ns:tag" -> "/tag"
        memmove(name + 1, colon_pos + 1, strlen(colon_pos + 1) + 1);
      } else {
        name = colon_pos + 1;
      }
    }
  }

  {
    // Names longer than SXMLNameLength are truncated
    unsigned int i;
    for (i = 0; i < SXMLNameLength - 1 && name[i] != '\0'; i++) {
      explorer->tag_name[i] = name[i];
    }
    explorer->tag_name[i] = '\0';
  }

  if (explorer->tag_func == NULL) {
    return SXMLExplorerContinue;
  }
  return explorer->tag_func(name);
}

unsigned char priv_sxml_change_explorer_state(SXMLExplorer* explorer, SXMLExplorerState state) {
  unsigned char ret = SXMLExplorerContinue;
  SXMLExplorerState from = explorer->state;

  if (from == IN_ATTRIBUTE_VALUE && state == IN_ATTRIBUTE_KEY) {
    // Attribute values are reported even when empty
    if (explorer->attribute_value_func != NULL) {
      ret = explorer->attribute_value_func(explorer->buffer);
    }
  } else if (explorer->bp > 0) {
    if (from == IN_TAG && (state == IN_CONTENT || state == IN_ATTRIBUTE_KEY)) {
      ret = priv_sxml_emit_tag(explorer);
    } else if ((from == IN_CONTENT || from == INITIAL) && state == IN_TAG && explorer->content_func != NULL) {
      ret = explorer->content_func(explorer->buffer);
    } else if (from == IN_ATTRIBUTE_KEY && state == IN_ATTRIBUTE_EQUAL && explorer->attribute_key_func != NULL) {
      ret = explorer->attribute_key_func(explorer->buffer);
    } else if (from == IN_COMMENT && state == IN_CONTENT && explorer->comment_func != NULL) {
      ret = explorer->comment_func(explorer->buffer);
    } else if (from == IN_CDATA && state == IN_CONTENT && explorer->content_func != NULL) {
      ret = explorer->content_func(explorer->buffer);
    }
  }

  explorer->bp = 0;
  explorer->buffer[0] = '\0';
  explorer->last[0] = explorer->last[1] = '\0';

  explorer->state = state;

  return ret;
}

// Report the end of an <empty/> element
static unsigned char priv_sxml_emit_self_closing(SXMLExplorer* explorer) {
  unsigned char ret = SXMLExplorerContinue;

  explorer->self_closing = 0;
  if (explorer->tag_func != NULL && explorer->tag_name[0] != '\0') {
    priv_set_buffer(explorer, "/", explorer->tag_name);
    ret = explorer->tag_func(explorer->buffer);
    explorer->bp = 0;
    explorer->buffer[0] = '\0';
  }
  return ret;
}

static void priv_sxml_begin_entity(SXMLExplorer* explorer) {
  explorer->prev_state = explorer->state;
  explorer->state = IN_ENTITY;
  explorer->entity_bp = 0;
  explorer->entity_buffer[0] = '\0';
}

// =============================================================================
// XML MAIN PARSER
// =============================================================================

// Feed one character to the state machine. All state lives in the explorer,
// so documents can be split at any position across sxml_run_explorer() calls.
static unsigned char priv_sxml_feed_char(SXMLExplorer* explorer, char c) {
  unsigned char ret = SXMLExplorerContinue;

  switch (explorer->state) {
    case INITIAL:
    case IN_CONTENT:
      if (c == '<') {
        return priv_sxml_change_explorer_state(explorer, IN_TAG);
      }
      if (c == '&' && explorer->enable_entity_processing) {
        priv_sxml_begin_entity(explorer);
        return SXMLExplorerContinue;
      }
      break;

    case IN_TAG:
      if (explorer->bp == 0) {
        explorer->self_closing = 0;
        if (c == '?') {
          // XML declaration or processing instruction
          explorer->state = IN_DECLARATION;
          return SXMLExplorerContinue;
        }
        if (priv_is_space(c)) {
          return SXMLExplorerContinue;
        }
      }
      if ((explorer->bp == 0 && c == '!') || explorer->buffer[0] == '!') {
        // Markup declaration: comment, CDATA section or DOCTYPE
        priv_append_char(explorer, c);
        if (strcmp(explorer->buffer, "!--") == 0) {
          return priv_sxml_change_explorer_state(explorer, IN_COMMENT);
        } else if (strcmp(explorer->buffer, "![CDATA[") == 0) {
          return priv_sxml_change_explorer_state(explorer, IN_CDATA);
        } else if (strcmp(explorer->buffer, "!DOCTYPE") == 0) {
          explorer->doctype_depth = 0;
          return priv_sxml_change_explorer_state(explorer, IN_DOCTYPE);
        } else if (c == '>') {
          // Unknown declaration, skip it
          return priv_sxml_change_explorer_state(explorer, IN_CONTENT);
        }
        return SXMLExplorerContinue;
      }
      if (c == '>') {
        return priv_sxml_change_explorer_state(explorer, IN_CONTENT);
      }
      if (priv_is_space(c)) {
        return priv_sxml_change_explorer_state(explorer, IN_ATTRIBUTE_KEY);
      }
      if (c == '/' && explorer->bp > 0) {
        ret = priv_sxml_change_explorer_state(explorer, IN_ATTRIBUTE_KEY);
        explorer->self_closing = 1;
        return ret;
      }
      break;

    case IN_ATTRIBUTE_KEY:
      if (c == '>') {
        ret = priv_sxml_change_explorer_state(explorer, IN_CONTENT);
        if (ret == SXMLExplorerContinue && explorer->self_closing) {
          ret = priv_sxml_emit_self_closing(explorer);
        }
        return ret;
      }
      if (c == '/') {
        explorer->self_closing = 1;
        return SXMLExplorerContinue;
      }
      if (c == '=') {
        return priv_sxml_change_explorer_state(explorer, IN_ATTRIBUTE_EQUAL);
      }
      if (priv_is_space(c)) {
        return SXMLExplorerContinue;
      }
      explorer->self_closing = 0;
      break;

    case IN_ATTRIBUTE_EQUAL:
      if (c == '"' || c == '\'') {
        explorer->quote = c;
        explorer->state = IN_ATTRIBUTE_VALUE;
      } else if (c == '>') {
        // Attribute without value
        return priv_sxml_change_explorer_state(explorer, IN_CONTENT);
      }
      return SXMLExplorerContinue;

    case IN_ATTRIBUTE_VALUE:
      if (c == explorer->quote) {
        return priv_sxml_change_explorer_state(explorer, IN_ATTRIBUTE_KEY);
      }
      if (c == '&' && explorer->enable_entity_processing) {
        priv_sxml_begin_entity(explorer);
        return SXMLExplorerContinue;
      }
      break;

    case IN_DECLARATION:
      if (c == '>' && explorer->last[1] == '?') {
        return priv_sxml_change_explorer_state(explorer, IN_CONTENT);
      }
      break;

    case IN_COMMENT:
      if (c == '>' && explorer->last[0] == '-' && explorer->last[1] == '-') {
        if (explorer->bp >= 2 && strcmp(explorer->buffer + explorer->bp - 2, "--") == 0) {
          explorer->bp -= 2;
          explorer->buffer[explorer->bp] = '\0';
        }
        return priv_sxml_change_explorer_state(explorer, IN_CONTENT);
      }
      break;

    case IN_CDATA:
      if (c == '>' && explorer->last[0] == ']' && explorer->last[1] == ']') {
        if (explorer->bp >= 2 && strcmp(explorer->buffer + explorer->bp - 2, "]]") == 0) {
          explorer->bp -= 2;
          explorer->buffer[explorer->bp] = '\0';
        }
        return priv_sxml_change_explorer_state(explorer, IN_CONTENT);
      }
      break;

    case IN_ENTITY:
      if (c == ';') {
        explorer->entity_buffer[explorer->entity_bp] = '\0';
        explorer->state = explorer->prev_state;
        return priv_sxml_process_entity(explorer, explorer->entity_buffer);
      }
      if (c == '<' || c == '&' || priv_is_space(c) ||
          explorer->entity_bp >= sizeof(explorer->entity_buffer) - 1) {
        return SXMLExplorerErrorInvalidEntity;
      }
      explorer->entity_buffer[explorer->entity_bp++] = c;
      return SXMLExplorerContinue;

    case IN_DOCTYPE:
      if (c == '[') {
        explorer->doctype_depth++;
      } else if (c == ']' && explorer->doctype_depth > 0) {
        explorer->doctype_depth--;
      } else if (c == '>' && explorer->doctype_depth == 0) {
        return priv_sxml_change_explorer_state(explorer, IN_CONTENT);
      }
      break;
  }

  // Buffer overflow prevention - data beyond the buffer is truncated
  if (explorer->bp < SXMLElementLength - 1) {
    explorer->buffer[explorer->bp++] = c;
    explorer->buffer[explorer->bp] = '\0';
  }
  explorer->last[0] = explorer->last[1];
  explorer->last[1] = c;

  return ret;
}

unsigned char sxml_run_explorer(SXMLExplorer* explorer, char *xml) {
  unsigned char result = SXMLExplorerContinue;

  if (explorer == NULL || xml == NULL) {
    return SXMLExplorerErrorMalformedXML;
  }

  while (*xml != '\0' && result == SXMLExplorerContinue) {
    result = priv_sxml_feed_char(explorer, *xml++);
  }

  if (result == SXMLExplorerContinue) {
    return SXMLExplorerComplete;
  }
  if (result == SXMLExplorerStop) {
    return SXMLExplorerInterrupted;
  }
  return result;
}

// =============================================================================
// EXI SUPPORT: SCHEMA-LESS DECODER
// =============================================================================
//
// Decodes W3C EXI 1.0 streams that were encoded without a schema, using the
// built-in document and element grammars (spec section 8.4) together with
// grammar learning and the string table (section 7.3). Supported are
// bit-packed and byte-aligned streams with the fidelity options set by
// sxml_set_exi_options(). EXI options embedded in the header, compression,
// pre-compression and self-contained elements are not supported.
// Define SXML_NO_EXI to leave the decoder out of XML-only builds.

#ifdef SXML_NO_EXI

unsigned char sxml_run_explorer_exi(SXMLExplorer* explorer, unsigned char* exi, unsigned int len) {
  (void)explorer;
  (void)exi;
  (void)len;
  return SXMLExplorerErrorUnsupported;
}

#else

typedef struct ExiNode ExiNode;
typedef struct ExiProd ExiProd;
typedef struct ExiQName ExiQName;
typedef struct ExiUri ExiUri;

struct ExiNode {         // Entry of a string table partition
  ExiNode* next;
  const char* str;
};

enum { EXI_PROD_SE, EXI_PROD_AT, EXI_PROD_CH, EXI_PROD_EE };

struct ExiProd {         // Learned production
  ExiProd* next;
  ExiQName* qname;
  unsigned char type;
};

typedef struct {
  ExiProd* prods;        // Newest first, so the list position is the event code
  unsigned int count;
  unsigned char learned_ch;
} ExiGrammar;

struct ExiQName {
  ExiQName* next;
  ExiUri* uri;
  unsigned int id;
  const char* local;
  ExiGrammar start_tag;  // Built-in element grammar: StartTagContent
  ExiGrammar content;    // Built-in element grammar: ElementContent
  unsigned char ready;
  unsigned char learned_ee;
  unsigned char learned_xsi_type;
  ExiNode* values;       // Local value partition
  unsigned int value_count;
};

struct ExiUri {
  ExiUri* next;
  unsigned int id;
  ExiQName* names;
  unsigned int name_count;
  unsigned int prefix_count;
};

typedef struct {
  ExiQName* qname;
  unsigned char in_content;
} ExiFrame;

typedef struct {
  SXMLExplorer* explorer;
  const unsigned char* data;
  unsigned int len;
  unsigned int pos;
  unsigned int bit;
  unsigned char options;
  unsigned char* arena;
  unsigned int arena_size;
  unsigned int arena_used;
  ExiUri* uris;
  unsigned int uri_count;
  ExiNode* values;       // Global value partition
  unsigned int value_count;
  ExiFrame* stack;
  unsigned int depth;
  unsigned char error;
} ExiDecoder;

static const char* exi_xml_names[] = {"base", "id", "lang", "space"};
static const char* exi_xsi_names[] = {"nil", "type"};

static void* exi_alloc(ExiDecoder* d, unsigned int size) {
  uintptr_t base = (uintptr_t)d->arena;
  uintptr_t start = (base + d->arena_used + sizeof(void*) - 1) & ~(uintptr_t)(sizeof(void*) - 1);
  unsigned int offset = (unsigned int)(start - base);

  if (d->error) {
    return NULL;
  }
  if (offset > d->arena_size || size > d->arena_size - offset) {
    d->error = SXMLExplorerErrorBufferOverflow;
    return NULL;
  }
  d->arena_used = offset + size;
  memset(d->arena + offset, 0, size);
  return d->arena + offset;
}

// Number of bits needed to represent n distinct values
static unsigned int exi_bits_for(unsigned int n) {
  unsigned int bits = 0;
  if (n <= 1) {
    return 0;
  }
  n--;
  while (n) {
    bits++;
    n >>= 1;
  }
  return bits;
}

static unsigned int exi_read_raw_bits(ExiDecoder* d, unsigned int n) {
  unsigned int value = 0;
  while (n--) {
    if (d->pos >= d->len) {
      d->error = SXMLExplorerErrorMalformedXML;
      return 0;
    }
    value = (value << 1) | ((d->data[d->pos] >> (7 - d->bit)) & 1);
    if (++d->bit == 8) {
      d->bit = 0;
      d->pos++;
    }
  }
  return value;
}

// n-bit unsigned integer (spec 7.1.9)
static unsigned int exi_read_bits(ExiDecoder* d, unsigned int n) {
  unsigned int value = 0;
  unsigned int shift;

  if (d->error) {
    return 0;
  }
  if (!(d->options & SXML_EXI_BYTE_ALIGNED)) {
    return exi_read_raw_bits(d, n);
  }
  for (shift = 0; shift < n; shift += 8) {
    if (d->pos >= d->len) {
      d->error = SXMLExplorerErrorMalformedXML;
      return 0;
    }
    value |= (unsigned int)d->data[d->pos++] << shift;
  }
  return value;
}

static unsigned int exi_read_boolean(ExiDecoder* d) {
  return exi_read_bits(d, (d->options & SXML_EXI_BYTE_ALIGNED) ? 8 : 1) != 0;
}

// Unsigned integer (spec 7.1.6)
static unsigned int exi_read_uint(ExiDecoder* d) {
  unsigned int value = 0;
  unsigned int shift = 0;
  unsigned int octet;

  do {
    octet = exi_read_bits(d, 8);
    if (shift > 28) {
      d->error = SXMLExplorerErrorMalformedXML;
    }
    if (d->error) {
      return 0;
    }
    value |= (octet & 0x7F) << shift;
    shift += 7;
  } while (octet & 0x80);

  return value;
}

// Read `count` characters as UTF-8. Strings that enter a string table are
// kept in the workspace, others are written to the explorer buffer.
static char* exi_read_chars(ExiDecoder* d, unsigned int count, unsigned char keep) {
  char* out;
  unsigned int cap;
  unsigned int n = 0;
  char utf8[4];

  if (keep) {
    out = exi_alloc(d, 1);
    if (out == NULL) {
      return NULL;
    }
    cap = d->arena_size - (unsigned int)((unsigned char*)out - d->arena);
  } else {
    out = d->explorer->buffer;
    cap = SXMLElementLength;
  }

  while (count-- > 0) {
    unsigned int k = priv_encode_utf8(exi_read_uint(d), utf8);
    if (d->error) {
      return NULL;
    }
    if (k == 0) {
      d->error = SXMLExplorerErrorMalformedXML;
      return NULL;
    }
    if (n + k < cap) {
      memcpy(out + n, utf8, k);
      n += k;
    } else if (keep) {
      d->error = SXMLExplorerErrorBufferOverflow;
      return NULL;
    }
  }
  out[n] = '\0';

  if (keep) {
    d->arena_used += n;
  } else {
    d->explorer->bp = n;
  }
  return out;
}

static char* exi_read_string(ExiDecoder* d, unsigned char keep) {
  unsigned int count = exi_read_uint(d);
  if (d->error) {
    return NULL;
  }
  return exi_read_chars(d, count, keep);
}

static ExiUri* exi_add_uri(ExiDecoder* d, unsigned int prefix_count) {
  ExiUri* uri = exi_alloc(d, sizeof(ExiUri));
  if (uri != NULL) {
    uri->id = d->uri_count++;
    uri->prefix_count = prefix_count;
    uri->next = d->uris;
    d->uris = uri;
  }
  return uri;
}

static ExiQName* exi_add_qname(ExiDecoder* d, ExiUri* uri, const char* local) {
  ExiQName* qname = exi_alloc(d, sizeof(ExiQName));
  if (qname != NULL) {
    qname->uri = uri;
    qname->id = uri->name_count++;
    qname->local = local;
    qname->next = uri->names;
    uri->names = qname;
  }
  return qname;
}

static const char* exi_find_string(ExiDecoder* d, ExiNode* list, unsigned int count, unsigned int id) {
  if (id >= count) {
    d->error = SXMLExplorerErrorMalformedXML;
    return NULL;
  }
  for (id = count - 1 - id; id > 0; id--) {
    list = list->next;
  }
  return list->str;
}

static ExiUri* exi_decode_uri(ExiDecoder* d) {
  unsigned int id = exi_read_bits(d, exi_bits_for(d->uri_count + 1));
  ExiUri* uri;

  if (d->error) {
    return NULL;
  }
  if (id == 0) {
    // URI miss: only the table size matters, the string is not reported
    if (exi_read_string(d, 0) == NULL) {
      return NULL;
    }
    return exi_add_uri(d, 0);
  }
  id--;
  if (id >= d->uri_count) {
    d->error = SXMLExplorerErrorMalformedXML;
    return NULL;
  }
  for (uri = d->uris; uri->id != id; uri = uri->next);
  return uri;
}

static ExiQName* exi_decode_local_name(ExiDecoder* d, ExiUri* uri) {
  unsigned int length = exi_read_uint(d);
  unsigned int id;
  ExiQName* qname;

  if (d->error) {
    return NULL;
  }
  if (length > 0) {
    const char* local = exi_read_chars(d, length - 1, 1);
    return local ? exi_add_qname(d, uri, local) : NULL;
  }
  id = exi_read_bits(d, exi_bits_for(uri->name_count));
  if (d->error) {
    return NULL;
  }
  if (id >= uri->name_count) {
    d->error = SXMLExplorerErrorMalformedXML;
    return NULL;
  }
  for (qname = uri->names; qname->id != id; qname = qname->next);
  return qname;
}

static ExiQName* exi_decode_qname(ExiDecoder* d) {
  ExiUri* uri = exi_decode_uri(d);
  return uri ? exi_decode_local_name(d, uri) : NULL;
}

// Prefix of a QName, only present when prefixes are preserved
static void exi_decode_qname_prefix(ExiDecoder* d, ExiQName* qname) {
  if ((d->options & SXML_EXI_PRESERVE_PREFIXES) && qname->uri->id != 0 &&
      qname->uri->prefix_count > 1) {
    exi_read_bits(d, exi_bits_for(qname->uri->prefix_count));
  }
}

static void exi_decode_namespace_declaration(ExiDecoder* d) {
  ExiUri* uri = exi_decode_uri(d);
  if (uri == NULL) {
    return;
  }
  if (exi_read_bits(d, exi_bits_for(uri->prefix_count + 1)) == 0 && !d->error) {
    if (exi_read_string(d, 0) != NULL) {
      uri->prefix_count++;
    }
  }
  exi_read_boolean(d); // local-element-ns
}

// String value with the string table (spec 7.3.3)
static const char* exi_decode_value(ExiDecoder* d, ExiQName* context) {
  unsigned int kind = exi_read_uint(d);
  const char* value;
  ExiNode* global;
  ExiNode* local;

  if (d->error) {
    return NULL;
  }
  if (kind == 0) {
    unsigned int id = exi_read_bits(d, exi_bits_for(context->value_count));
    return d->error ? NULL : exi_find_string(d, context->values, context->value_count, id);
  }
  if (kind == 1) {
    unsigned int id = exi_read_bits(d, exi_bits_for(d->value_count));
    return d->error ? NULL : exi_find_string(d, d->values, d->value_count, id);
  }
  if (kind == 2) {
    return "";  // Empty strings are not added to the table
  }

  value = exi_read_chars(d, kind - 2, 1);
  global = exi_alloc(d, sizeof(ExiNode));
  local = exi_alloc(d, sizeof(ExiNode));
  if (value == NULL || global == NULL || local == NULL) {
    return NULL;
  }
  global->str = local->str = value;
  global->next = d->values;
  d->values = global;
  d->value_count++;
  local->next = context->values;
  context->values = local;
  context->value_count++;
  return value;
}

static void exi_learn(ExiDecoder* d, ExiGrammar* grammar, unsigned char type, ExiQName* qname) {
  ExiProd* prod = exi_alloc(d, sizeof(ExiProd));
  if (prod != NULL) {
    prod->type = type;
    prod->qname = qname;
    prod->next = grammar->prods;
    grammar->prods = prod;
    grammar->count++;
  }
}

static unsigned char exi_callback(ExiDecoder* d, unsigned char (*func)(char*), const char* prefix, const char* text) {
  if (func == NULL || d->error) {
    return SXMLExplorerContinue;
  }
  priv_set_buffer(d->explorer, prefix, text);
  return func(d->explorer->buffer);
}

static unsigned char exi_start_element(ExiDecoder* d, ExiQName* qname) {
  ExiFrame* frame;

  if (qname == NULL) {
    return SXMLExplorerContinue;
  }
  if (d->depth >= SXML_EXI_MAX_DEPTH) {
    d->error = SXMLExplorerErrorBufferOverflow;
    return SXMLExplorerContinue;
  }
  if (!qname->ready) {
    // ElementContent starts with EE on the first level
    exi_learn(d, &qname->content, EXI_PROD_EE, NULL);
    qname->ready = 1;
  }
  exi_decode_qname_prefix(d, qname);

  frame = &d->stack[d->depth++];
  frame->qname = qname;
  frame->in_content = 0;
  return exi_callback(d, d->explorer->tag_func, "", qname->local);
}

static unsigned char exi_end_element(ExiDecoder* d) {
  ExiQName* qname = d->stack[--d->depth].qname;
  return exi_callback(d, d->explorer->tag_func, "/", qname->local);
}

static unsigned char exi_attribute(ExiDecoder* d, ExiQName* qname) {
  const char* value;
  unsigned char ret;

  if (qname == NULL) {
    return SXMLExplorerContinue;
  }
  exi_decode_qname_prefix(d, qname);

  if (qname->uri->id == 2 && qname->id == 1 && !(d->options & SXML_EXI_PRESERVE_LEXICAL_VALUES)) {
    // xsi:type carries a QName
    ExiQName* type = exi_decode_qname(d);
    if (type == NULL) {
      return SXMLExplorerContinue;
    }
    exi_decode_qname_prefix(d, type);
    value = type->local;
  } else {
    value = exi_decode_value(d, qname);
  }
  if (value == NULL) {
    return SXMLExplorerContinue;
  }

  ret = exi_callback(d, d->explorer->attribute_key_func, "", qname->local);
  if (ret != SXMLExplorerContinue) {
    return ret;
  }
  return exi_callback(d, d->explorer->attribute_value_func, "", value);
}

static unsigned char exi_characters(ExiDecoder* d, ExiQName* context) {
  const char* value = exi_decode_value(d, context);
  if (value == NULL || value[0] == '\0') {
    return SXMLExplorerContinue;
  }
  return exi_callback(d, d->explorer->content_func, "", value);
}

// Third level events: CM and PI
static unsigned char exi_misc(ExiDecoder* d) {
  unsigned char comments = (d->options & SXML_EXI_PRESERVE_COMMENTS) != 0;
  unsigned char pis = (d->options & SXML_EXI_PRESERVE_PIS) != 0;
  unsigned int code = exi_read_bits(d, exi_bits_for(comments + pis));

  if (d->error) {
    return SXMLExplorerContinue;
  }
  if (comments && code == 0) {
    if (exi_read_string(d, 0) == NULL) {
      return SXMLExplorerContinue;
    }
    if (d->explorer->comment_func == NULL || d->explorer->bp == 0) {
      return SXMLExplorerContinue;
    }
    return d->explorer->comment_func(d->explorer->buffer);
  }
  if (pis) {
    exi_read_string(d, 0);  // target
    exi_read_string(d, 0);  // data
    return SXMLExplorerContinue;
  }
  d->error = SXMLExplorerErrorMalformedXML;
  return SXMLExplorerContinue;
}

static void exi_doctype(ExiDecoder* d) {
  int i;
  for (i = 0; i < 4 && !d->error; i++) {
    exi_read_string(d, 0);  // name, public, system, text
  }
}

// Second level events of StartTagContent and ElementContent
static unsigned char exi_undeclared_event(ExiDecoder* d, ExiFrame* frame, ExiGrammar* grammar) {
  unsigned char prefixes = (d->options & SXML_EXI_PRESERVE_PREFIXES) != 0;
  unsigned char dtd = (d->options & SXML_EXI_PRESERVE_DTD) != 0;
  unsigned char misc = (d->options & (SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS)) != 0;
  ExiQName* qname = frame->qname;
  unsigned int code;

  if (!frame->in_content) {
    code = exi_read_bits(d, exi_bits_for(4 + prefixes + dtd + misc));
    if (d->error) {
      return SXMLExplorerContinue;
    }
    if (code == 0) {
      // EE
      if (!qname->learned_ee) {
        exi_learn(d, grammar, EXI_PROD_EE, NULL);
        qname->learned_ee = 1;
      }
      return exi_end_element(d);
    }
    if (code == 1) {
      // AT(*), xsi:type is learned only once
      ExiQName* at = exi_decode_qname(d);
      if (at == NULL) {
        return SXMLExplorerContinue;
      }
      if (at->uri->id == 2 && at->id == 1) {
        if (!qname->learned_xsi_type) {
          exi_learn(d, grammar, EXI_PROD_AT, at);
          qname->learned_xsi_type = 1;
        }
      } else {
        exi_learn(d, grammar, EXI_PROD_AT, at);
      }
      return exi_attribute(d, at);
    }
    code -= 2;
    if (prefixes) {
      if (code == 0) {
        exi_decode_namespace_declaration(d);
        return SXMLExplorerContinue;
      }
      code--;
    }
  } else {
    code = exi_read_bits(d, exi_bits_for(2 + dtd + misc));
    if (d->error) {
      return SXMLExplorerContinue;
    }
  }

  if (code == 0) {
    // SE(*)
    ExiQName* se = exi_decode_qname(d);
    if (se == NULL) {
      return SXMLExplorerContinue;
    }
    exi_learn(d, grammar, EXI_PROD_SE, se);
    frame->in_content = 1;
    return exi_start_element(d, se);
  }
  if (code == 1) {
    // CH
    if (!grammar->learned_ch) {
      exi_learn(d, grammar, EXI_PROD_CH, NULL);
      grammar->learned_ch = 1;
    }
    frame->in_content = 1;
    return exi_characters(d, qname);
  }
  code -= 2;
  if (dtd) {
    if (code == 0) {
      // ER
      exi_read_string(d, 0);
      frame->in_content = 1;
      return SXMLExplorerContinue;
    }
    code--;
  }
  if (misc && code == 0) {
    frame->in_content = 1;
    return exi_misc(d);
  }
  d->error = SXMLExplorerErrorMalformedXML;
  return SXMLExplorerContinue;
}

static unsigned char exi_element_event(ExiDecoder* d) {
  ExiFrame* frame = &d->stack[d->depth - 1];
  ExiGrammar* grammar = frame->in_content ? &frame->qname->content : &frame->qname->start_tag;
  unsigned int code = exi_read_bits(d, exi_bits_for(grammar->count + 1));
  ExiProd* prod;

  if (d->error) {
    return SXMLExplorerContinue;
  }
  if (code >= grammar->count) {
    return exi_undeclared_event(d, frame, grammar);
  }

  for (prod = grammar->prods; code > 0; code--) {
    prod = prod->next;
  }
  switch (prod->type) {
    case EXI_PROD_SE:
      frame->in_content = 1;
      return exi_start_element(d, prod->qname);
    case EXI_PROD_AT:
      return exi_attribute(d, prod->qname);
    case EXI_PROD_CH:
      frame->in_content = 1;
      return exi_characters(d, frame->qname);
    default:
      return exi_end_element(d);
  }
}

// EXI header (spec section 5)
static void exi_decode_header(ExiDecoder* d) {
  unsigned int version = 0;
  unsigned int chunk;

  if (d->len >= 1 && d->data[0] == '$') {
    if (d->len < 4 || memcmp(d->data, "$EXI", 4) != 0) {
      d->error = SXMLExplorerErrorMalformedXML;
      return;
    }
    d->pos = 4;
  }
  if (exi_read_raw_bits(d, 2) != 2) {             // Distinguishing bits "10"
    d->error = SXMLExplorerErrorMalformedXML;
    return;
  }
  if (exi_read_raw_bits(d, 1) || exi_read_raw_bits(d, 1)) {
    // Options in the header or a preview version
    if (!d->error) {
      d->error = SXMLExplorerErrorUnsupported;
    }
    return;
  }
  do {
    chunk = exi_read_raw_bits(d, 4);
    version += chunk;
  } while (chunk == 15 && !d->error);
  if (!d->error && version != 0) {
    d->error = SXMLExplorerErrorUnsupported;
  }
  if ((d->options & SXML_EXI_BYTE_ALIGNED) && d->bit != 0) {
    d->bit = 0;
    d->pos++;
  }
}

static unsigned char exi_decode(ExiDecoder* d) {
  unsigned char dtd = (d->options & SXML_EXI_PRESERVE_DTD) != 0;
  unsigned char misc = (d->options & (SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS)) != 0;
  unsigned char result = SXMLExplorerContinue;
  ExiUri* uri;
  unsigned int i;

  // Initial string table (spec appendix D)
  exi_add_uri(d, 1);
  uri = exi_add_uri(d, 1);
  for (i = 0; uri && i < 4; i++) {
    exi_add_qname(d, uri, exi_xml_names[i]);
  }
  uri = exi_add_uri(d, 1);
  for (i = 0; uri && i < 2; i++) {
    exi_add_qname(d, uri, exi_xsi_names[i]);
  }
  d->stack = exi_alloc(d, sizeof(ExiFrame) * SXML_EXI_MAX_DEPTH);

  exi_decode_header(d);

  // Document: SD DocContent
  while (!d->error && result == SXMLExplorerContinue) {
    if (exi_read_bits(d, (dtd || misc) ? 1 : 0) == 0) {
      break;  // SE(*)
    }
    if (exi_read_bits(d, exi_bits_for(dtd + misc)) == 0 && dtd) {
      exi_doctype(d);
    } else {
      result = exi_misc(d);
    }
  }
  if (!d->error && result == SXMLExplorerContinue) {
    result = exi_start_element(d, exi_decode_qname(d));
  }

  while (!d->error && result == SXMLExplorerContinue && d->depth > 0) {
    result = exi_element_event(d);
  }

  // DocEnd: ED
  while (!d->error && result == SXMLExplorerContinue) {
    if (exi_read_bits(d, misc ? 1 : 0) == 0) {
      break;  // ED
    }
    result = exi_misc(d);
  }

  if (d->error) {
    return d->error;
  }
  if (result == SXMLExplorerContinue) {
    return SXMLExplorerComplete;
  }
  if (result == SXMLExplorerStop) {
    return SXMLExplorerInterrupted;
  }
  return result;
}

unsigned char sxml_run_explorer_exi(SXMLExplorer* explorer, unsigned char* exi, unsigned int len) {
  ExiDecoder decoder;
  unsigned char result;

  if (explorer == NULL || exi == NULL || len == 0) {
    return SXMLExplorerErrorMalformedXML;
  }

  memset(&decoder, 0, sizeof(decoder));
  decoder.explorer = explorer;
  decoder.data = exi;
  decoder.len = len;
  decoder.options = explorer->exi_options;

  if (explorer->exi_workspace != NULL) {
    decoder.arena = explorer->exi_workspace;
    decoder.arena_size = explorer->exi_workspace_size;
  } else {
    decoder.arena = malloc(SXML_EXI_WORKSPACE_SIZE);
    if (decoder.arena == NULL) {
      return SXMLExplorerErrorBufferOverflow;
    }
    decoder.arena_size = SXML_EXI_WORKSPACE_SIZE;
  }

  result = exi_decode(&decoder);

  if (explorer->exi_workspace == NULL) {
    free(decoder.arena);
  }
  explorer->bp = 0;
  explorer->buffer[0] = '\0';
  return result;
}

#endif // SXML_NO_EXI
