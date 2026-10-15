#ifndef __SXMLExplorer__
#define __SXMLExplorer__

#define SXMLExplorerContinue 0x00
#define SXMLExplorerStop 0x01

#define SXMLExplorerComplete 0x02
#define SXMLExplorerInterrupted 0x03
#define SXMLExplorerErrorInvalidEntity 0x04
#define SXMLExplorerErrorBufferOverflow 0x05
#define SXMLExplorerErrorMalformedXML 0x06
#define SXMLExplorerErrorUnsupported 0x07

// Maximum length of a single tag name, attribute, content chunk or comment
// delivered to a callback. Longer data is truncated.
#ifndef SXMLElementLength
#define SXMLElementLength 1024
#endif

// Tag name remembered to report the closing event of <empty/> elements
#ifndef SXMLNameLength
#define SXMLNameLength 64
#endif

// EXI decoding needs string tables and learned grammars. They live in a
// workspace that is either supplied with sxml_set_exi_workspace() or
// allocated with malloc() for the duration of sxml_run_explorer_exi().
#ifndef SXML_EXI_WORKSPACE_SIZE
#define SXML_EXI_WORKSPACE_SIZE 32768
#endif

#ifndef SXML_EXI_MAX_DEPTH
#define SXML_EXI_MAX_DEPTH 32
#endif

// EXI options that are not carried in the EXI header (out-of-band options)
#define SXML_EXI_PRESERVE_COMMENTS 0x01
#define SXML_EXI_PRESERVE_PIS 0x02
#define SXML_EXI_PRESERVE_DTD 0x04
#define SXML_EXI_PRESERVE_PREFIXES 0x08
#define SXML_EXI_PRESERVE_LEXICAL_VALUES 0x10
#define SXML_EXI_BYTE_ALIGNED 0x20

typedef enum __SXMLExplorerState {
  INITIAL,
  IN_DECLARATION,
  IN_TAG,
  IN_ATTRIBUTE_KEY,
  IN_ATTRIBUTE_VALUE,
  IN_CONTENT,
  IN_COMMENT,
  IN_CDATA,
  IN_ENTITY,
  IN_DOCTYPE,
  IN_ATTRIBUTE_EQUAL
} SXMLExplorerState;

typedef struct __SXMLExplorer SXMLExplorer;

// An explorer parses either XML or EXI, chosen when it is created.
// Both kinds share the registration and option functions below.
SXMLExplorer* sxml_make_explorer(void);       // XML
SXMLExplorer* sxml_make_exi_explorer(void);   // EXI (NULL with SXML_NO_EXI)
void sxml_destroy_explorer(SXMLExplorer*);
// Heap-free alternatives: initialise an explorer in caller-provided memory
// of at least sxml_explorer_size() / sxml_exi_explorer_size() bytes,
// aligned for pointers (e.g. declared as void* memory[N]).
unsigned int sxml_explorer_size(void);
SXMLExplorer* sxml_init_explorer(void* memory, unsigned int size);
unsigned int sxml_exi_explorer_size(void);
SXMLExplorer* sxml_init_exi_explorer(void* memory, unsigned int size);
// Reset parsing state so the explorer can parse a new document.
// Callbacks and options are kept.
void sxml_reset_explorer(SXMLExplorer*);

// Callbacks receive the explorer that reports the event and the text, which
// is stored in the explorer and valid until the callback returns. They reach
// their own state through the user data instead of globals, and return
// SXMLExplorerContinue or SXMLExplorerStop.
typedef unsigned char (*SXMLCallback)(SXMLExplorer* explorer, char* text);

// Callbacks may be NULL
void sxml_register_func(SXMLExplorer*, SXMLCallback tag, SXMLCallback content,
                        SXMLCallback attribute_key, SXMLCallback attribute_value);
void sxml_register_comment_func(SXMLExplorer*, SXMLCallback comment);
void sxml_set_user_data(SXMLExplorer*, void*);
void* sxml_get_user_data(SXMLExplorer*);
void sxml_enable_entity_processing(SXMLExplorer*, unsigned char);
void sxml_enable_namespace_processing(SXMLExplorer*, unsigned char);
void sxml_enable_extended_entities(SXMLExplorer*, unsigned char);
void sxml_enable_numeric_entities(SXMLExplorer*, unsigned char);

// EXI explorers only
void sxml_set_exi_options(SXMLExplorer*, unsigned char);
void sxml_set_exi_workspace(SXMLExplorer*, void*, unsigned int);

// Return SXMLExplorerErrorUnsupported when called with the other kind of explorer
unsigned char sxml_run_explorer(SXMLExplorer*, char*);
unsigned char sxml_run_explorer_exi(SXMLExplorer*, unsigned char*, unsigned int);

#endif
