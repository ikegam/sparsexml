#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>

#include "sparsexml.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned int exi_tag_count = 0;
static unsigned int exi_content_count = 0;

static unsigned char exi_on_tag(char *name) {
  if (exi_tag_count == 0) {
    CU_ASSERT(strcmp(name, "root") == 0);
  } else if (exi_tag_count == 1) {
    CU_ASSERT(strcmp(name, "/root") == 0);
  }
  exi_tag_count++;
  return SXMLExplorerContinue;
}

static unsigned char exi_on_content(char *content) {
  CU_ASSERT(strcmp(content, "hi") == 0);
  exi_content_count++;
  return SXMLExplorerContinue;
}

void test_parse_simple_exi(void) {
  unsigned char exi[] = {0x01, 4, 'r','o','o','t', 0x04, 2, 'h','i', 0x02, 4, 'r','o','o','t', 0xFF};
  SXMLExplorer* ex = sxml_make_explorer();
  sxml_register_func(ex, exi_on_tag, exi_on_content, NULL, NULL);
  unsigned char ret = sxml_run_explorer_exi(ex, exi, sizeof(exi));
  CU_ASSERT(ret == SXMLExplorerComplete);
  CU_ASSERT(exi_tag_count == 2);
  CU_ASSERT(exi_content_count == 1);
  sxml_destroy_explorer(ex);
}

static unsigned int exi_attr_key_count = 0;
static unsigned int exi_attr_val_count = 0;

static unsigned char exi_on_attr_key(char *key) {
  CU_ASSERT(strcmp(key, "key") == 0);
  exi_attr_key_count++;
  return SXMLExplorerContinue;
}

static unsigned char exi_on_attr_val(char *val) {
  CU_ASSERT(strcmp(val, "value") == 0);
  exi_attr_val_count++;
  return SXMLExplorerContinue;
}

void test_parse_exi_with_attribute(void) {
  unsigned char exi[] = {0x01, 4, 'r','o','o','t', 0x03, 3, 'k','e','y', 5, 'v','a','l','u','e', 0x02, 4, 'r','o','o','t', 0xFF};
  SXMLExplorer* ex = sxml_make_explorer();
  sxml_register_func(ex, NULL, NULL, exi_on_attr_key, exi_on_attr_val);
  unsigned char ret = sxml_run_explorer_exi(ex, exi, sizeof(exi));
  CU_ASSERT(ret == SXMLExplorerComplete);
  CU_ASSERT(exi_attr_key_count == 1);
  CU_ASSERT(exi_attr_val_count == 1);
  sxml_destroy_explorer(ex);
}

static unsigned int exi_ns_tag_count = 0;

static unsigned char exi_on_ns_tag(char *name) {
  if (exi_ns_tag_count == 0) {
    CU_ASSERT(strcmp(name, "ns:root") == 0);
  } else if (exi_ns_tag_count == 1) {
    CU_ASSERT(strcmp(name, "/root") == 0);
  }
  exi_ns_tag_count++;
  return SXMLExplorerContinue;
}

void test_parse_exi_with_namespace(void) {
  unsigned char exi[] = {0x06, 2, 'n','s', 4, 'r','o','o','t', 0x02, 4, 'r','o','o','t', 0xFF};
  SXMLExplorer* ex = sxml_make_explorer();
  sxml_register_func(ex, exi_on_ns_tag, NULL, NULL, NULL);
  unsigned char ret = sxml_run_explorer_exi(ex, exi, sizeof(exi));
  CU_ASSERT(ret == SXMLExplorerComplete);
  CU_ASSERT(exi_ns_tag_count == 2);
  sxml_destroy_explorer(ex);
}

// Callbacks and counters for parsing external EXI sample
static unsigned int atom_exi_tag_count = 0;
static unsigned int atom_exi_content_count = 0;
static unsigned int atom_exi_comment_count = 0;
static unsigned char atom_exi_found_entities = 0;

static unsigned char atom_exi_on_tag(char *name) {
  atom_exi_tag_count++;
  return SXMLExplorerContinue;
}

static unsigned char atom_exi_on_content(char *content) {
  atom_exi_content_count++;
  if (strlen(content) > 0) {
    atom_exi_found_entities = 1;
  }
  return SXMLExplorerContinue;
}

static unsigned char atom_exi_on_comment(char *comment) {
  atom_exi_comment_count++;
  return SXMLExplorerContinue;
}

void test_parse_atom_feed_exi(void) {
  FILE* f = fopen("test-data/test-oss-1.exi", "rb");
  CU_ASSERT_PTR_NOT_NULL_FATAL(f);
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);
  unsigned char* buf = (unsigned char*)malloc(size);
  CU_ASSERT_PTR_NOT_NULL_FATAL(buf);
  fread(buf, 1, size, f);
  fclose(f);

  atom_exi_tag_count = 0;
  atom_exi_content_count = 0;
  atom_exi_comment_count = 0;
  atom_exi_found_entities = 0;

  SXMLExplorer* ex = sxml_make_explorer();
  sxml_enable_entity_processing(ex, 1);
  sxml_register_func(ex, atom_exi_on_tag, atom_exi_on_content, NULL, NULL);
  sxml_register_comment_func(ex, atom_exi_on_comment);
  unsigned char ret = sxml_run_explorer_exi(ex, buf, size);

  CU_ASSERT(ret == SXMLExplorerComplete);
  CU_ASSERT(atom_exi_tag_count >= 10);
  CU_ASSERT(atom_exi_content_count >= 5);
  CU_ASSERT(atom_exi_comment_count == 1);
  CU_ASSERT(atom_exi_found_entities == 1);

  sxml_destroy_explorer(ex);
  free(buf);
}

