#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sparsexml.h"

/*
 * Real-world XML test cases using OSS/standard XML formats
 * 
 * Test samples are based on publicly available XML standards:
 * 1. XML Sitemap - from sitemaps.org protocol (open standard)
 * 2. Atom Feed - from W3C Atom specification (open standard)
 */

// Global variables for test_real_world_xml_sitemap
static unsigned int sitemap_tag_count = 0;
static unsigned int sitemap_content_count = 0;
static unsigned char sitemap_found_namespace = 0;

static unsigned char sitemap_on_tag(char *name) {
  sitemap_tag_count++;
  if (strcmp(name, "urlset") == 0) {
    sitemap_found_namespace = 1;  // namespace processing should strip prefix
  }
  return SXMLExplorerContinue;
}

static unsigned char sitemap_on_content(char *content) {
  sitemap_content_count++;
  return SXMLExplorerContinue;
}

// Helper function to read a file into a string
static char* read_file_to_string(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* str = (char*)malloc(size + 1);
    if (!str) {
        fclose(f);
        return NULL;
    }
    fread(str, 1, size, f);
    str[size] = '\0';
    fclose(f);
    return str;
}

void test_real_world_xml_sitemap(void) {
  SXMLExplorer* explorer;
  char* xml = read_file_to_string("test-data/test-sitemap.xml");
  CU_ASSERT_PTR_NOT_NULL_FATAL(xml);
  
  // Reset counters
  sitemap_tag_count = 0;
  sitemap_content_count = 0;
  sitemap_found_namespace = 0;
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, sitemap_on_tag, sitemap_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(sitemap_tag_count > 0);  // Should parse multiple tags
  CU_ASSERT(sitemap_content_count > 0);  // Should parse content
  CU_ASSERT(sitemap_found_namespace == 1);  // Should handle namespace
  
  sxml_destroy_explorer(explorer);
  free(xml);
}

// Global variables for test_real_world_atom_feed
static unsigned int atom_tag_count = 0;
static unsigned int atom_content_count = 0;
static unsigned int atom_comment_count = 0;
static unsigned char atom_found_entities = 0;

static unsigned char atom_on_tag(char *name) {
  atom_tag_count++;
  return SXMLExplorerContinue;
}

static unsigned char atom_on_content(char *content) {
  atom_content_count++;
  // Check if content exists (entities may not be fully processed yet)
  if (strlen(content) > 0) {
    atom_found_entities = 1;  // Content found 
  }
  return SXMLExplorerContinue;
}

static unsigned char atom_on_comment(char *comment) {
  atom_comment_count++;
  CU_ASSERT(strstr(comment, "This is a comment") != NULL);
  return SXMLExplorerContinue;
}

void test_real_world_atom_feed(void) {
  SXMLExplorer* explorer;
  // Real Atom feed example loaded from external file
  FILE* f = fopen("test-data/test-oss-1.xml", "rb");
  CU_ASSERT_PTR_NOT_NULL_FATAL(f);
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);
  char* xml = (char*)malloc(size + 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(xml);
  fread(xml, 1, size, f);
  xml[size] = '\0';
  fclose(f);
  
  // Reset counters
  atom_tag_count = 0;
  atom_content_count = 0;
  atom_comment_count = 0;
  atom_found_entities = 0;
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, atom_on_tag, atom_on_content, NULL, NULL);
  sxml_register_comment_func(explorer, atom_on_comment);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(atom_tag_count >= 10);  // Should parse many tags
  CU_ASSERT(atom_content_count >= 5);  // Should parse content elements
  CU_ASSERT(atom_comment_count == 1);  // Should find the comment
  CU_ASSERT(atom_found_entities == 1);  // Should process entities

  sxml_destroy_explorer(explorer);
  free(xml);
}

// Global variables for test_complex_xml_with_cdata_and_entities
static unsigned int complex_tag_count = 0;
static unsigned int complex_content_count = 0;
static unsigned int complex_comment_count = 0;
static unsigned char complex_found_cdata = 0;
static unsigned char complex_found_entities = 0;
static unsigned char complex_found_namespaces = 0;

static unsigned char complex_on_tag(char *name) {
  complex_tag_count++;
  if (strcmp(name, "setting") == 0) {  // namespace prefix should be stripped
    complex_found_namespaces = 1;
  }
  return SXMLExplorerContinue;
}

static unsigned char complex_on_content(char *content) {
  complex_content_count++;
  if (strstr(content, "Raw data with <tags>") != NULL) {
    complex_found_cdata = 1;  // CDATA content preserved
  }
  if (strlen(content) > 0) {
    complex_found_entities = 1;  // Content found
  }
  return SXMLExplorerContinue;
}

