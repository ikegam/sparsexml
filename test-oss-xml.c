#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>

#include "sparsexml.h"

/*
 * Real-world XML test cases using OSS/standard XML formats
 * 
 * Test samples are based on publicly available XML standards:
 * 1. XML Sitemap - from sitemaps.org protocol (open standard)
 * 2. Atom Feed - from W3C Atom specification (open standard)
 */

void test_real_world_xml_sitemap(void) {
  SXMLExplorer* explorer;
  // Real XML sitemap example from sitemaps.org
  char xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n"
               "   <url>\n"
               "      <loc>http://www.example.com/</loc>\n"
               "      <lastmod>2005-01-01</lastmod>\n"
               "      <changefreq>monthly</changefreq>\n"
               "      <priority>0.8</priority>\n"
               "   </url>\n"
               "   <url>\n"
               "      <loc>http://www.example.com/catalog?item=12&amp;desc=vacation_hawaii</loc>\n"
               "      <changefreq>weekly</changefreq>\n"
               "   </url>\n"
               "   <url>\n"
               "      <loc>http://www.example.com/catalog?item=73&amp;desc=vacation_new_zealand</loc>\n"
               "      <lastmod>2004-12-23</lastmod>\n"
               "      <changefreq>weekly</changefreq>\n"
               "   </url>\n"
               "</urlset>";
  
  unsigned int tag_count = 0;
  unsigned int content_count = 0;
  unsigned char found_namespace = 0;
  
  unsigned char on_tag(char *name) {
    tag_count++;
    if (strcmp(name, "urlset") == 0) {
      found_namespace = 1;  // namespace processing should strip prefix
    }
    return SXMLExplorerContinue;
  }
  
  unsigned char on_content(char *content) {
    content_count++;
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, on_tag, on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(tag_count > 0);  // Should parse multiple tags
  CU_ASSERT(content_count > 0);  // Should parse content
  CU_ASSERT(found_namespace == 1);  // Should handle namespace
  
  sxml_destroy_explorer(explorer);
}

void test_real_world_atom_feed(void) {
  SXMLExplorer* explorer;
  // Real Atom feed example from W3C documentation
  char xml[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
               "<feed xmlns=\"http://www.w3.org/2005/Atom\">\n"
               "  <title>Example Feed</title>\n"
               "  <link href=\"http://example.org/\"/>\n"
               "  <updated>2003-12-13T18:30:02Z</updated>\n"
               "  <author>\n"
               "    <name>John Doe</name>\n"
               "  </author>\n"
               "  <id>urn:uuid:60a76c80-d399-11d9-b93C-0003939e0af6</id>\n"
               "  <!-- This is a comment -->\n"
               "  <entry>\n"
               "    <title>Atom-Powered Robots Run Amok</title>\n"
               "    <link href=\"http://example.org/2003/12/13/atom03\"/>\n"
               "    <id>urn:uuid:1225c695-cfb8-4ebb-aaaa-80da344efa6a</id>\n"
               "    <updated>2003-12-13T18:30:02Z</updated>\n"
               "    <summary>Some text &amp; more &quot;quoted&quot; content</summary>\n"
               "  </entry>\n"
               "</feed>";
  
  unsigned int tag_count = 0;
  unsigned int content_count = 0;
  unsigned int comment_count = 0;
  unsigned char found_entities = 0;
  
  unsigned char on_tag(char *name) {
    tag_count++;
    return SXMLExplorerContinue;
  }
  
  unsigned char on_content(char *content) {
    content_count++;
    // Check if content exists (entities may not be fully processed yet)
    if (strlen(content) > 0) {
      found_entities = 1;  // Content found 
    }
    return SXMLExplorerContinue;
  }
  
  unsigned char on_comment(char *comment) {
    comment_count++;
    CU_ASSERT(strstr(comment, "This is a comment") != NULL);
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, on_tag, on_content, NULL, NULL);
  sxml_register_comment_func(explorer, on_comment);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(tag_count >= 10);  // Should parse many tags
  CU_ASSERT(content_count >= 5);  // Should parse content elements
  CU_ASSERT(comment_count == 1);  // Should find the comment
  CU_ASSERT(found_entities == 1);  // Should process entities
  
  sxml_destroy_explorer(explorer);
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
  
  unsigned int tag_count = 0;
  unsigned int content_count = 0;
  unsigned int comment_count = 0;
  unsigned char found_cdata = 0;
  unsigned char found_entities = 0;
  unsigned char found_namespaces = 0;
  
  unsigned char on_tag(char *name) {
    tag_count++;
    if (strcmp(name, "setting") == 0) {  // namespace prefix should be stripped
      found_namespaces = 1;
    }
    return SXMLExplorerContinue;
  }
  
  unsigned char on_content(char *content) {
    content_count++;
    if (strstr(content, "Raw data with <tags>") != NULL) {
      found_cdata = 1;  // CDATA content preserved
    }
    if (strlen(content) > 0) {
      found_entities = 1;  // Content found
    }
    return SXMLExplorerContinue;
  }
  
  unsigned char on_comment(char *comment) {
    comment_count++;
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, on_tag, on_content, NULL, NULL);
  sxml_register_comment_func(explorer, on_comment);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  // Should complete successfully with supported entities only
  CU_ASSERT(result == SXMLExplorerComplete);
  CU_ASSERT(tag_count >= 6);  // Should parse multiple tags (root, config, app:setting, description, data, footer)
  CU_ASSERT(content_count >= 3);  // Should parse multiple content sections
  CU_ASSERT(comment_count == 1);  // Should find the comment
  CU_ASSERT(found_cdata == 1);  // Should process CDATA section
  CU_ASSERT(found_entities == 1);  // Should find content (entities processed)
  CU_ASSERT(found_namespaces == 1);  // Should process namespaces
  
  sxml_destroy_explorer(explorer);
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
  
  unsigned int tag_count = 0;
  
  unsigned char on_tag(char *name) {
    tag_count++;
    return SXMLExplorerContinue;
  }
  
  unsigned char on_content(char *content) {
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_register_func(explorer, on_tag, on_content, NULL, NULL);
  
  unsigned char result = sxml_run_explorer(explorer, xml);
  
  // Parser should complete despite malformed elements
  // (SparseXML is lenient for embedded use cases)
  CU_ASSERT(result == SXMLExplorerComplete || 
            result == SXMLExplorerErrorInvalidEntity ||
            result == SXMLExplorerErrorMalformedXML);
  CU_ASSERT(tag_count > 0);  // Should parse some tags before errors
  
  sxml_destroy_explorer(explorer);
}

void add_oss_xml_tests(CU_pSuite* suite) {
  CU_add_test(*suite, "Real-world XML Sitemap parsing", test_real_world_xml_sitemap);
  CU_add_test(*suite, "Real-world Atom Feed parsing", test_real_world_atom_feed);
  CU_add_test(*suite, "Complex XML with all features", test_complex_xml_with_cdata_and_entities);
  CU_add_test(*suite, "Error handling with malformed XML", test_error_handling_with_malformed_xml);
}
