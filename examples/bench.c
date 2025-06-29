#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <time.h>
#include "sparsexml.h"

static const char xml[] = "<?xml version=\"1.0\"?><root attr=\"value\">text<child>child</child></root>";

static unsigned char tag_cb(char* tag){return SXMLExplorerContinue;}
static unsigned char content_cb(char* content){return SXMLExplorerContinue;}
static unsigned char attr_key_cb(char* key){return SXMLExplorerContinue;}
static unsigned char attr_value_cb(char* val){return SXMLExplorerContinue;}

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

int main(){
    int iter=100000;
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
