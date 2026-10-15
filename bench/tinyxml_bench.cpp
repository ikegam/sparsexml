#include <malloc.h>
#include <stdio.h>
#include <tinyxml2.h>

#include "tinyxml_bench.h"

struct TinyXMLDoc {
    tinyxml2::XMLDocument dom;
    size_t mem;
};

TinyXMLDoc* tinyxml_load_string(const char *xml){
    size_t before = mallinfo2().uordblks;
    TinyXMLDoc *doc = new TinyXMLDoc;
    if(doc->dom.Parse(xml) != tinyxml2::XML_SUCCESS){
        fprintf(stderr, "tinyxml2: %s\n", doc->dom.ErrorStr());
        delete doc;
        return NULL;
    }
    doc->mem = mallinfo2().uordblks - before;
    return doc;
}

size_t tinyxml_mem_usage(TinyXMLDoc *doc){
    return doc ? doc->mem : 0;
}

static unsigned int count_elements(const tinyxml2::XMLElement *e){
    unsigned int count = 0;
    for(; e; e = e->NextSiblingElement()){
        count += 1 + count_elements(e->FirstChildElement());
    }
    return count;
}

unsigned int tinyxml_element_count(TinyXMLDoc *doc){
    return doc ? count_elements(doc->dom.FirstChildElement()) : 0;
}

void tinyxml_free(TinyXMLDoc *doc){
    delete doc;
}
