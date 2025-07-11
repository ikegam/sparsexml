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

static const char xml[] = "<?xml version=\"1.0\"?><root attr=\"value\">text<child>child</child></root>";

static unsigned int tag_count;
static unsigned int content_count;
static unsigned int attr_count;

static unsigned int expat_tag_count;
static unsigned int expat_content_count;
static unsigned int expat_attr_count;


static void reset_counters(void){
    tag_count = 0;
    content_count = 0;
    attr_count = 0;
    expat_tag_count = 0;
    expat_content_count = 0;
    expat_attr_count = 0;
}


static unsigned char tag_cb(char* tag){return SXMLExplorerContinue;}
static unsigned char content_cb(char* content){return SXMLExplorerContinue;}
static unsigned char attr_key_cb(char* key){return SXMLExplorerContinue;}
static unsigned char attr_value_cb(char* val){return SXMLExplorerContinue;}

static void expat_start(void *userData, const char *name, const char **atts){
    (void)userData;
    expat_tag_count++;
    if(atts){
        for(const char **a = atts; *a; a+=2){
            expat_attr_count++;
        }
    }
}

static void expat_end(void *userData, const char *name){
    (void)userData;
    (void)name;
    expat_tag_count++;
}

static void expat_char(void *userData, const char *s, int len){
    (void)userData;
    for(int i=0;i<len;i++){
        if(!isspace((unsigned char)s[i])){
            expat_content_count++;
            break;
        }
    }
}

static unsigned char count_tag(char *name){
    tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char count_content(char *c){
    if(strlen(c) > 0)
        content_count++;
    return SXMLExplorerContinue;
}

static unsigned char count_attr_key(char *k){
    attr_count++;
    return SXMLExplorerContinue;
}

static unsigned char count_attr_val(char *v){
    (void)v;
    return SXMLExplorerContinue;
}

static void run_test(void){
    reset_counters();
    SXMLExplorer *ex = sxml_make_explorer();
    sxml_register_func(ex, count_tag, count_content, count_attr_key, count_attr_val);
    unsigned char ret = sxml_run_explorer(ex, (char *)xml);
    sxml_destroy_explorer(ex);
    assert(ret == SXMLExplorerComplete);
    assert(tag_count == 4);
    assert(content_count == 2);
    assert(attr_count == 1);
    printf("[TEST] counters ok (tags=%u, contents=%u, attrs=%u)\n", tag_count, content_count, attr_count);
}

static void run_expat_test(void){
    reset_counters();
    XML_Parser p = XML_ParserCreate(NULL);
    XML_SetElementHandler(p, expat_start, expat_end);
    XML_SetCharacterDataHandler(p, expat_char);
    int ret = XML_Parse(p, xml, strlen(xml), XML_TRUE);
    XML_ParserFree(p);
    assert(ret != XML_STATUS_ERROR);
    assert(expat_tag_count == 4);
    assert(expat_content_count == 2);
    assert(expat_attr_count == 1);
    printf("[TEST] expat counters ok (tags=%u, contents=%u, attrs=%u)\n", expat_tag_count, expat_content_count, expat_attr_count);
}

void bench_sparsexml(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0;i<iterations;i++){
        SXMLExplorer* ex = sxml_make_explorer();
        sxml_register_func(ex, tag_cb, content_cb, attr_key_cb, attr_value_cb);
        size_t used = malloc_usable_size(ex);
        if(used > maximum)
            maximum = used;
        total += used;
        sxml_run_explorer(ex,(char*)xml);
        sxml_destroy_explorer(ex);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

void bench_expat(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0;i<iterations;i++){
        XML_Parser p = XML_ParserCreate(NULL);
        size_t used = malloc_usable_size(p);
        if(used > maximum)
            maximum = used;
        total += used;
        XML_Parse(p, xml, strlen(xml), XML_TRUE);
        XML_ParserFree(p);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

void bench_tinyxml(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0;i<iterations;i++){
        TinyXMLDoc *doc = tinyxml_load_string(xml);
        size_t used = tinyxml_mem_usage(doc);
        if(used > maximum)
            maximum = used;
        total += used;
        tinyxml_free(doc);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

#ifdef BENCH_LIBRARY
int bench_basic_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    int iter = 100000;
    if(argc > 1){
        iter = atoi(argv[1]);
        if(iter <= 0)
            iter = 100000;
    }

    run_test();
    run_expat_test();

    /* Print table header once with aligned columns */
    printf("%-12s | %-8s | %-18s | %-14s | %-14s | %-16s | %-16s | %-16s\n",
           "Benchmark", "Param", "SparseXML(bytes)",
           "Expat(bytes)", "TinyXML(bytes)",
           "Sparse time(s)", "Expat time(s)", "Tiny time(s)");

    clock_t start, end;
    size_t sparse_avg = 0, sparse_max = 0;
    start = clock();
    bench_sparsexml(iter, &sparse_avg, &sparse_max);
    end = clock();
    double sparse_time = (double)(end - start) / CLOCKS_PER_SEC;

    size_t expat_avg = 0, expat_max = 0;
    start = clock();
    bench_expat(iter, &expat_avg, &expat_max);
    end = clock();
    double expat_time = (double)(end - start) / CLOCKS_PER_SEC;

    size_t tiny_avg = 0, tiny_max = 0;
    start = clock();
    bench_tinyxml(iter, &tiny_avg, &tiny_max);
    end = clock();
    double tiny_time = (double)(end - start) / CLOCKS_PER_SEC;

    /* Print as a single table row matching the header format */
    printf("%-12s | %8d | %18zu | %14zu | %14zu | %16.6f | %16.6f | %16.6f\n",
           "basic", iter, sparse_avg, expat_avg, tiny_avg,
           sparse_time, expat_time, tiny_time);
    (void)sparse_max; /* keep variables unused in case future metrics needed */
    (void)expat_max;
    (void)tiny_max;
    return 0;
}
