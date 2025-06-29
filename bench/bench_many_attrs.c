#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <malloc.h>
#include <time.h>
#include "sparsexml.h"
#include "tinyxml_stub.h"

static unsigned char tag_cb(char* t){ return SXMLExplorerContinue; }
static unsigned char content_cb(char* c){ return SXMLExplorerContinue; }
static unsigned char key_cb(char* k){ return SXMLExplorerContinue; }
static unsigned char val_cb(char* v){ return SXMLExplorerContinue; }

static void build_xml(char** out, int count){
    size_t size = strlen("<root />") + count * 16 + 1;
    char *buf = malloc(size);
    strcpy(buf, "<root");
    for(int i=0;i<count;i++){
        char tmp[16];
        sprintf(tmp, " a%d=\"v\"", i);
        strcat(buf, tmp);
    }
    strcat(buf, "/>");
    *out = buf;
}

static size_t mem_usage_sparsexml(char* xml, double* t){
    struct mallinfo2 mi_before = mallinfo2();
    clock_t t_start = clock();
    SXMLExplorer *ex = sxml_make_explorer();
    sxml_register_func(ex, tag_cb, content_cb, key_cb, val_cb);
    sxml_run_explorer(ex, xml);
    clock_t t_end = clock();
    struct mallinfo2 mi_after = mallinfo2();
    size_t used = mi_after.uordblks - mi_before.uordblks;
    sxml_destroy_explorer(ex);
    if(t) *t = (double)(t_end - t_start) / CLOCKS_PER_SEC;
    return used;
}

static void start(void *ud,const char *name,const char **atts){ (void)ud; (void)name; (void)atts; }
static void end(void *ud,const char *name){ (void)ud; (void)name; }
static void ch(void *ud,const char *s,int len){ (void)ud; (void)s; (void)len; }

static size_t mem_usage_expat(char* xml, double* t){
    struct mallinfo2 mi_before = mallinfo2();
    XML_Parser p = XML_ParserCreate(NULL);
    clock_t t_start = clock();
    XML_SetElementHandler(p, start, end);
    XML_SetCharacterDataHandler(p, ch);
    XML_Parse(p, xml, strlen(xml), XML_TRUE);
    clock_t t_end = clock();
    struct mallinfo2 mi_after = mallinfo2();
    size_t used = mi_after.uordblks - mi_before.uordblks;
    XML_ParserFree(p);
    if(t) *t = (double)(t_end - t_start) / CLOCKS_PER_SEC;
    return used;
}

static size_t mem_usage_tinyxml(char* xml, double* t){
    clock_t t_start = clock();
    TinyXMLDoc *doc = tinyxml_load_string(xml);
    clock_t t_end = clock();
    size_t used = tinyxml_mem_usage(doc);
    tinyxml_free(doc);
    if(t) *t = (double)(t_end - t_start) / CLOCKS_PER_SEC;
    return used;
}

#ifdef BENCH_LIBRARY
int bench_attrs_main(int argc,char **argv)
#else
int main(int argc,char **argv)
#endif
{
    int count = 50;
    if(argc>1) count = atoi(argv[1]);
    char* xml = NULL;
    build_xml(&xml, count);
    double s_time = 0.0, e_time = 0.0, t_time = 0.0;
    size_t sxml = mem_usage_sparsexml(xml, &s_time);
    size_t expat = mem_usage_expat(xml, &e_time);
    size_t tiny = mem_usage_tinyxml(xml, &t_time);
    printf("%-12s | %8d | %18zu | %14zu | %14zu | %16.6f | %16.6f | %16.6f\n",
           "many_attrs", count, sxml, expat, tiny, s_time, e_time, t_time);
    free(xml);
    return 0;
}