// Helper function to read a binary file into a buffer
static unsigned char* read_file_to_buffer(const char* filename, unsigned int* size) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    unsigned char* buffer = (unsigned char*)malloc(file_size);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, file_size, f);
    *size = (unsigned int)file_size;
    fclose(f);
    return buffer;
}

// Test for RSS feed EXI
static unsigned int rss_exi_tag_count = 0;
static unsigned int rss_exi_content_count = 0;

static unsigned char rss_exi_on_tag(char* name) {
    rss_exi_tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char rss_exi_on_content(char* content) {
    if (strlen(content) > 0) {
        rss_exi_content_count++;
    }
    return SXMLExplorerContinue;
}

void test_rss_feed_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    unsigned int exi_size;
    unsigned char* exi = read_file_to_buffer("test-data/test-rss.exi", &exi_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

    rss_exi_tag_count = rss_exi_content_count = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_register_func(explorer, rss_exi_on_tag, rss_exi_on_content, NULL, NULL);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(rss_exi_tag_count > 0);  // Should parse some tags
    CU_ASSERT(rss_exi_content_count > 0);  // Should parse some content

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for Atom entry EXI
static unsigned int atom_entry_exi_tag_count = 0;
static unsigned int atom_entry_exi_content_count = 0;

static unsigned char atom_entry_exi_on_tag(char* name) {
    atom_entry_exi_tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char atom_entry_exi_on_content(char* content) {
    if (strlen(content) > 0) {
        atom_entry_exi_content_count++;
    }
    return SXMLExplorerContinue;
}

void test_atom_entry_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    unsigned int exi_size;
    unsigned char* exi = read_file_to_buffer("test-data/test-atom-entry.exi", &exi_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

    atom_entry_exi_tag_count = atom_entry_exi_content_count = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_register_func(explorer, atom_entry_exi_on_tag, atom_entry_exi_on_content, NULL, NULL);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(atom_entry_exi_tag_count > 0);  // Should parse some tags
    CU_ASSERT(atom_entry_exi_content_count > 0);  // Should parse some content

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for sitemap EXI
static unsigned int sitemap_exi_tag_count = 0;
static unsigned int sitemap_exi_content_count = 0;

static unsigned char sitemap_exi_on_tag(char* name) {
    sitemap_exi_tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char sitemap_exi_on_content(char* content) {
    if (strlen(content) > 0) {
        sitemap_exi_content_count++;
    }
    return SXMLExplorerContinue;
}

void test_sitemap_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    unsigned int exi_size;
    unsigned char* exi = read_file_to_buffer("test-data/test-sitemap.exi", &exi_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

    sitemap_exi_tag_count = sitemap_exi_content_count = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_register_func(explorer, sitemap_exi_on_tag, sitemap_exi_on_content, NULL, NULL);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(sitemap_exi_tag_count > 0);  // Should parse some tags
    CU_ASSERT(sitemap_exi_content_count > 0);  // Should parse some content

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for EXI with comments
static unsigned int comments_exi_count = 0;

static unsigned char comments_exi_callback(char* text) {
    comments_exi_count++;
    return SXMLExplorerContinue;
}

void test_with_comments_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    unsigned int exi_size;
    unsigned char* exi = read_file_to_buffer("test-data/test-with-comments.exi", &exi_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

    comments_exi_count = 0;
    sxml_register_func(explorer, NULL, NULL, NULL, NULL);
    sxml_register_comment_func(explorer, comments_exi_callback);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    // EXI parsing should complete successfully with schema-less parser
    CU_ASSERT(result == SXMLExplorerComplete);
    // Note: Comments are extracted from the data and processed by the schema-less parser

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for EXI with CDATA
static unsigned int cdata_exi_content_count = 0;

static unsigned char cdata_exi_on_content(char* c) {
    if (strlen(c) > 0) {
        cdata_exi_content_count++;
    }
    return SXMLExplorerContinue;
}

void test_with_cdata_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    unsigned int exi_size;
    unsigned char* exi = read_file_to_buffer("test-data/test-with-cdata.exi", &exi_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

    cdata_exi_content_count = 0;
    sxml_register_func(explorer, NULL, cdata_exi_on_content, NULL, NULL);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(cdata_exi_content_count > 0);  // Should parse some content

    sxml_destroy_explorer(explorer);
    free(exi);
}

void add_exi_tests(CU_pSuite* suite) {
  // Real EXI file tests with external data
  CU_add_test(*suite, "Parse Atom feed EXI", test_parse_atom_feed_exi);
  CU_add_test(*suite, "Parse RSS feed EXI", test_rss_feed_exi);
  CU_add_test(*suite, "Parse Atom entry EXI", test_atom_entry_exi);
  CU_add_test(*suite, "Parse sitemap EXI", test_sitemap_exi);
  CU_add_test(*suite, "Parse EXI with comments", test_with_comments_exi);
  CU_add_test(*suite, "Parse EXI with CDATA", test_with_cdata_exi);
}
