#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sparsexml.h"

/*
 * EXI tests
 *
 * The .exi files in test-data were produced by EXIficient (the W3C EXI
 * reference implementation) from the .xml file with the same base name,
 * schema-less, e.g.:
 *
 *   EXIficientCMD -encode -noSchema -i test-features.xml -o test-features.exi
 *   ... -preserveComments -preservePIs -preserveDTDs -preservePrefixes
 *       -> test-features-fidelity.exi
 *   ... -bytePacked -> test-features-bytealigned.exi
 *   ... -includeOptions -includeCookie -> test-features-header-options.exi
 *
 * Decoding an .exi file must produce the same events as parsing the
 * original XML. Namespace declarations and whitespace-only text are not
 * compared, and prefixes are removed from names, because EXI does not keep
 * them unless asked to. EXIficient drops xsi:schemaLocation and joins the
 * text around comments it does not preserve, so the comparison does too.
 */

#define MAX_EVENTS 512
#define MAX_EVENT_LENGTH 128

typedef struct {
  char events[MAX_EVENTS][MAX_EVENT_LENGTH];
  unsigned int count;
  unsigned char skip_value;
  unsigned char record_comments;
} EventLog;

static EventLog xml_log, exi_log;
static EventLog* current_log;

static void log_event(char kind, const char* text) {
  if (current_log->count < MAX_EVENTS) {
    snprintf(current_log->events[current_log->count++], MAX_EVENT_LENGTH, "%c:%s", kind, text);
  }
}

static const char* local_part(const char* name) {
  const char* colon = strchr(name, ':');
  return colon ? colon + 1 : name;
}

static unsigned char log_tag(char* name) {
  log_event('T', name);
  return SXMLExplorerContinue;
}

static unsigned char log_content(char* content) {
  const char* p = content;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  if (*p != '\0') {
    char* last = current_log->count ? current_log->events[current_log->count - 1] : NULL;
    if (last != NULL && last[0] == 'C') {
      // Adjacent text, e.g. around a comment that is not recorded
      strncat(last, content, MAX_EVENT_LENGTH - strlen(last) - 1);
    } else {
      log_event('C', content);
    }
  }
  return SXMLExplorerContinue;
}

static unsigned char log_attribute_key(char* key) {
  current_log->skip_value = (strncmp(key, "xmlns", 5) == 0 ||
                             strcmp(key, "xsi:schemaLocation") == 0);
  if (!current_log->skip_value) {
    log_event('K', local_part(key));
  }
  return SXMLExplorerContinue;
}

static unsigned char log_attribute_value(char* value) {
  if (!current_log->skip_value) {
    log_event('V', value);
  }
  current_log->skip_value = 0;
  return SXMLExplorerContinue;
}

static unsigned char log_comment(char* comment) {
  if (current_log->record_comments) {
    log_event('M', comment);
  }
  return SXMLExplorerContinue;
}

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

static SXMLExplorer* make_logging_explorer(EventLog* log, unsigned char record_comments) {
  SXMLExplorer* explorer = sxml_make_explorer();
  memset(log, 0, sizeof(EventLog));
  log->record_comments = record_comments;
  current_log = log;
  sxml_enable_entity_processing(explorer, 1);
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, log_tag, log_content, log_attribute_key, log_attribute_value);
  sxml_register_comment_func(explorer, log_comment);
  return explorer;
}

static void parse_xml_file(const char* filename, unsigned char record_comments) {
  unsigned int size;
  char* xml = (char*)read_file(filename, &size);
  CU_ASSERT_PTR_NOT_NULL_FATAL(xml);

  SXMLExplorer* explorer = make_logging_explorer(&xml_log, record_comments);
  CU_ASSERT_EQUAL(sxml_run_explorer(explorer, xml), SXMLExplorerComplete);
  sxml_destroy_explorer(explorer);
  free(xml);
}

