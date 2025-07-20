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

static const char unicode_xml[] = 
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<multilang>\n"
    "  <english lang=\"en\">Hello World! This is a test document.</english>\n"
    "  <japanese lang=\"ja\">こんにちは世界！これはテストドキュメントです。</japanese>\n"
    "  <chinese lang=\"zh\">你好世界！这是一个测试文档。</chinese>\n"
    "  <korean lang=\"ko\">안녕하세요 세계! 이것은 테스트 문서입니다.</korean>\n"
    "  <russian lang=\"ru\">Привет мир! Это тестовый документ.</russian>\n"
    "  <arabic lang=\"ar\">مرحبا بالعالم! هذا مستند اختبار.</arabic>\n"
    "  <german lang=\"de\">Hallo Welt! Das ist ein Testdokument.</german>\n"
    "  <french lang=\"fr\">Bonjour le monde! Ceci est un document de test.</french>\n"
    "  <spanish lang=\"es\">¡Hola Mundo! Este es un documento de prueba.</spanish>\n"
    "  <symbols>\n"
    "    <math>∑∫∂∆∇∞±√∝∈∉⊂⊃∪∩</math>\n"
    "    <currency>$€£¥₹₿¢₨₩₦</currency>\n"
    "    <arrows>←→↑↓↔↕⇒⇐⇔⇕</arrows>\n"
    "    <misc>★☆♠♥♦♣✓✗©®™§¶</misc>\n"
    "  </symbols>\n"
    "  <mixed>\n"
    "    <text>English text with 日本語 and العربية and русский mixed together!</text>\n"
    "    <emojis>🌍🌎🌏 🚀💻📱 🎵🎨🎭 🍕🍜🥘</emojis>\n"
    "    <complex attr1=\"値１\" attr2=\"значение2\" attr3=\"قيمة3\">複雑な内容 with עברית and हिन्दी text</complex>\n"
    "  </mixed>\n"
    "  <special_chars>\n"
    "    <quotes>\"'`''""«»‹›</quotes>\n"
    "    <brackets>()[]{}⟨⟩⟪⟫</brackets>\n"
    "    <dashes>-–—―</dashes>\n"
    "  </special_chars>\n"
    "</multilang>";

static unsigned char unicode_tag_cb(char* tag){ (void)tag; return SXMLExplorerContinue; }
static unsigned char unicode_content_cb(char* content){ (void)content; return SXMLExplorerContinue; }
static unsigned char unicode_attr_key_cb(char* key){ (void)key; return SXMLExplorerContinue; }
static unsigned char unicode_attr_value_cb(char* val){ (void)val; return SXMLExplorerContinue; }

static void expat_start_unicode(void *userData, const char *name, const char **atts){
    (void)userData; (void)name; (void)atts;
}

static void expat_end_unicode(void *userData, const char *name){
    (void)userData; (void)name;
}

static void expat_char_unicode(void *userData, const char *s, int len){
    (void)userData; (void)s; (void)len;
}

void bench_sparsexml_unicode(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0; i<iterations; i++){
        SXMLExplorer* ex = sxml_make_explorer();
        sxml_register_func(ex, unicode_tag_cb, unicode_content_cb, unicode_attr_key_cb, unicode_attr_value_cb);
        size_t used = malloc_usable_size(ex);
        if(used > maximum) maximum = used;
        total += used;
        sxml_run_explorer(ex, (char*)unicode_xml);
        sxml_destroy_explorer(ex);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

void bench_expat_unicode(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0; i<iterations; i++){
        XML_Parser p = XML_ParserCreate(NULL);
        XML_SetElementHandler(p, expat_start_unicode, expat_end_unicode);
        XML_SetCharacterDataHandler(p, expat_char_unicode);
        size_t used = malloc_usable_size(p);
        if(used > maximum) maximum = used;
        total += used;
        XML_Parse(p, unicode_xml, strlen(unicode_xml), XML_TRUE);
        XML_ParserFree(p);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

void bench_tinyxml_unicode(int iterations, size_t *avg_mem, size_t *max_mem){
    size_t total = 0;
    size_t maximum = 0;
    for(int i=0; i<iterations; i++){
        TinyXMLDoc *doc = tinyxml_load_string(unicode_xml);
        size_t used = tinyxml_mem_usage(doc);
        if(used > maximum) maximum = used;
        total += used;
        tinyxml_free(doc);
    }
    if(avg_mem) *avg_mem = total / iterations;
    if(max_mem) *max_mem = maximum;
}

int bench_unicode_main(int argc, char **argv){
    int iter = 30000;
    if(argc > 1){
        iter = atoi(argv[1]);
        if(iter <= 0) iter = 30000;
    }

    clock_t start, end;
    size_t sparse_avg = 0, sparse_max = 0;
    start = clock();
    bench_sparsexml_unicode(iter, &sparse_avg, &sparse_max);
    end = clock();
    double sparse_time = (double)(end - start) / CLOCKS_PER_SEC;

    size_t expat_avg = 0, expat_max = 0;
    start = clock();
    bench_expat_unicode(iter, &expat_avg, &expat_max);
    end = clock();
    double expat_time = (double)(end - start) / CLOCKS_PER_SEC;

    size_t tiny_avg = 0, tiny_max = 0;
    start = clock();
    bench_tinyxml_unicode(iter, &tiny_avg, &tiny_max);
    end = clock();
    double tiny_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("%-12s | %8d | %18zu | %14zu | %14zu | %16.6f | %16.6f | %16.6f\n",
           "unicode", iter, sparse_avg, expat_avg, tiny_avg,
           sparse_time, expat_time, tiny_time);
    return 0;
}