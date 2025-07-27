#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include "sparsexml.h"

static unsigned char dummy_tag_cb(char* t){ (void)t; return SXMLExplorerContinue; }
static unsigned char dummy_content_cb(char* c){ (void)c; return SXMLExplorerContinue; }
static unsigned char dummy_attr_key_cb(char* k){ (void)k; return SXMLExplorerContinue; }
static unsigned char dummy_attr_val_cb(char* v){ (void)v; return SXMLExplorerContinue; }

static unsigned char* read_file_to_buffer(const char* filename, unsigned int* size){
    FILE* f = fopen(filename, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    unsigned char* buffer = (unsigned char*)malloc(file_size);
    if(!buffer){
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, file_size, f);
    fclose(f);
    *size = (unsigned int)file_size;
    return buffer;
}

static void bench_sparsexml_exi(unsigned char* data, unsigned int size, int iterations, size_t* avg_mem, size_t* max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0;i<iterations;i++){
        SXMLExplorer* ex = sxml_make_explorer();
        sxml_register_func(ex, dummy_tag_cb, dummy_content_cb, dummy_attr_key_cb, dummy_attr_val_cb);
        size_t used = malloc_usable_size(ex);
        if(used > maximum) maximum = used;
        total += used;
        sxml_run_explorer_exi(ex, data, size);
        sxml_destroy_explorer(ex);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

#ifdef BENCH_LIBRARY
int bench_exi_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    int iter = 100000;
    const char* file = "test-data/test-oss-1.exi";
    if(argc > 1){
        iter = atoi(argv[1]);
        if(iter <= 0) iter = 100000;
    }
    if(argc > 2){
        file = argv[2];
    }
    unsigned int size = 0;
    unsigned char* exi = read_file_to_buffer(file, &size);
    if(!exi){
        printf("Failed to open %s\n", file);
        return 1;
    }
    clock_t start, end;
    size_t sparse_avg = 0, sparse_max = 0;
    start = clock();
    bench_sparsexml_exi(exi, size, iter, &sparse_avg, &sparse_max);
    end = clock();
    double sparse_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("%-12s | %8d | %18zu | %14s | %14s | %16.6f | %16s | %16s\n",
           "exi", iter, sparse_avg, "N/A", "N/A", sparse_time, "N/A", "N/A");
    free(exi);
    (void)sparse_max;
    return 0;
}
