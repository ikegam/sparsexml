#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sparsexml.h"
#include "test-log.h"

/*
 * EXI tests
 *
 * The .exi files in test-data are produced by EXIficient (the W3C EXI
 * reference implementation) from the .xml files, schema-less, with the
 * options listed in test-data/Makefile ("make -C test-data").
 *
 * Decoding an .exi file must produce the same events as parsing the
 * original XML. Namespace declarations and whitespace-only text are not
 * compared. Prefixes are removed from names (namespace processing), because
 * EXI does not keep them unless asked to; with SXML_EXI_PRESERVE_PREFIXES
 * the qualified names are compared as well. EXIficient drops
 * xsi:schemaLocation and joins the text around comments it does not
 * preserve, so the comparison does too (TEST_LOG_CANONICAL).
 */

static TestLog xml_log, exi_log;

static unsigned char* read_file(const char* filename, unsigned int* size) {
  FILE* f = fopen(filename, "rb");
  long file_size;
  unsigned char* buffer;

  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  file_size = ftell(f);
  rewind(f);
  buffer = (unsigned char*)malloc(file_size + 1);
  if (buffer) {
    *size = (unsigned int)fread(buffer, 1, file_size, f);
    buffer[*size] = '\0';
  }
  fclose(f);
  return buffer;
}

// Record into log in the form used to compare XML with EXI
static void attach_log(SXMLExplorer* explorer, TestLog* log, unsigned char comments, unsigned char namespaces) {
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, namespaces);
  test_log_attach(explorer, log, TEST_LOG_CANONICAL |
                  (comments ? TEST_LOG_COMMENTS : 0) |
                  (namespaces ? TEST_LOG_LOCAL_KEYS : 0));
}

static void parse_xml_file(const char* filename, unsigned char comments, unsigned char namespaces) {
  unsigned int size;
  char* xml = (char*)read_file(filename, &size);
  CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

  SXMLExplorer* explorer = sxml_make_explorer();
  attach_log(explorer, &xml_log, comments, namespaces);
  CU_ASSERT_EQUAL(sxml_run_explorer(explorer, xml), SXMLExplorerComplete);
  sxml_destroy_explorer(explorer);
  free(xml);
}

static unsigned char parse_exi_file(const char* filename, unsigned char options,
                                    unsigned char comments, unsigned char namespaces) {
  unsigned int size;
  unsigned char* exi = read_file(filename, &size);
  unsigned char result;
  CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

  SXMLExplorer* explorer = sxml_make_exi_explorer();
  attach_log(explorer, &exi_log, comments, namespaces);
  sxml_set_exi_options(explorer, options);
  result = sxml_run_explorer_exi(explorer, exi, size);
  sxml_destroy_explorer(explorer);
  free(exi);
  return result;
}

static void assert_logs_equal(void) {
  unsigned int i;
  CU_ASSERT_EQUAL(exi_log.count, xml_log.count);
  for (i = 0; i < xml_log.count && i < exi_log.count; i++) {
    if (strcmp(xml_log.events[i], exi_log.events[i]) != 0) {
      printf("\n  event %u differs: XML '%s' EXI '%s'\n", i, xml_log.events[i], exi_log.events[i]);
      CU_FAIL("EXI events differ from XML events");
      return;
    }
  }
}

static void check_exi_matches_xml_names(const char* base, const char* exi_file, unsigned char options,
                                        unsigned char namespaces) {
  char xml_file[128];
  unsigned char comments = (options & SXML_EXI_PRESERVE_COMMENTS) != 0;

  snprintf(xml_file, sizeof(xml_file), "test-data/%s.xml", base);
  parse_xml_file(xml_file, comments, namespaces);
  CU_ASSERT_EQUAL(parse_exi_file(exi_file, options, comments, namespaces), SXMLExplorerComplete);
  CU_ASSERT(exi_log.count > 0);
  assert_logs_equal();
}

// Names compared with namespace processing, i.e. without prefixes
static void check_exi_matches_xml(const char* base, const char* exi_file, unsigned char options) {
  check_exi_matches_xml_names(base, exi_file, options, 1);
}

// =============================================================================
// Real-world documents
// =============================================================================

