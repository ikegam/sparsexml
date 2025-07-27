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
  FILE* f = fopen("test-oss-1.exi", "rb");
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

void add_exi_tests(CU_pSuite* suite) {
  CU_add_test(*suite, "Parse simple EXI", test_parse_simple_exi);
  CU_add_test(*suite, "Parse EXI with attribute", test_parse_exi_with_attribute);
  CU_add_test(*suite, "Parse EXI with namespace", test_parse_exi_with_namespace);
  CU_add_test(*suite, "Parse Atom feed EXI", test_parse_atom_feed_exi);
}
