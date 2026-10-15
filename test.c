#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <stdio.h>

#include "sparsexml.h"
#include "test-log.h"

void add_private_test(CU_pSuite*);
void add_oss_xml_tests(CU_pSuite*);
void add_entity_tests(CU_pSuite*);
void add_exi_tests(CU_pSuite*);
void add_chunked_tests(CU_pSuite*);

// Static callback functions for tests
static unsigned char test_parse_simple_xml_on_tag(SXMLExplorer* explorer, char *name) {
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

static unsigned char test_check_parsing_attribute_on_attribute_key(SXMLExplorer* explorer, char *name) {
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

static unsigned char test_check_parsing_attribute_on_attribute_value(SXMLExplorer* explorer, char *name) {
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

static unsigned char test_check_parsing_attribute_on_tag(SXMLExplorer* explorer, char *name) {
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
static unsigned char test_check_parsing_comments_on_comment(SXMLExplorer* explorer, char *comment) {
  comment_count++;
  if (comment_count == 1) {
    CU_ASSERT(strcmp(comment, " First comment ") == 0);
  } else if (comment_count == 2) {
    CU_ASSERT(strcmp(comment, " Second comment ") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned int cdata_content_count = 0;
static unsigned char test_check_parsing_cdata_on_content(SXMLExplorer* explorer, char *content) {
  cdata_content_count++;
  if (cdata_content_count == 1) {
    CU_ASSERT(strcmp(content, "<tag>content & special chars</tag>") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned int entity_content_count = 0;
static unsigned char test_check_parsing_entities_on_content(SXMLExplorer* explorer, char *content) {
  entity_content_count++;
  if (entity_content_count == 1) {
    CU_ASSERT(strcmp(content, "simple content") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned int namespace_tag_count = 0;
static unsigned char test_check_parsing_namespaces_on_tag(SXMLExplorer* explorer, char *name) {
  namespace_tag_count++;
  if (namespace_tag_count == 1) {
    CU_ASSERT(strcmp(name, "root") == 0);  // namespace processing strips prefix
  } else if (namespace_tag_count == 2) {
    CU_ASSERT(strcmp(name, "child") == 0);
  } else if (namespace_tag_count == 3) {
    CU_ASSERT(strcmp(name, "/child") == 0);  // closing tag keeps "/"
  } else if (namespace_tag_count == 4) {
    CU_ASSERT(strcmp(name, "/root") == 0);   // closing tag keeps "/"
  }
  return SXMLExplorerContinue;
}

static unsigned int doctype_tag_count = 0;
static unsigned char test_check_parsing_doctype_on_tag(SXMLExplorer* explorer, char *name) {
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
static unsigned char test_check_event_on_content_on_content(SXMLExplorer* explorer, char *content) {
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


static unsigned int long_content_length = 0;
static unsigned char test_long_content_on_content(SXMLExplorer* explorer, char* content) {
  long_content_length = strlen(content);
  return SXMLExplorerContinue;
}

// Events of the syntax tests, recorded through the explorer's user data
static TestLog syntax_log;

static unsigned char parse_syntax(const char* text) {
  char xml[512];
  unsigned char ret;
  SXMLExplorer* explorer = sxml_make_explorer();
  strcpy(xml, text);
  test_log_attach(explorer, &syntax_log, TEST_LOG_COMMENTS);
  ret = sxml_run_explorer(explorer, xml);
  sxml_destroy_explorer(explorer);
  return ret;
}

void test_self_closing_tags(void) {
  CU_ASSERT_EQUAL(parse_syntax("<root><a/><b x=\"1\"/><c /></root>"), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log), "T[root]T[a]T[/a]T[b]K[x]V[1]T[/b]T[c]T[/c]T[/root]");
}

void test_attribute_syntax(void) {
  CU_ASSERT_EQUAL(parse_syntax("<r a='single' b = \"spaced\"\n\tc=\"\" d=\"it's\" e='say \"hi\"'>x</r>"), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log),
    "T[r]K[a]V[single]K[b]V[spaced]K[c]V[]K[d]V[it's]K[e]V[say \"hi\"]C[x]T[/r]");
}

void test_whitespace_in_tags(void) {
  CU_ASSERT_EQUAL(parse_syntax("\n  <root\n  attr=\"v\"\n><child\t/></root\n>"), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log), "C[\n  ]T[root]K[attr]V[v]T[child]T[/child]T[/root]");
}

void test_processing_instructions(void) {
  CU_ASSERT_EQUAL(parse_syntax("<?xml version=\"1.0\"?><?style href=\"a>b\"?><r>a<?pi data?>b</r>"), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log), "T[r]C[a]C[b]T[/r]");
}

void test_doctype_internal_subset(void) {
  CU_ASSERT_EQUAL(parse_syntax("<!DOCTYPE r [<!ELEMENT r ANY><!ATTLIST r a CDATA \"x\">]><r>ok</r>"), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log), "T[r]C[ok]T[/r]");
}

void test_content_around_markup(void) {
  CU_ASSERT_EQUAL(parse_syntax("<r>a<!-- c -- d -->b<![CDATA[]]x]]>c</r>"), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log), "T[r]C[a]M[ c -- d ]C[b]C[]]x]C[c]T[/r]");
}

void test_empty_input(void) {
  SXMLExplorer* explorer = sxml_make_explorer();
  char empty[] = "";
  CU_ASSERT_EQUAL(sxml_run_explorer(explorer, empty), SXMLExplorerComplete);
  CU_ASSERT_EQUAL(sxml_run_explorer(explorer, NULL), SXMLExplorerErrorMalformedXML);
  sxml_destroy_explorer(explorer);
}

void test_static_explorer(void) {
  static void* memory[4096 / sizeof(void*)];
  char xml[] = "<a>1</a>";
  void* tiny[1];
  SXMLExplorer* explorer;

  CU_ASSERT(sxml_explorer_size() <= sizeof(memory));
  CU_ASSERT_PTR_NULL(sxml_init_explorer(tiny, sizeof(tiny)));
  CU_ASSERT_PTR_NULL(sxml_init_explorer((char*)memory + 1, sizeof(memory) - 1));  // misaligned
  explorer = sxml_init_explorer(memory, sizeof(memory));
  CU_ASSERT_PTR_NOT_NULL_FATAL(explorer);
  test_log_attach(explorer, &syntax_log, 0);
  CU_ASSERT_EQUAL(sxml_run_explorer(explorer, xml), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log), "T[a]C[1]T[/a]");
  sxml_destroy_explorer(explorer);  // no-op for caller-provided memory
}

void test_reset_explorer(void) {
  SXMLExplorer* explorer = sxml_make_explorer();
  char broken[] = "<a attr=\"unterminated";
  char xml[] = "<b>2</b>";

  test_log_attach(explorer, &syntax_log, 0);
  sxml_run_explorer(explorer, broken);
  sxml_reset_explorer(explorer);
  test_log_attach(explorer, &syntax_log, 0);
  CU_ASSERT_EQUAL(sxml_run_explorer(explorer, xml), SXMLExplorerComplete);
  CU_ASSERT_STRING_EQUAL(test_log_string(&syntax_log), "T[b]C[2]T[/b]");
  sxml_destroy_explorer(explorer);
}

void test_explorer_kinds(void) {
  SXMLExplorer* xml_explorer = sxml_make_explorer();
  SXMLExplorer* exi_explorer = sxml_make_exi_explorer();
  char xml[] = "<a/>";
  unsigned char exi[] = {0x80, 0x00};

  CU_ASSERT_PTR_NOT_NULL_FATAL(exi_explorer);
  CU_ASSERT_EQUAL(sxml_run_explorer(exi_explorer, xml), SXMLExplorerErrorUnsupported);
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(xml_explorer, exi, sizeof(exi)), SXMLExplorerErrorUnsupported);
  CU_ASSERT(sxml_exi_explorer_size() > sizeof(void*));
  sxml_destroy_explorer(xml_explorer);
  sxml_destroy_explorer(exi_explorer);
}

// Callbacks reach their explorer, and through it their own state
static unsigned char user_data_on_tag(SXMLExplorer* explorer, char* name) {
  unsigned int* count = sxml_get_user_data(explorer);
  (*count)++;
  return SXMLExplorerContinue;
}

void test_user_data(void) {
  SXMLExplorer* first = sxml_make_explorer();
  SXMLExplorer* second = sxml_make_explorer();
  unsigned int first_count = 0, second_count = 0;
  char xml1[] = "<a><b/></a>";
  char xml2[] = "<c/>";

  sxml_set_user_data(first, &first_count);
  sxml_set_user_data(second, &second_count);
  sxml_register_func(first, user_data_on_tag, NULL, NULL, NULL);
  sxml_register_func(second, user_data_on_tag, NULL, NULL, NULL);
  sxml_run_explorer(first, xml1);
  sxml_run_explorer(second, xml2);
  CU_ASSERT_EQUAL(first_count, 4);
  CU_ASSERT_EQUAL(second_count, 2);
  CU_ASSERT_PTR_EQUAL(sxml_get_user_data(first), &first_count);
  sxml_destroy_explorer(first);
  sxml_destroy_explorer(second);
}

void test_long_content_truncated(void) {
  static char xml[SXMLElementLength * 2 + 32];
  unsigned int i;
  strcpy(xml, "<r>");
  for (i = 0; i < SXMLElementLength + 100; i++) strcat(xml, "x");
  strcat(xml, "</r>");

  SXMLExplorer* explorer = sxml_make_explorer();
  sxml_register_func(explorer, NULL, test_long_content_on_content, NULL, NULL);
  long_content_length = 0;
  CU_ASSERT_EQUAL(sxml_run_explorer(explorer, xml), SXMLExplorerComplete);
  CU_ASSERT_EQUAL(long_content_length, SXMLElementLength - 1);
  sxml_destroy_explorer(explorer);
}

int main(void) {
  unsigned int failures;
  CU_pSuite core_suite, internal_suite, advanced_suite, entity_suite, realworld_suite, chunked_suite, exi_suite;
  CU_initialize_registry();

  // Core API and Basic Parsing Suite
  core_suite = CU_add_suite("Core API & Basic Parsing", NULL, NULL);
  CU_add_test(core_suite, "Parse simple XML", test_parse_simple_xml);
  CU_add_test(core_suite, "Check status in running explorer", test_check_event_on_content);
  CU_add_test(core_suite, "Check parsing attribute", test_check_parsing_attribute);
  CU_add_test(core_suite, "Self-closing tags", test_self_closing_tags);
  CU_add_test(core_suite, "Attribute syntax variants", test_attribute_syntax);
  CU_add_test(core_suite, "Whitespace inside tags", test_whitespace_in_tags);
  CU_add_test(core_suite, "Processing instructions", test_processing_instructions);
  CU_add_test(core_suite, "DOCTYPE internal subset", test_doctype_internal_subset);
  CU_add_test(core_suite, "Content around comments and CDATA", test_content_around_markup);
  CU_add_test(core_suite, "Empty input", test_empty_input);
  CU_add_test(core_suite, "Explorer in caller memory", test_static_explorer);
  CU_add_test(core_suite, "Reset explorer", test_reset_explorer);
  CU_add_test(core_suite, "Long content is truncated", test_long_content_truncated);
  CU_add_test(core_suite, "XML and EXI explorers", test_explorer_kinds);
  CU_add_test(core_suite, "User data in callbacks", test_user_data);

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

  // Chunked Parsing Suite
  chunked_suite = CU_add_suite("Chunked XML Parsing", NULL, NULL);
  add_chunked_tests(&chunked_suite);

  // EXI Suite
  exi_suite = CU_add_suite("EXI Support", NULL, NULL);
  add_exi_tests(&exi_suite);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  failures = CU_get_number_of_failures();
  CU_cleanup_registry();

  return failures == 0 ? 0 : 1;
}
