#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sparsexml.h"

static unsigned char feed_chunks(SXMLExplorer* ex, const char* xml, size_t chunk_size) {
    size_t len = strlen(xml);
    size_t pos = 0;
    unsigned char ret = SXMLExplorerContinue;
    while (pos < len) {
        size_t csize = chunk_size;
        if (pos + csize > len) csize = len - pos;
        char* buf = (char*)malloc(csize + 1);
        memcpy(buf, xml + pos, csize);
        buf[csize] = '\0';
        ret = sxml_run_explorer(ex, buf);
        free(buf);
        pos += csize;
        if (ret == SXMLExplorerInterrupted ||
            ret == SXMLExplorerErrorInvalidEntity ||
            ret == SXMLExplorerErrorBufferOverflow ||
            ret == SXMLExplorerErrorMalformedXML) {
            break;
        }
    }
    return ret;
}

static unsigned int simple_tag_counter = 0;
static unsigned char chunked_simple_on_tag(char* name) {
    if (strcmp("tag", name) == 0 && simple_tag_counter == 0) {
        simple_tag_counter++;
    } else if (strcmp("/tag", name) == 0 && simple_tag_counter == 1) {
        simple_tag_counter++;
    }
    if (simple_tag_counter == 2) return SXMLExplorerStop;
    return SXMLExplorerContinue;
}

void test_chunked_simple_xml(void) {
    SXMLExplorer* ex = sxml_make_explorer();
    const char xml[] = "<?xml version=\"1.1\"?><tag></tag>";
    simple_tag_counter = 0;
    sxml_register_func(ex, chunked_simple_on_tag, NULL, NULL, NULL);
    unsigned char ret = feed_chunks(ex, xml, 5);
    CU_ASSERT_EQUAL(ret, SXMLExplorerInterrupted);
    CU_ASSERT_EQUAL(simple_tag_counter, 2);
    sxml_destroy_explorer(ex);
}

static unsigned int attr_counter = 0;
static unsigned char chunked_attr_on_attr_key(char* key) {
    if (strcmp(key, "hoge") == 0 && attr_counter == 1) attr_counter++;
    if (strcmp(key, "no") == 0 && attr_counter == 3) attr_counter++;
    if (strcmp(key, "goe") == 0 && attr_counter == 6) attr_counter++;
    return SXMLExplorerContinue;
}
static unsigned char chunked_attr_on_attr_val(char* val) {
    if (strcmp(val, "fuga") == 0 && attr_counter == 2) attr_counter++;
    if (strcmp(val, "</tag>") == 0 && attr_counter == 4) attr_counter++;
    if (strcmp(val, "ds") == 0 && attr_counter == 7) attr_counter++;
    return SXMLExplorerContinue;
}
static unsigned char chunked_attr_on_tag(char* name) {
    if (strcmp(name, "tag") == 0 && attr_counter == 0) attr_counter++;
    if (strcmp(name, "tag2") == 0 && attr_counter == 5) attr_counter++;
    if (strcmp(name, "/tag") == 0 && attr_counter == 8) attr_counter++;
    return SXMLExplorerContinue;
}

void test_chunked_attributes(void) {
    const char xml[] = "<?xml version=\"1.1\"?><tag hoge=\"fuga\" no=\"</tag>\"><tag2 goe=\"ds\"   /></tag>";
    SXMLExplorer* ex = sxml_make_explorer();
    attr_counter = 0;
    sxml_register_func(ex, chunked_attr_on_tag, NULL, chunked_attr_on_attr_key, chunked_attr_on_attr_val);
    unsigned char ret = feed_chunks(ex, xml, 7);
    CU_ASSERT_EQUAL(ret, SXMLExplorerComplete);
    CU_ASSERT_EQUAL(attr_counter, 9);
    sxml_destroy_explorer(ex);
}

static unsigned int comment_counter = 0;
static unsigned char chunked_comment_cb(char* c) {
    comment_counter++;
    return SXMLExplorerContinue;
}

void test_chunked_comments(void) {
    const char xml[] = "<?xml version=\"1.0\"?><!-- First comment --><root>text<!-- Second comment --></root>";
    SXMLExplorer* ex = sxml_make_explorer();
    comment_counter = 0;
    sxml_register_func(ex, NULL, NULL, NULL, NULL);
    sxml_register_comment_func(ex, chunked_comment_cb);
    unsigned char ret = feed_chunks(ex, xml, 32);
    CU_ASSERT_EQUAL(ret, SXMLExplorerComplete);
    CU_ASSERT_EQUAL(comment_counter, 2);
    sxml_destroy_explorer(ex);
}

static unsigned int sitemap_tag_count = 0;
static unsigned char sitemap_on_tag(char* name) {
    sitemap_tag_count++;
    return SXMLExplorerContinue;
}

static char* read_file_to_string(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* str = (char*)malloc(size + 1);
    if (!str) { fclose(f); return NULL; }
    fread(str, 1, size, f);
    str[size] = '\0';
    fclose(f);
    return str;
}

void test_chunked_sitemap(void) {
    char* xml = read_file_to_string("test-data/test-sitemap.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);
    SXMLExplorer* ex = sxml_make_explorer();
    sitemap_tag_count = 0;
    sxml_register_func(ex, sitemap_on_tag, NULL, NULL, NULL);
    unsigned char ret = feed_chunks(ex, xml, 32);
    CU_ASSERT_EQUAL(ret, SXMLExplorerComplete);
    CU_ASSERT_EQUAL(sitemap_tag_count, 12);
    sxml_destroy_explorer(ex);
    free(xml);
}

void add_chunked_tests(CU_pSuite* suite) {
    CU_add_test(*suite, "Chunked simple XML", test_chunked_simple_xml);
    CU_add_test(*suite, "Chunked attributes", test_chunked_attributes);
    CU_add_test(*suite, "Chunked comments", test_chunked_comments);
    CU_add_test(*suite, "Chunked sitemap", test_chunked_sitemap);
}