void test_exi_atom_feed(void) {
  check_exi_matches_xml("test-oss-1", "test-data/test-oss-1.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "T[feed]");
  CU_ASSERT_STRING_EQUAL(exi_log.events[1], "T[title]");
  CU_ASSERT_STRING_EQUAL(exi_log.events[2], "C[Example Feed]");
}

void test_exi_rss_feed(void) {
  check_exi_matches_xml("test-rss", "test-data/test-rss.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "T[rss]");
  CU_ASSERT_STRING_EQUAL(exi_log.events[1], "K[version]");
  CU_ASSERT_STRING_EQUAL(exi_log.events[2], "V[2.0]");
}

void test_exi_atom_entry(void) {
  check_exi_matches_xml("test-atom-entry", "test-data/test-atom-entry.exi", 0);
}

void test_exi_sitemap(void) {
  check_exi_matches_xml("test-sitemap", "test-data/test-sitemap.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "T[urlset]");
  CU_ASSERT_STRING_EQUAL(exi_log.events[exi_log.count - 1], "T[/urlset]");
}

void test_exi_large_document(void) {
  check_exi_matches_xml("test-large-document", "test-data/test-large-document.exi", 0);
  CU_ASSERT(exi_log.count > 300);
}

void test_exi_cdata(void) {
  check_exi_matches_xml("test-with-cdata", "test-data/test-with-cdata.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[2], "C[This is some CDATA text with < and > characters.]");
}

// =============================================================================
// Fidelity options
// =============================================================================

void test_exi_comments_not_preserved(void) {
  check_exi_matches_xml("test-with-comments", "test-data/test-with-comments.exi", 0);
}

void test_exi_comments_preserved(void) {
  check_exi_matches_xml("test-with-comments", "test-data/test-with-comments-preserved.exi",
                        SXML_EXI_PRESERVE_COMMENTS);
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "M[ This is a comment ]");
}

void test_exi_features(void) {
  check_exi_matches_xml("test-features", "test-data/test-features.exi", 0);
}

void test_exi_features_fidelity(void) {
  check_exi_matches_xml("test-features", "test-data/test-features-fidelity.exi",
                        SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS |
                        SXML_EXI_PRESERVE_DTD | SXML_EXI_PRESERVE_PREFIXES);
  CU_ASSERT_STRING_EQUAL(exi_log.events[exi_log.count - 1], "M[ trailing comment ]");
}

void test_exi_features_byte_aligned(void) {
  check_exi_matches_xml("test-features", "test-data/test-features-bytealigned.exi",
                        SXML_EXI_BYTE_ALIGNED);
  check_exi_matches_xml("test-features", "test-data/test-features-fidelity-bytealigned.exi",
                        SXML_EXI_BYTE_ALIGNED | SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS |
                        SXML_EXI_PRESERVE_DTD | SXML_EXI_PRESERVE_PREFIXES);
}

// With prefixes preserved, qualified names match the XML without namespace processing
void test_exi_qualified_names(void) {
  check_exi_matches_xml_names("test-features", "test-data/test-features-fidelity.exi",
                              SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS |
                              SXML_EXI_PRESERVE_DTD | SXML_EXI_PRESERVE_PREFIXES, 0);
  check_exi_matches_xml_names("test-features", "test-data/test-features-fidelity-bytealigned.exi",
                              SXML_EXI_BYTE_ALIGNED | SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS |
                              SXML_EXI_PRESERVE_DTD | SXML_EXI_PRESERVE_PREFIXES, 0);

  CU_ASSERT(test_log_find(&exi_log, "T[inv:inventory]") >= 0);
  CU_ASSERT(test_log_find(&exi_log, "K[xml:lang]") >= 0);
  CU_ASSERT(test_log_find(&exi_log, "K[x:at]") >= 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[exi_log.count - 2], "T[/inv:inventory]");
}

void test_exi_xsi_type(void) {
  static const char* expected[] = {
    "T[values]", "T[value]", "K[type]", "V[Celsius]", "C[21.5]", "T[/value]",
    "T[value]", "K[type]", "V[Celsius]", "C[22.0]", "T[/value]", "T[/values]"
  };
  unsigned int i;

  CU_ASSERT_EQUAL(parse_exi_file("test-data/test-xsi-type.exi", 0, 0, 1), SXMLExplorerComplete);
  CU_ASSERT_EQUAL_FATAL(exi_log.count, sizeof(expected) / sizeof(expected[0]));
  for (i = 0; i < exi_log.count; i++) {
    CU_ASSERT_STRING_EQUAL(exi_log.events[i], expected[i]);
  }
}

// Wrong out-of-band options must not be mistaken for a valid document
void test_exi_option_mismatch(void) {
  CU_ASSERT_NOT_EQUAL(parse_exi_file("test-data/test-features-fidelity.exi", 0, 0, 1), SXMLExplorerComplete);
}

// =============================================================================
// Errors and limits
// =============================================================================

void test_exi_invalid_headers(void) {
  unsigned char xml_data[] = {'<', 'r', 'o', 'o', 't', '>'};
  unsigned char bad_cookie[] = {'$', 'E', 'X', 'X', 0x80};
  unsigned char short_cookie[] = {'$', 'E'};
  unsigned char preview_version[] = {0x90, 0x00};
  unsigned char unknown_version[] = {0x81, 0x00};
  SXMLExplorer* explorer = sxml_make_exi_explorer();
  attach_log(explorer, &exi_log, 0, 1);

  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, xml_data, sizeof(xml_data)), SXMLExplorerErrorMalformedXML);
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, bad_cookie, sizeof(bad_cookie)), SXMLExplorerErrorMalformedXML);
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, short_cookie, sizeof(short_cookie)), SXMLExplorerErrorMalformedXML);
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, preview_version, sizeof(preview_version)), SXMLExplorerErrorUnsupported);
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, unknown_version, sizeof(unknown_version)), SXMLExplorerErrorUnsupported);
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, NULL, 0), SXMLExplorerErrorMalformedXML);
  CU_ASSERT_EQUAL(exi_log.count, 0);

  sxml_destroy_explorer(explorer);
}