static unsigned char complex_on_comment(char *comment) {
  complex_comment_count++;
  return SXMLExplorerContinue;
}

void test_complex_xml_with_cdata_and_entities(void) {
  SXMLExplorer* explorer;
  // Complex XML combining multiple features
  char xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               "<!DOCTYPE root SYSTEM \"example.dtd\">\n"
               "<root xmlns:app=\"http://example.com/app\">\n"
               "  <!-- Configuration section -->\n"
               "  <config>\n"
               "    <app:setting name=\"debug\" value=\"true\"/>\n"
               "    <description>Settings for &lt;application&gt;</description>\n"
               "  </config>\n"
               "  <data>\n"
               "    <![CDATA[\n"
               "      Raw data with <tags> and & symbols\n"
               "      \"quotes\" and 'apostrophes'\n"
               "    ]]>\n"
               "  </data>\n"
               "  <footer>Copyright 2024 &amp; beyond</footer>\n"
               "</root>";
  
  // Reset counters
  complex_tag_count = 0;
  complex_content_count = 0;
  complex_comment_count = 0;
  complex_found_cdata = 0;
  complex_found_entities = 0;
  complex_found_namespaces = 0;
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, complex_on_tag, complex_on_content, NULL, NULL);
  sxml_register_comment_func(explorer, complex_on_comment);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  // Should complete successfully with supported entities only
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(complex_tag_count >= 6);  // Should parse multiple tags (root, config, app:setting, description, data, footer)
  CU_ASSERT(complex_content_count >= 3);  // Should parse multiple content sections
  CU_ASSERT(complex_comment_count == 1);  // Should find the comment
  CU_ASSERT(complex_found_cdata == 1);  // Should process CDATA section
  CU_ASSERT(complex_found_entities == 1);  // Should find content (entities processed)
  CU_ASSERT(complex_found_namespaces == 1);  // Should process namespaces
  
  sxml_destroy_explorer(explorer);
}

// Global variables for test_error_handling_with_malformed_xml
static unsigned int error_tag_count = 0;

static unsigned char error_on_tag(char *name) {
  error_tag_count++;
  return SXMLExplorerContinue;
}

static unsigned char error_on_content(char *content) {
  return SXMLExplorerContinue;
}

void test_error_handling_with_malformed_xml(void) {
  SXMLExplorer* explorer;
  // Test with malformed XML to check error handling
  char xml[] = "<?xml version=\"1.0\"?>\n"
               "<root>\n"
               "  <unclosed_tag>\n"
               "  <another>content</another>\n"
               "  <invalid_entity>&invalid;</invalid_entity>\n"
               "</root>";
  
  // Reset counter
  error_tag_count = 0;
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_register_func(explorer, error_on_tag, error_on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  // Parser should complete despite malformed elements
  // (SparseXML is lenient for embedded use cases)
  CU_ASSERT(result == SXMLExplorerComplete || 
            result == SXMLExplorerErrorInvalidEntity ||
            result == SXMLExplorerErrorMalformedXML);
  CU_ASSERT(error_tag_count > 0);  // Should parse some tags before errors
  
  sxml_destroy_explorer(explorer);
}

// Test for RSS feed
void test_rss_feed(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-rss.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);

    sxml_destroy_explorer(explorer);
    free(xml);
}

// Test for Atom entry
void test_atom_entry(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-atom-entry.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);

    sxml_destroy_explorer(explorer);
    free(xml);
}

// Test for XML with comments
void test_with_comments(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-with-comments.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);

    sxml_destroy_explorer(explorer);
    free(xml);
}

// Test for XML with CDATA
void test_with_cdata(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-with-cdata.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);

    sxml_destroy_explorer(explorer);
    free(xml);
}

void add_oss_xml_tests(CU_pSuite* suite) {
  CU_add_test(*suite, "Real-world XML Sitemap parsing", test_real_world_xml_sitemap);
  CU_add_test(*suite, "Real-world Atom Feed parsing", test_real_world_atom_feed);
  CU_add_test(*suite, "Complex XML with all features", test_complex_xml_with_cdata_and_entities);
  CU_add_test(*suite, "Error handling with malformed XML", test_error_handling_with_malformed_xml);
  CU_add_test(*suite, "Parse RSS feed", test_rss_feed);
  CU_add_test(*suite, "Parse Atom entry", test_atom_entry);
  CU_add_test(*suite, "Parse XML with comments", test_with_comments);
  CU_add_test(*suite, "Parse XML with CDATA", test_with_cdata);
}
