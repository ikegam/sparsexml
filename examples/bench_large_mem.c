#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <malloc.h>
#include "sparsexml.h"

static unsigned char tag_cb(char* t){ return SXMLExplorerContinue; }
static unsigned char content_cb(char* c){ return SXMLExplorerContinue; }
static unsigned char key_cb(char* k){ return SXMLExplorerContinue; }
static unsigned char val_cb(char* v){ return SXMLExplorerContinue; }

static void build_xml(char** out, int repeat){
    const char frag[] = "<child attr=\"v\">text</child>";
    size_t frag_len = strlen(frag);
    size_t size = repeat * frag_len + strlen("<root></root>") + 1;
    char *buf = malloc(size);
    strcpy(buf, "<root>");
    for(int i=0;i<repeat;i++){
        strcat(buf, frag);
    }
    strcat(buf, "</root>");
    *out = buf;
}

static size_t mem_usage_sparsexml(char* xml){
    struct mallinfo2 mi_before = mallinfo2();
    SXMLExplorer *ex = sxml_make_explorer();
    sxml_register_func(ex, tag_cb, content_cb, key_cb, val_cb);
    sxml_run_explorer(ex, xml);
    struct mallinfo2 mi_after = mallinfo2();
    size_t used = mi_after.uordblks - mi_before.uordblks;
    sxml_destroy_explorer(ex);
    return used;
}

static void start(void *ud,const char *name,const char **atts){ (void)ud; }
static void end(void *ud,const char *name){ (void)ud; }
static void ch(void *ud,const char *s,int len){ (void)ud; }

static size_t mem_usage_expat(char* xml){
    struct mallinfo2 mi_before = mallinfo2();
    XML_Parser p = XML_ParserCreate(NULL);
    XML_SetElementHandler(p, start, end);
    XML_SetCharacterDataHandler(p, ch);
    XML_Parse(p, xml, strlen(xml), XML_TRUE);
    struct mallinfo2 mi_after = mallinfo2();
    size_t used = mi_after.uordblks - mi_before.uordblks;
    XML_ParserFree(p);
    return used;
}

int main(int argc,char** argv){
    int repeat = 1000;
    if(argc>1) repeat = atoi(argv[1]);
    char* xml = NULL;
    build_xml(&xml, repeat);
    size_t sxml = mem_usage_sparsexml(xml);
    size_t expat = mem_usage_expat(xml);
    printf("Large XML with %d children\n", repeat);
    printf("SparseXML memory: %zu bytes\n", sxml);
    printf("Expat memory: %zu bytes\n", expat);
    free(xml);
    return 0;
}
