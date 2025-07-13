#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <stdio.h>

#include "sparsexml.h"

/*
 * Comprehensive entity reference tests
 * Tests standard XML entities, numeric character references, and extended HTML entities
 */

// Global counters for entity tests
static unsigned int test_standard_content_count = 0;
static unsigned int test_decimal_content_count = 0;
static unsigned int test_hex_content_count = 0;
static unsigned int test_extended_content_count = 0;
static unsigned int test_mixed_content_count = 0;
static unsigned int test_attr_count = 0;
static unsigned int test_disabled_content_count = 0;
static unsigned int test_complex_content_count = 0;
static unsigned char found_amp = 0, found_copy = 0, found_code = 0, found_unicode = 0;

// Static callback functions for entity tests
static unsigned char test_standard_xml_entities_on_content(char *content) {
  test_standard_content_count++;
  printf("DEBUG Standard entities: Content %d: '%s'\n", test_standard_content_count, content);
  if (test_standard_content_count == 1) {
    CU_ASSERT(strcmp(content, "<tag> content & \"quoted\" 'text'") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned char test_numeric_decimal_on_content(char *content) {
  test_decimal_content_count++;
  if (test_decimal_content_count == 1) {
    CU_ASSERT(strcmp(content, "ABC") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned char test_numeric_hex_on_content(char *content) {
  test_hex_content_count++;
  if (test_hex_content_count == 1) {
    CU_ASSERT(strcmp(content, "ABC") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned char test_extended_html_on_content(char *content) {
  test_extended_content_count++;
  if (test_extended_content_count == 1) {
    CU_ASSERT(strcmp(content, "(c) (R) (TM)   E #") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned char test_mixed_entity_on_content(char *content) {
  test_mixed_content_count++;
  if (test_mixed_content_count == 1) {
    CU_ASSERT(strcmp(content, "<TestA> (c)  ") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned char test_entity_in_attributes_on_attribute_value(char *value) {
  test_attr_count++;
  if (test_attr_count == 1) {
    CU_ASSERT(strcmp(value, "<value>") == 0);
  } else if (test_attr_count == 2) {
    CU_ASSERT(strcmp(value, "AB") == 0);
  }
  return SXMLExplorerContinue;
}

static unsigned char test_entity_processing_disabled_on_content(char *content) {
  test_disabled_content_count++;
  if (test_disabled_content_count == 1) {
    // Should contain raw entity references when processing is disabled
    CU_ASSERT(strstr(content, "&lt;") != NULL);
    CU_ASSERT(strstr(content, "&gt;") != NULL);
  }
  return SXMLExplorerContinue;
}

static unsigned char test_complex_real_world_on_content(char *content) {
  test_complex_content_count++;
  if (strstr(content, "API Reference & Documentation") != NULL) {
    found_amp = 1;
  }
  if (strstr(content, "(c) 2024 Company (TM)") != NULL) {
    found_copy = 1;
  }
  if (strstr(content, "<html>") != NULL && strstr(content, "</html>") != NULL) {
    found_code = 1;
  }
  if (strstr(content, "Test: ABC") != NULL) {
    found_unicode = 1;
  }
  return SXMLExplorerContinue;
}

void test_standard_xml_entities(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&lt;tag&gt; content &amp; &quot;quoted&quot; &apos;text&apos;</test>";
  
  test_standard_content_count = 0;
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_register_func(explorer, NULL, test_standard_xml_entities_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_standard_content_count == 1);
  
  sxml_destroy_explorer(explorer);
}

void test_numeric_character_references_decimal(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&#65;&#66;&#67;</test>";  // ABC
  
  test_decimal_content_count = 0;
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_numeric_entities(explorer, 1);
  sxml_register_func(explorer, NULL, test_numeric_decimal_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_decimal_content_count == 1);
  
  sxml_destroy_explorer(explorer);
}

void test_numeric_character_references_hex(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&#x41;&#x42;&#x43;</test>";  // ABC
  
  test_hex_content_count = 0;
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_numeric_entities(explorer, 1);
  sxml_register_func(explorer, NULL, test_numeric_hex_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_hex_content_count == 1);
  
  sxml_destroy_explorer(explorer);
}

void test_extended_html_entities(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&copy; &reg; &trade; &nbsp; &euro; &pound;</test>";
  
  test_extended_content_count = 0;
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_extended_entities(explorer, 1);
  sxml_register_func(explorer, NULL, test_extended_html_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_extended_content_count == 1);
  
  sxml_destroy_explorer(explorer);
}

void test_mixed_entity_types(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&lt;Test&#65;&gt; &copy; &#x20;</test>";
  
  test_mixed_content_count = 0;
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_numeric_entities(explorer, 1);
  sxml_enable_extended_entities(explorer, 1);
  sxml_register_func(explorer, NULL, test_mixed_entity_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_mixed_content_count == 1);
  
  sxml_destroy_explorer(explorer);
}

void test_entity_in_attributes(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test title=\"&lt;value&gt;\" desc=\"&#65;&#66;\">content</test>";
  
  test_attr_count = 0;
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_numeric_entities(explorer, 1);
  sxml_register_func(explorer, NULL, NULL, NULL, test_entity_in_attributes_on_attribute_value);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_attr_count == 2);
  
  sxml_destroy_explorer(explorer);
}

void test_invalid_entities_handling(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&invalid; &unknown;</test>";
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_register_func(explorer, NULL, NULL, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  // Should return error for invalid entity
  CU_ASSERT(result == SXMLExplorerErrorInvalidEntity);
  
  sxml_destroy_explorer(explorer);
}

void test_numeric_entity_out_of_range(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&#255;</test>";  // Beyond ASCII range
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_numeric_entities(explorer, 1);
  sxml_register_func(explorer, NULL, NULL, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  // Should return error for out-of-range numeric entity
  CU_ASSERT(result == SXMLExplorerErrorInvalidEntity);
  
  sxml_destroy_explorer(explorer);
}

void test_entity_processing_disabled(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><test>&lt;raw&gt;</test>";
  
  test_disabled_content_count = 0;
  explorer = sxml_make_explorer();
  // Note: NOT enabling entity processing
  sxml_register_func(explorer, NULL, test_entity_processing_disabled_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_disabled_content_count == 1);
  
  sxml_destroy_explorer(explorer);
}

void test_complex_real_world_entities(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?>\n"
               "<document>\n"
               "  <title>API Reference &amp; Documentation</title>\n"
               "  <copyright>&copy; 2024 Company &trade;</copyright>\n"
               "  <code>&lt;html&gt;&#10;  &lt;body&gt;&#10;&lt;/html&gt;</code>\n"
               "  <unicode>Test: &#65;&#x42;&#67;</unicode>\n"
               "</document>";
  
  test_complex_content_count = 0;
  found_amp = 0; found_copy = 0; found_code = 0; found_unicode = 0;
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_numeric_entities(explorer, 1);
  sxml_enable_extended_entities(explorer, 1);
  sxml_register_func(explorer, NULL, test_complex_real_world_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(test_complex_content_count >= 4);
  CU_ASSERT(found_amp == 1);
  CU_ASSERT(found_copy == 1);
  CU_ASSERT(found_code == 1);
  CU_ASSERT(found_unicode == 1);
  
  sxml_destroy_explorer(explorer);
}

void add_entity_tests(CU_pSuite* suite) {
  CU_add_test(*suite, "Standard XML entities", test_standard_xml_entities);
  CU_add_test(*suite, "Numeric character references (decimal)", test_numeric_character_references_decimal);
  CU_add_test(*suite, "Numeric character references (hex)", test_numeric_character_references_hex);
  CU_add_test(*suite, "Extended HTML entities", test_extended_html_entities);
  CU_add_test(*suite, "Mixed entity types", test_mixed_entity_types);
  CU_add_test(*suite, "Entities in attributes", test_entity_in_attributes);
  CU_add_test(*suite, "Invalid entities handling", test_invalid_entities_handling);
  CU_add_test(*suite, "Numeric entity out of range", test_numeric_entity_out_of_range);
  CU_add_test(*suite, "Entity processing disabled", test_entity_processing_disabled);
  CU_add_test(*suite, "Complex real-world entities", test_complex_real_world_entities);
}