static unsigned char parse_exi_file(const char* filename, unsigned char options, unsigned char record_comments) {
  unsigned int size;
  unsigned char* exi = read_file(filename, &size);
  unsigned char result;
  CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

  SXMLExplorer* explorer = make_logging_explorer(&exi_log, record_comments);
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

static void check_exi_matches_xml(const char* base, const char* exi_file, unsigned char options) {
  char xml_file[128];
  unsigned char comments = (options & SXML_EXI_PRESERVE_COMMENTS) != 0;

  snprintf(xml_file, sizeof(xml_file), "test-data/%s.xml", base);
  parse_xml_file(xml_file, comments);
  CU_ASSERT_EQUAL(parse_exi_file(exi_file, options, comments), SXMLExplorerComplete);
  CU_ASSERT(exi_log.count > 0);
  assert_logs_equal();
}

// =============================================================================
// Real-world documents
// =============================================================================

void test_exi_atom_feed(void) {
  check_exi_matches_xml("test-oss-1", "test-data/test-oss-1.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "T:feed");
  CU_ASSERT_STRING_EQUAL(exi_log.events[1], "T:title");
  CU_ASSERT_STRING_EQUAL(exi_log.events[2], "C:Example Feed");
}

void test_exi_rss_feed(void) {
  check_exi_matches_xml("test-rss", "test-data/test-rss.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "T:rss");
  CU_ASSERT_STRING_EQUAL(exi_log.events[1], "K:version");
  CU_ASSERT_STRING_EQUAL(exi_log.events[2], "V:2.0");
}

void test_exi_atom_entry(void) {
  check_exi_matches_xml("test-atom-entry", "test-data/test-atom-entry.exi", 0);
}

void test_exi_sitemap(void) {
  check_exi_matches_xml("test-sitemap", "test-data/test-sitemap.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "T:urlset");
  CU_ASSERT_STRING_EQUAL(exi_log.events[exi_log.count - 1], "T:/urlset");
}

void test_exi_large_document(void) {
  check_exi_matches_xml("test-large-document", "test-data/test-large-document.exi", 0);
  CU_ASSERT(exi_log.count > 300);
}

void test_exi_cdata(void) {
  check_exi_matches_xml("test-with-cdata", "test-data/test-with-cdata.exi", 0);
  CU_ASSERT_STRING_EQUAL(exi_log.events[2], "C:This is some CDATA text with < and > characters.");
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
  CU_ASSERT_STRING_EQUAL(exi_log.events[0], "M: This is a comment ");
}

void test_exi_features(void) {
  check_exi_matches_xml("test-features", "test-data/test-features.exi", 0);
}

void test_exi_features_fidelity(void) {
  check_exi_matches_xml("test-features", "test-data/test-features-fidelity.exi",
                        SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS |
                        SXML_EXI_PRESERVE_DTD | SXML_EXI_PRESERVE_PREFIXES);
  CU_ASSERT_STRING_EQUAL(exi_log.events[exi_log.count - 1], "M: trailing comment ");
}

void test_exi_features_byte_aligned(void) {
  check_exi_matches_xml("test-features", "test-data/test-features-bytealigned.exi",
                        SXML_EXI_BYTE_ALIGNED);
  check_exi_matches_xml("test-features", "test-data/test-features-fidelity-bytealigned.exi",
                        SXML_EXI_BYTE_ALIGNED | SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_PRESERVE_PIS |
                        SXML_EXI_PRESERVE_DTD | SXML_EXI_PRESERVE_PREFIXES);
}

void test_exi_xsi_type(void) {
  static const char* expected[] = {
    "T:values", "T:value", "K:type", "V:Celsius", "C:21.5", "T:/value",
    "T:value", "K:type", "V:Celsius", "C:22.0", "T:/value", "T:/values"
  };
  unsigned int i;

  CU_ASSERT_EQUAL(parse_exi_file("test-data/test-xsi-type.exi", 0, 0), SXMLExplorerComplete);
  CU_ASSERT_EQUAL_FATAL(exi_log.count, sizeof(expected) / sizeof(expected[0]));
  for (i = 0; i < exi_log.count; i++) {
    CU_ASSERT_STRING_EQUAL(exi_log.events[i], expected[i]);
  }
}

// Wrong out-of-band options must not be mistaken for a valid document
void test_exi_option_mismatch(void) {
  CU_ASSERT_NOT_EQUAL(parse_exi_file("test-data/test-features-fidelity.exi", 0, 0), SXMLExplorerComplete);
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
  SXMLExplorer* explorer = make_logging_explorer(&exi_log, 0);

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
  CU_ASSERT_EQUAL(parse_exi_file("test-data/test-features-header-options.exi", 0, 0), SXMLExplorerErrorUnsupported);
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
    SXMLExplorer* explorer = make_logging_explorer(&exi_log, 0);
    for (len = 1; len < size; len++) {
      exi_log.count = 0;
      CU_ASSERT_EQUAL(sxml_run_explorer_exi(explorer, exi, len), SXMLExplorerErrorMalformedXML);
    }
    sxml_destroy_explorer(explorer);
    free(exi);
  }
}

static unsigned int stop_count = 0;
static unsigned char stop_on_third_tag(char* name) {
  return (++stop_count == 3) ? SXMLExplorerStop : SXMLExplorerContinue;
}

void test_exi_stop_from_callback(void) {
  unsigned int size;
  unsigned char* exi = read_file("test-data/test-sitemap.exi", &size);
  CU_ASSERT_PTR_NOT_NULL_FATAL(exi);
  SXMLExplorer* explorer = sxml_make_explorer();

  stop_count = 0;
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
  SXMLExplorer* explorer = make_logging_explorer(&exi_log, 0);

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
  CU_add_test(*suite, "EXI xsi:type", test_exi_xsi_type);
  CU_add_test(*suite, "EXI option mismatch detected", test_exi_option_mismatch);
  CU_add_test(*suite, "EXI invalid headers", test_exi_invalid_headers);
  CU_add_test(*suite, "EXI header options unsupported", test_exi_header_options_unsupported);
  CU_add_test(*suite, "EXI truncated streams", test_exi_truncated);
  CU_add_test(*suite, "EXI stop from callback", test_exi_stop_from_callback);
  CU_add_test(*suite, "EXI caller-provided workspace", test_exi_caller_workspace);
}
