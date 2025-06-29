#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include "sparsexml.h"

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

void bench_sparsexml(int iterations){
    SXMLExplorer* ex = sxml_make_explorer();
    sxml_register_func(ex, tag_cb, content_cb, attr_key_cb, attr_value_cb);
    for(int i=0;i<iterations;i++){
        sxml_run_explorer(ex,(char*)xml);
    }
    sxml_destroy_explorer(ex);
}

void bench_expat(int iterations){
    for(int i=0;i<iterations;i++){
        XML_Parser p = XML_ParserCreate(NULL);
        XML_Parse(p, xml, strlen(xml), XML_TRUE);
        XML_ParserFree(p);
    }
}

int main(int argc, char **argv){
    int iter = 100000;
    if(argc > 1)
        iter = atoi(argv[1]);

    run_test();
    run_expat_test();

    clock_t start, end;
    start = clock();
    bench_sparsexml(iter);
    end = clock();
    double sparse_time = (double)(end - start) / CLOCKS_PER_SEC;

    start = clock();
    bench_expat(iter);
    end = clock();
    double expat_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("SparseXML: %f seconds for %d iterations\n", sparse_time, iter);
    printf("Expat:     %f seconds for %d iterations\n", expat_time, iter);
    return 0;
}
