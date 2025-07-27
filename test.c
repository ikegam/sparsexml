#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>

#include "sparsexml.h"

void add_private_test(CU_pSuite*);
void add_oss_xml_tests(CU_pSuite*);
void add_entity_tests(CU_pSuite*);
void add_exi_tests(CU_pSuite*);

// Static callback functions for tests
static unsigned char test_parse_simple_xml_on_tag(char *name) {
  static int c = 0;
  if (strcmp("tag", name) == 0 && c == 0) {
    c++;
  } else if (strcmp("/tag", name) == 0 && c == 1) {
    c++;
  }
  if (c==2) {
    return SXMLExplorerStop;
  }
  return SXMLExplorerContinue;
}

static unsigned int attr_test_c = 0;  // Global variable for attribute test

static unsigned char test_check_parsing_attribute_on_attribute_key(char *name) {
  if (strcmp("hoge", name) == 0 && attr_test_c == 1) {
    attr_test_c++;
  }
  if (strcmp("no", name) == 0 && attr_test_c == 3) {
    attr_test_c++;
  }
  if (strcmp("goe", name) == 0 && attr_test_c == 6) {
    attr_test_c++;
  }
  return SXMLExplorerContinue;
}

static unsigned char test_check_parsing_attribute_on_attribute_value(char *name) {
  if (strcmp("fuga", name) == 0 && attr_test_c == 2) {
    attr_test_c++;
  }
  if (strcmp("</tag>", name) == 0 && attr_test_c == 4) {
    attr_test_c++;
  }
  if (strcmp("ds", name) == 0 && attr_test_c == 7) {
    attr_test_c++;
  }
  return SXMLExplorerContinue;
}

static unsigned char test_check_parsing_attribute_on_tag(char *name) {
  if (strcmp("tag", name) == 0 && attr_test_c == 0) {
    attr_test_c++;
  }
  if (strcmp("tag2", name) == 0 && attr_test_c == 5) {
    attr_test_c++;
  }
  if (strcmp("/tag", name) == 0 && attr_test_c == 8) {
    attr_test_c++;
  }
  return SXMLExplorerContinue;
}

