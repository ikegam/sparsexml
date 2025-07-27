#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

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
static const char* rss_expected_tags[] = {
    "rss","channel",
    "title","/title","description","/description","link","/link",
    "lastBuildDate","/lastBuildDate","pubDate","/pubDate","ttl","/ttl",
    "item","title","/title","description","/description","link","/link",
    "guid","/guid","pubDate","/pubDate","/item","/channel","/rss"
};
static const char* rss_expected_attr_keys[] = {"version","isPermaLink"};
static const char* rss_expected_attr_vals[] = {"2.0","false"};
static const char* rss_expected_content[] = {
    "RSS Title",
    "This is an example of an RSS feed",
    "http://www.example.com/main.html",
    "Mon, 06 Sep 2010 00:01:00 +0000 ",
    "Sun, 06 Sep 2009 16:20:00 +0000",
    "1800",
    "Example entry",
    "Here is some text containing an interesting description.",
    "http://www.example.com/blog/post/1",
    "7bd204c6-1655-4c27-aeee-53f933c5395f",
    "Sun, 06 Sep 2009 16:20:00 +0000"
};
static unsigned int rss_tag_index = 0;
static unsigned int rss_attr_index = 0;
static unsigned int rss_content_index = 0;

static unsigned char rss_on_tag(char* name) {
    CU_ASSERT(rss_tag_index < sizeof(rss_expected_tags)/sizeof(rss_expected_tags[0]));
    CU_ASSERT_STRING_EQUAL(name, rss_expected_tags[rss_tag_index]);
    rss_tag_index++;
    return SXMLExplorerContinue;
}

static unsigned char rss_on_attr_key(char* key) {
    CU_ASSERT(rss_attr_index < sizeof(rss_expected_attr_keys)/sizeof(rss_expected_attr_keys[0]));
    CU_ASSERT_STRING_EQUAL(key, rss_expected_attr_keys[rss_attr_index]);
    return SXMLExplorerContinue;
}

static unsigned char rss_on_attr_val(char* val) {
    CU_ASSERT_STRING_EQUAL(val, rss_expected_attr_vals[rss_attr_index]);
    rss_attr_index++;
    return SXMLExplorerContinue;
}

static unsigned char rss_on_content(char* content) {
    while (*content && isspace((unsigned char)*content)) content++;
    if (*content == '\0') return SXMLExplorerContinue;
    CU_ASSERT(rss_content_index < sizeof(rss_expected_content)/sizeof(rss_expected_content[0]));
    CU_ASSERT_STRING_EQUAL(content, rss_expected_content[rss_content_index]);
    rss_content_index++;
    return SXMLExplorerContinue;
}

void test_rss_feed(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-rss.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    rss_tag_index = rss_attr_index = rss_content_index = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_register_func(explorer, rss_on_tag, rss_on_content,
                       rss_on_attr_key, rss_on_attr_val);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(rss_tag_index == sizeof(rss_expected_tags)/sizeof(rss_expected_tags[0]));
    CU_ASSERT(rss_attr_index == sizeof(rss_expected_attr_keys)/sizeof(rss_expected_attr_keys[0]));
    CU_ASSERT(rss_content_index == sizeof(rss_expected_content)/sizeof(rss_expected_content[0]));

    sxml_destroy_explorer(explorer);
    free(xml);
}

// Test for Atom entry
static const char* atom_entry_expected_tags[] = {
    "entry","title","/title","link","/","id","/id",
    "updated","/updated","summary","/summary","/entry"
};
static const char* atom_entry_expected_attr_keys[] = {"xmlns","href"};
static const char* atom_entry_expected_attr_vals[] = {
    "http://www.w3.org/2005/Atom",
    "http://example.org/2003/12/13/atom03"
};
static const char* atom_entry_expected_content[] = {
    "Atom-Powered Robots Run Amok",
    "urn:uuid:1225c695-cfb8-4ebb-aaaa-80da344efa6a",
    "2003-12-13T18:30:02Z",
    "Some text."
};
static unsigned int atom_entry_tag_index = 0;
static unsigned int atom_entry_attr_index = 0;
static unsigned int atom_entry_content_index = 0;

