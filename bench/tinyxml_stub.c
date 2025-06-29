#include "tinyxml_stub.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

TinyXMLDoc* tinyxml_load_string(const char *xml){
    TinyXMLDoc *doc = malloc(sizeof(TinyXMLDoc));
    if(!doc) return NULL;
    size_t len = strlen(xml) + 1;
    doc->data = malloc(len);
    if(doc->data)
        memcpy(doc->data, xml, len);
    return doc;
}

size_t tinyxml_mem_usage(TinyXMLDoc *doc){
    if(!doc) return 0;
    size_t total = malloc_usable_size(doc);
    if(doc->data)
        total += malloc_usable_size(doc->data);
    return total;
}

void tinyxml_free(TinyXMLDoc *doc){
    if(!doc) return;
    free(doc->data);
    free(doc);
}