static unsigned int comment_count = 0;
static unsigned char test_check_parsing_comments_on_comment(char *comment) {
  comment_count++;
  if (comment_count == 1) {
    CU_ASSERT(strcmp(comment, " First comment ") == 0);
  } else if (comment_count == 2) {
    CU_ASSERT(strcmp(comment, " Second comment ") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned int cdata_content_count = 0;
static unsigned char test_check_parsing_cdata_on_content(char *content) {
  cdata_content_count++;
  if (cdata_content_count == 1) {
    CU_ASSERT(strcmp(content, "<tag>content & special chars</tag>") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned int entity_content_count = 0;
static unsigned char test_check_parsing_entities_on_content(char *content) {
  entity_content_count++;
  if (entity_content_count == 1) {
    CU_ASSERT(strcmp(content, "simple content") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned int namespace_tag_count = 0;
static unsigned char test_check_parsing_namespaces_on_tag(char *name) {
  namespace_tag_count++;
  if (namespace_tag_count == 1) {
    CU_ASSERT(strcmp(name, "root") == 0);  // namespace processing strips prefix
  } else if (namespace_tag_count == 2) {
    CU_ASSERT(strcmp(name, "child") == 0);
  } else if (namespace_tag_count == 3) {
    CU_ASSERT(strcmp(name, "child") == 0);  // closing tag
  } else if (namespace_tag_count == 4) {
    CU_ASSERT(strcmp(name, "root") == 0);   // closing tag
  }
  return SXMLExplorerContinue;
}

static unsigned int doctype_tag_count = 0;
static unsigned char test_check_parsing_doctype_on_tag(char *name) {
  doctype_tag_count++;
  if (doctype_tag_count == 1) {
    CU_ASSERT(strcmp(name, "root") == 0);
  } else if (doctype_tag_count == 2) {
    CU_ASSERT(strcmp(name, "/root") == 0);
  }
  return SXMLExplorerContinue;
}

void test_parse_simple_xml(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.1\"?><tag></tag>";
  unsigned char ret;

  explorer = sxml_make_explorer();
  sxml_register_func(explorer, test_parse_simple_xml_on_tag, NULL, NULL, NULL);
  ret = sxml_run_explorer(explorer, xml);
  CU_ASSERT (ret == SXMLExplorerInterrupted);
  sxml_destroy_explorer(explorer);
}

// Static callback functions for test_check_event_on_content
static unsigned char test_check_event_on_content_on_content(char *content) {
  static int c = 0;
  switch (content[0]) {
    case '1':
      if (content[2] == '0') {
        CU_ASSERT (c++ == 0);
      }
      break;
    case '2':
      c++;
      break;
    case '3':
      c++;
      break;
  }
  if (c==3) {
    return SXMLExplorerStop;
  }
  return SXMLExplorerContinue;
}

void test_check_event_on_content(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.1\"?><tag>1 0<tag2>2</tag2>3</tag>";
  unsigned char ret;

  explorer = sxml_make_explorer();
  sxml_register_func(explorer, NULL, test_check_event_on_content_on_content, NULL, NULL);
  ret = sxml_run_explorer(explorer, xml);
  CU_ASSERT (ret == SXMLExplorerInterrupted);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_attribute(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.1\"?><tag hoge=\"fuga\" no=\"</tag>\"><tag2 goe=\"ds\"   /></tag>";

  attr_test_c = 0;  // Reset counter
  explorer = sxml_make_explorer();
  sxml_register_func(explorer, test_check_parsing_attribute_on_tag, NULL, test_check_parsing_attribute_on_attribute_key, test_check_parsing_attribute_on_attribute_value);
  sxml_run_explorer(explorer, xml);
  CU_ASSERT(attr_test_c == 9);

  sxml_destroy_explorer(explorer);
}

void test_check_parsing_comments(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><!-- First comment --><root>content<!-- Second comment --></root>";
  
  comment_count = 0;  // Reset counter
  explorer = sxml_make_explorer();
  sxml_register_func(explorer, NULL, NULL, NULL, NULL);
  sxml_register_comment_func(explorer, test_check_parsing_comments_on_comment);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(comment_count == 2);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_cdata(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><root><![CDATA[<tag>content & special chars</tag>]]></root>";
  
  cdata_content_count = 0;  // Reset counter
  explorer = sxml_make_explorer();
  sxml_register_func(explorer, NULL, test_check_parsing_cdata_on_content, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(cdata_content_count == 1);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_entities(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><root>simple content</root>";
  
  entity_content_count = 0;  // Reset counter
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_register_func(explorer, NULL, test_check_parsing_entities_on_content, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(entity_content_count == 1);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_namespaces(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><ns:root xmlns:ns=\"http://example.com\"><ns:child>content</ns:child></ns:root>";
  
  namespace_tag_count = 0;  // Reset counter
  explorer = sxml_make_explorer();
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, test_check_parsing_namespaces_on_tag, NULL, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(namespace_tag_count == 4);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_doctype(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><!DOCTYPE root SYSTEM \"example.dtd\"><root>content</root>";
  
  doctype_tag_count = 0;  // Reset counter
  explorer = sxml_make_explorer();
  sxml_register_func(explorer, test_check_parsing_doctype_on_tag, NULL, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(doctype_tag_count == 2);
  sxml_destroy_explorer(explorer);
}

int main(void) {
  CU_pSuite core_suite, internal_suite, advanced_suite, entity_suite, realworld_suite, exi_suite;
  CU_initialize_registry();

  // Core API and Basic Parsing Suite
  core_suite = CU_add_suite("Core API & Basic Parsing", NULL, NULL);
  CU_add_test(core_suite, "Parse simple XML", test_parse_simple_xml);
  CU_add_test(core_suite, "Check status in running explorer", test_check_event_on_content);
  CU_add_test(core_suite, "Check parsing attribute", test_check_parsing_attribute);

  // Internal Implementation Suite
  internal_suite = CU_add_suite("Internal Implementation", NULL, NULL);
  add_private_test(&internal_suite);

  // Advanced XML Features Suite
  advanced_suite = CU_add_suite("Advanced XML Features", NULL, NULL);
  CU_add_test(advanced_suite, "Check XML comment parsing", test_check_parsing_comments);
  CU_add_test(advanced_suite, "Check CDATA section parsing", test_check_parsing_cdata);
  CU_add_test(advanced_suite, "Check entity reference parsing", test_check_parsing_entities);
  CU_add_test(advanced_suite, "Check namespace parsing", test_check_parsing_namespaces);
  CU_add_test(advanced_suite, "Check DOCTYPE parsing", test_check_parsing_doctype);

  // Entity Processing Suite
  entity_suite = CU_add_suite("Entity Processing", NULL, NULL);
  add_entity_tests(&entity_suite);

  // Real-world XML Suite
  realworld_suite = CU_add_suite("Real-world XML Samples", NULL, NULL);
  add_oss_xml_tests(&realworld_suite);

  // EXI Suite
  exi_suite = CU_add_suite("EXI Support", NULL, NULL);
  add_exi_tests(&exi_suite);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}