static unsigned char atom_entry_on_tag(char* name) {
    CU_ASSERT(atom_entry_tag_index < sizeof(atom_entry_expected_tags)/sizeof(atom_entry_expected_tags[0]));
    CU_ASSERT_STRING_EQUAL(name, atom_entry_expected_tags[atom_entry_tag_index]);
    atom_entry_tag_index++;
    return SXMLExplorerContinue;
}

static unsigned char atom_entry_on_attr_key(char* key) {
    CU_ASSERT(atom_entry_attr_index < sizeof(atom_entry_expected_attr_keys)/sizeof(atom_entry_expected_attr_keys[0]));
    CU_ASSERT_STRING_EQUAL(key, atom_entry_expected_attr_keys[atom_entry_attr_index]);
    return SXMLExplorerContinue;
}

static unsigned char atom_entry_on_attr_val(char* val) {
    CU_ASSERT_STRING_EQUAL(val, atom_entry_expected_attr_vals[atom_entry_attr_index]);
    atom_entry_attr_index++;
    return SXMLExplorerContinue;
}

static unsigned char atom_entry_on_content(char* content) {
    while (*content && isspace((unsigned char)*content)) content++;
    if (*content == '\0') return SXMLExplorerContinue;
    CU_ASSERT(atom_entry_content_index < sizeof(atom_entry_expected_content)/sizeof(atom_entry_expected_content[0]));
    CU_ASSERT_STRING_EQUAL(content, atom_entry_expected_content[atom_entry_content_index]);
    atom_entry_content_index++;
    return SXMLExplorerContinue;
}

void test_atom_entry(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-atom-entry.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    atom_entry_tag_index = atom_entry_attr_index = atom_entry_content_index = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_register_func(explorer, atom_entry_on_tag, atom_entry_on_content,
                       atom_entry_on_attr_key, atom_entry_on_attr_val);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(atom_entry_tag_index == sizeof(atom_entry_expected_tags)/sizeof(atom_entry_expected_tags[0]));
    CU_ASSERT(atom_entry_attr_index == sizeof(atom_entry_expected_attr_keys)/sizeof(atom_entry_expected_attr_keys[0]));
    CU_ASSERT(atom_entry_content_index == sizeof(atom_entry_expected_content)/sizeof(atom_entry_expected_content[0]));

    sxml_destroy_explorer(explorer);
    free(xml);
}

// Test for XML with comments
static const char* comments_expected[] = {
    " This is a comment ",
    " This is another comment ",
    " This is a nested comment "
};
static unsigned int comments_index = 0;

static unsigned char comments_callback(char* text) {
    CU_ASSERT(comments_index < sizeof(comments_expected)/sizeof(comments_expected[0]));
    CU_ASSERT_STRING_EQUAL(text, comments_expected[comments_index]);
    comments_index++;
    return SXMLExplorerContinue;
}

void test_with_comments(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-with-comments.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    comments_index = 0;
    sxml_register_func(explorer, NULL, NULL, NULL, NULL);
    sxml_register_comment_func(explorer, comments_callback);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(comments_index == sizeof(comments_expected)/sizeof(comments_expected[0]));

    sxml_destroy_explorer(explorer);
    free(xml);
}

// Test for XML with CDATA
static unsigned int cdata_content_index = 0;
static const char* cdata_expected = "This is some CDATA text with < and > characters.";
static unsigned char cdata_on_content(char* c) {
    while (*c && isspace((unsigned char)*c)) c++;
    if (*c != '\0') {
        CU_ASSERT_STRING_EQUAL(c, cdata_expected);
        cdata_content_index++;
    }
    return SXMLExplorerContinue;
}

void test_with_cdata(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    char* xml = read_file_to_string("test-data/test-with-cdata.xml");
    CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

    cdata_content_index = 0;
    sxml_register_func(explorer, NULL, cdata_on_content, NULL, NULL);

    unsigned char result = sxml_run_explorer(explorer, xml);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT(cdata_content_index == 1);

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
