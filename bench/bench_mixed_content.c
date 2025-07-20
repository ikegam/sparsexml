#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include "sparsexml.h"
#include "tinyxml_stub.h"

static const char mixed_xml[] = 
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<document xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
    "  <metadata created=\"2023-01-01\" version=\"1.0\">\n"
    "    <title>Sample Document</title>\n"
    "    <author email=\"test@example.com\">John Doe</author>\n"
    "    <keywords>xml,parsing,benchmark,test</keywords>\n"
    "  </metadata>\n"
    "  <content>\n"
    "    <section id=\"intro\" importance=\"high\">\n"
    "      <heading>Introduction</heading>\n"
    "      <paragraph>This is a sample document with mixed content including text, attributes, and various element types.</paragraph>\n"
    "      <list type=\"ordered\">\n"
    "        <item priority=\"1\">First item with <emphasis>emphasis</emphasis></item>\n"
    "        <item priority=\"2\">Second item with <link href=\"http://example.com\">external link</link></item>\n"
    "        <item priority=\"3\">Third item with <code>inline code</code></item>\n"
    "      </list>\n"
    "    </section>\n"
    "    <section id=\"data\" importance=\"medium\">\n"
    "      <heading>Data Section</heading>\n"
    "      <table border=\"1\" cellpadding=\"2\">\n"
    "        <header>\n"
    "          <cell type=\"header\">Name</cell>\n"
    "          <cell type=\"header\">Value</cell>\n"
    "          <cell type=\"header\">Description</cell>\n"
    "        </header>\n"
    "        <row id=\"row1\">\n"
    "          <cell type=\"data\">Alpha</cell>\n"
    "          <cell type=\"data\">123.45</cell>\n"
    "          <cell type=\"data\">First measurement</cell>\n"
    "        </row>\n"
    "        <row id=\"row2\">\n"
    "          <cell type=\"data\">Beta</cell>\n"
    "          <cell type=\"data\">678.90</cell>\n"
    "          <cell type=\"data\">Second measurement</cell>\n"
    "        </row>\n"
    "      </table>\n"
    "    </section>\n"
    "  </content>\n"
    "  <footer>\n"
    "    <copyright year=\"2023\">© Test Corporation</copyright>\n"
    "    <contact>\n"
    "      <email verified=\"true\">contact@example.com</email>\n"
    "      <phone country=\"US\" type=\"business\">+1-555-0123</phone>\n"
    "    </contact>\n"
    "  </footer>\n"
    "</document>";

static unsigned char dummy_tag_cb(char* tag){ (void)tag; return SXMLExplorerContinue; }
static unsigned char dummy_content_cb(char* content){ (void)content; return SXMLExplorerContinue; }
static unsigned char dummy_attr_key_cb(char* key){ (void)key; return SXMLExplorerContinue; }
static unsigned char dummy_attr_value_cb(char* val){ (void)val; return SXMLExplorerContinue; }

static void expat_start_mixed(void *userData, const char *name, const char **atts){
    (void)userData; (void)name; (void)atts;
}

static void expat_end_mixed(void *userData, const char *name){
    (void)userData; (void)name;
}

static void expat_char_mixed(void *userData, const char *s, int len){
    (void)userData; (void)s; (void)len;
}

void bench_sparsexml_mixed(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0; i<iterations; i++){
        SXMLExplorer* ex = sxml_make_explorer();
        sxml_register_func(ex, dummy_tag_cb, dummy_content_cb, dummy_attr_key_cb, dummy_attr_value_cb);
        size_t used = malloc_usable_size(ex);
        if(used > maximum) maximum = used;
        total += used;
        sxml_run_explorer(ex, (char*)mixed_xml);
        sxml_destroy_explorer(ex);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

void bench_expat_mixed(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0; i<iterations; i++){
        XML_Parser p = XML_ParserCreate(NULL);
        XML_SetElementHandler(p, expat_start_mixed, expat_end_mixed);
        XML_SetCharacterDataHandler(p, expat_char_mixed);
        size_t used = malloc_usable_size(p);
        if(used > maximum) maximum = used;
        total += used;
        XML_Parse(p, mixed_xml, strlen(mixed_xml), XML_TRUE);
        XML_ParserFree(p);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

void bench_tinyxml_mixed(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0; i<iterations; i++){
        TinyXMLDoc *doc = tinyxml_load_string(mixed_xml);
        size_t used = tinyxml_mem_usage(doc);
        if(used > maximum) maximum = used;
        total += used;
        tinyxml_free(doc);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

int bench_mixed_main(int argc, char **argv){
    int iter = 50000;
    if(argc > 1){
        iter = atoi(argv[1]);
        if(iter <= 0) iter = 50000;
    }

    clock_t start, end;
    size_t sparse_avg = 0, sparse_max = 0;
    start = clock();
    bench_sparsexml_mixed(iter, &sparse_avg, &sparse_max);
    end = clock();
    double sparse_time = (double)(end - start) / CLOCKS_PER_SEC;

    size_t expat_avg = 0, expat_max = 0;
    start = clock();
    bench_expat_mixed(iter, &expat_avg, &expat_max);
    end = clock();
    double expat_time = (double)(end - start) / CLOCKS_PER_SEC;

    size_t tiny_avg = 0, tiny_max = 0;
    start = clock();
    bench_tinyxml_mixed(iter, &tiny_avg, &tiny_max);
    end = clock();
    double tiny_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("%-12s | %8d | %18zu | %14zu | %14zu | %16.6f | %16.6f | %16.6f\n",
           "mixed", iter, sparse_avg, expat_avg, tiny_avg,
           sparse_time, expat_time, tiny_time);
    return 0;
}