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

static char* generate_stress_xml(int depth, int width, int attr_count) {
    // Calculate buffer size needed
    size_t buffer_size = 100000; // Start with reasonable buffer
    char *xml = malloc(buffer_size);
    if (!xml) return NULL;
    
    strcpy(xml, "<?xml version=\"1.0\"?><root>");
    
    char temp[1000];
    for (int d = 0; d < depth; d++) {
        for (int w = 0; w < width; w++) {
            // Add opening tag with attributes
            snprintf(temp, sizeof(temp), "<elem%d_%d", d, w);
            strcat(xml, temp);
            
            for (int a = 0; a < attr_count; a++) {
                snprintf(temp, sizeof(temp), " attr%d=\"value%d_%d_%d\"", a, d, w, a);
                strcat(xml, temp);
            }
            strcat(xml, ">");
            
            // Add some content
            snprintf(temp, sizeof(temp), "Content for element %d-%d with some text data", d, w);
            strcat(xml, temp);
        }
    }
    
    // Close all elements
    for (int d = depth - 1; d >= 0; d--) {
        for (int w = width - 1; w >= 0; w--) {
            snprintf(temp, sizeof(temp), "</elem%d_%d>", d, w);
            strcat(xml, temp);
        }
    }
    
    strcat(xml, "</root>");
    return xml;
}

static char* small_stress_xml = NULL;
static char* medium_stress_xml = NULL;
static char* large_stress_xml = NULL;

static unsigned char stress_tag_cb(char* tag){ (void)tag; return SXMLExplorerContinue; }
static unsigned char stress_content_cb(char* content){ (void)content; return SXMLExplorerContinue; }
static unsigned char stress_attr_key_cb(char* key){ (void)key; return SXMLExplorerContinue; }
static unsigned char stress_attr_value_cb(char* val){ (void)val; return SXMLExplorerContinue; }

static void expat_start_stress(void *userData, const char *name, const char **atts){
    (void)userData; (void)name; (void)atts;
}

static void expat_end_stress(void *userData, const char *name){
    (void)userData; (void)name;
}

static void expat_char_stress(void *userData, const char *s, int len){
    (void)userData; (void)s; (void)len;
}

void bench_stress_size(const char* xml, const char* size_name, int iterations) {
    if (!xml) {
        printf("%-12s | %8d | %18s | %14s | %14s | %16s | %16s | %16s\n",
               size_name, iterations, "ERROR", "ERROR", "ERROR", "ERROR", "ERROR", "ERROR");
        return;
    }

    clock_t start, end;
    size_t sparse_avg = 0;
    size_t total = 0, maximum = 0;
    
    // SparseXML benchmark
    start = clock();
    for(int i = 0; i < iterations; i++){
        SXMLExplorer* ex = sxml_make_explorer();
        sxml_register_func(ex, stress_tag_cb, stress_content_cb, stress_attr_key_cb, stress_attr_value_cb);
        size_t used = malloc_usable_size(ex);
        if(used > maximum) maximum = used;
        total += used;
        sxml_run_explorer(ex, (char*)xml);
        sxml_destroy_explorer(ex);
    }
    end = clock();
    sparse_avg = total / iterations;
    double sparse_time = (double)(end - start) / CLOCKS_PER_SEC;

    // Expat benchmark
    total = 0; maximum = 0;
    start = clock();
    for(int i = 0; i < iterations; i++){
        XML_Parser p = XML_ParserCreate(NULL);
        XML_SetElementHandler(p, expat_start_stress, expat_end_stress);
        XML_SetCharacterDataHandler(p, expat_char_stress);
        size_t used = malloc_usable_size(p);
        if(used > maximum) maximum = used;
        total += used;
        XML_Parse(p, xml, strlen(xml), XML_TRUE);
        XML_ParserFree(p);
    }
    end = clock();
    size_t expat_avg = total / iterations;
    double expat_time = (double)(end - start) / CLOCKS_PER_SEC;

    // TinyXML benchmark
    total = 0; maximum = 0;
    start = clock();
    for(int i = 0; i < iterations; i++){
        TinyXMLDoc *doc = tinyxml_load_string(xml);
        size_t used = tinyxml_mem_usage(doc);
        if(used > maximum) maximum = used;
        total += used;
        tinyxml_free(doc);
    }
    end = clock();
    size_t tiny_avg = total / iterations;
    double tiny_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("%-12s | %8d | %18zu | %14zu | %14zu | %16.6f | %16.6f | %16.6f\n",
           size_name, iterations, sparse_avg, expat_avg, tiny_avg,
           sparse_time, expat_time, tiny_time);
}

int bench_stress_main(int argc, char **argv){
    int base_iter = 10000;
    if(argc > 1){
        base_iter = atoi(argv[1]);
        if(base_iter <= 0) base_iter = 10000;
    }

    // Generate test XMLs
    small_stress_xml = generate_stress_xml(3, 5, 2);   // Small: 3 levels, 5 elements per level, 2 attrs each
    medium_stress_xml = generate_stress_xml(5, 8, 3);  // Medium: 5 levels, 8 elements per level, 3 attrs each  
    large_stress_xml = generate_stress_xml(7, 10, 4);  // Large: 7 levels, 10 elements per level, 4 attrs each

    // Run benchmarks with different iteration counts based on complexity
    bench_stress_size(small_stress_xml, "stress_sm", base_iter);
    bench_stress_size(medium_stress_xml, "stress_md", base_iter / 2);
    bench_stress_size(large_stress_xml, "stress_lg", base_iter / 5);

    // Cleanup
    if(small_stress_xml) free(small_stress_xml);
    if(medium_stress_xml) free(medium_stress_xml);
    if(large_stress_xml) free(large_stress_xml);

    return 0;
}