void test_exi_header_options_unsupported(void) {
  CU_ASSERT_EQUAL(parse_exi_file("test-data/test-features-header-options.exi", 0, 0, 1), SXMLExplorerErrorUnsupported);
}

// Every truncated stream must be reported as malformed
void test_exi_truncated(void) {
  static const char* files[] = {
    "test-data/test-oss-1.exi", "test-data/test-rss.exi", "test-data/test-sitemap.exi",
    "test-data/test-features.exi", "test-data/test-large-document.exi"
  };
  unsigned int f, len, size;

  for (f = 0; f < sizeof(files) / sizeof(files[0]); f++) {
    unsigned char* exi = read_file(files[f], &size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);
    SXMLExplorer* explorer = sxml_make_exi_explorer();
    attach_log(explorer, &exi_log, 0, 1);
    for (len = 1; len < size; len++) {
      exi_log.count = 0;
      CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, exi, len), SXMLExplorerErrorMalformedXML);
    }
    sxml_destroy_explorer(explorer);
    free(exi);
  }
}

static unsigned char stop_on_third_tag(SXMLExplorer* explorer, char* name) {
  unsigned int* count = sxml_get_user_data(explorer);
  return (++*count == 3) ? SXMLExplorerStop : SXMLExplorerContinue;
}

void test_exi_stop_from_callback(void) {
  unsigned int size;
  unsigned char* exi = read_file("test-data/test-sitemap.exi", &size);
  CU_ASSERT_PTR_NOT_NULL_FATAL(exi);
  SXMLExplorer* explorer = sxml_make_exi_explorer();

  unsigned int stop_count = 0;
  sxml_set_user_data(explorer, &stop_count);
  sxml_register_func(explorer, stop_on_third_tag, NULL, NULL, NULL);
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, exi, size), SXMLExplorerInterrupted);
  CU_ASSERT_EQUAL(stop_count, 3);

  sxml_destroy_explorer(explorer);
  free(exi);
}

void test_exi_caller_workspace(void) {
  static unsigned char workspace[4096];
  static unsigned char tiny_workspace[256];
  unsigned int size;
  unsigned char* exi = read_file("test-data/test-sitemap.exi", &size);
  CU_ASSERT_PTR_NOT_NULL_FATAL(exi);
  SXMLExplorer* explorer = sxml_make_exi_explorer();
  attach_log(explorer, &exi_log, 0, 1);

  sxml_set_exi_workspace(explorer, workspace, sizeof(workspace));
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, exi, size), SXMLExplorerComplete);
  CU_ASSERT_EQUAL(exi_log.count, 16);  // 12 tags and 4 texts

  sxml_set_exi_workspace(explorer, tiny_workspace, sizeof(tiny_workspace));
  CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, exi, size), SXMLExplorerErrorBufferOverflow);

  sxml_destroy_explorer(explorer);
  free(exi);
}

void add_exi_tests(CU_pSuite* suite) {
  CU_add_test(*suite, "EXI Atom feed matches XML", test_exi_atom_feed);
  CU_add_test(*suite, "EXI RSS feed matches XML", test_exi_rss_feed);
  CU_add_test(*suite, "EXI Atom entry matches XML", test_exi_atom_entry);
  CU_add_test(*suite, "EXI sitemap matches XML", test_exi_sitemap);
  CU_add_test(*suite, "EXI large document matches XML", test_exi_large_document);
  CU_add_test(*suite, "EXI CDATA matches XML", test_exi_cdata);
  CU_add_test(*suite, "EXI comments not preserved", test_exi_comments_not_preserved);
  CU_add_test(*suite, "EXI comments preserved", test_exi_comments_preserved);
  CU_add_test(*suite, "EXI features (default options)", test_exi_features);
  CU_add_test(*suite, "EXI features (comments, PIs, DTD, prefixes)", test_exi_features_fidelity);
  CU_add_test(*suite, "EXI features (byte-aligned)", test_exi_features_byte_aligned);
  CU_add_test(*suite, "EXI qualified names with prefixes", test_exi_qualified_names);
  CU_add_test(*suite, "EXI xsi:type", test_exi_xsi_type);
  CU_add_test(*suite, "EXI option mismatch detected", test_exi_option_mismatch);
  CU_add_test(*suite, "EXI invalid headers", test_exi_invalid_headers);
  CU_add_test(*suite, "EXI header options unsupported", test_exi_header_options_unsupported);
  CU_add_test(*suite, "EXI truncated streams", test_exi_truncated);
  CU_add_test(*suite, "EXI stop from callback", test_exi_stop_from_callback);
  CU_add_test(*suite, "EXI caller-provided workspace", test_exi_caller_workspace);
}
