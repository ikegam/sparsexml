#ifndef TINYXML_STUB_H
#define TINYXML_STUB_H

#include <stddef.h>

typedef struct {
    char *data;
} TinyXMLDoc;

TinyXMLDoc* tinyxml_load_string(const char *xml);
size_t tinyxml_mem_usage(TinyXMLDoc *doc);
void tinyxml_free(TinyXMLDoc *doc);

#endif /* TINYXML_STUB_H */
