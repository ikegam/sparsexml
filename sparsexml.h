#ifndef __SXMLExplorer__
#define __SXMLExplorer__

#define SXMLExplorerContinue 0x00
#define SXMLExplorerStop 0x01

#define SXMLExplorerComplete 0x02
#define SXMLExplorerInterrupted 0x03
#define SXMLExplorerErrorInvalidEntity 0x04
#define SXMLExplorerErrorBufferOverflow 0x05
#define SXMLExplorerErrorMalformedXML 0x06

#define SXMLElementLength 1024

typedef enum __SXMLExplorerWhereIs {
  ON_NORMAL,
  ON_DECLARATION,
  ON_COMMENT
} SXMLExplorerWhereIs;

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
  IN_DOCTYPE
} SXMLExplorerState;

typedef struct __SXMLExplorer SXMLExplorer;

SXMLExplorer* sxml_make_explorer(void);
void sxml_destroy_explorer(SXMLExplorer*);
void sxml_register_func(SXMLExplorer*, void*, void*, void*, void*);
void sxml_register_comment_func(SXMLExplorer*, void*);
void sxml_enable_entity_processing(SXMLExplorer*, unsigned char);
void sxml_enable_namespace_processing(SXMLExplorer*, unsigned char);

unsigned char sxml_run_explorer(SXMLExplorer*, char*);

#endif
