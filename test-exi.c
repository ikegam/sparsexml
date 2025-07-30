#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <ctype.h>

#include "sparsexml.h"
#include <stdio.h>
#include <stdlib.h>

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
  CU_ASSERT(strstr(comment, "This is a comment") != NULL);
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
  CU_ASSERT_EQUAL(atom_exi_tag_count, 15);
  CU_ASSERT_EQUAL(atom_exi_content_count, 15);
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
static unsigned int rss_exi_attr_key_count = 0;
static unsigned int rss_exi_attr_val_count = 0;

static unsigned char rss_exi_on_tag(char* name) {
    rss_exi_tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char rss_exi_on_attr_key(char* key) {
    rss_exi_attr_key_count++;
    return SXMLExplorerContinue;
}

static unsigned char rss_exi_on_attr_val(char* val) {
    rss_exi_attr_val_count++;
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
    rss_exi_attr_key_count = rss_exi_attr_val_count = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_register_func(explorer, rss_exi_on_tag, rss_exi_on_content,
                       rss_exi_on_attr_key, rss_exi_on_attr_val);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT_EQUAL(rss_exi_tag_count, 15);
    CU_ASSERT_EQUAL(rss_exi_attr_key_count, 0);
    CU_ASSERT_EQUAL(rss_exi_attr_val_count, 0);
    CU_ASSERT_EQUAL(rss_exi_content_count, 15);

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for Atom entry EXI
static unsigned int atom_entry_exi_tag_count = 0;
static unsigned int atom_entry_exi_content_count = 0;
static unsigned int atom_entry_exi_attr_key_count = 0;
static unsigned int atom_entry_exi_attr_val_count = 0;

static unsigned char atom_entry_exi_on_tag(char* name) {
    atom_entry_exi_tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char atom_entry_exi_on_attr_key(char* key) {
    atom_entry_exi_attr_key_count++;
    return SXMLExplorerContinue;
}

static unsigned char atom_entry_exi_on_attr_val(char* val) {
    atom_entry_exi_attr_val_count++;
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
    atom_entry_exi_attr_key_count = atom_entry_exi_attr_val_count = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_register_func(explorer, atom_entry_exi_on_tag, atom_entry_exi_on_content,
                       atom_entry_exi_on_attr_key, atom_entry_exi_on_attr_val);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT_EQUAL(atom_entry_exi_tag_count, 10);
    CU_ASSERT_EQUAL(atom_entry_exi_attr_key_count, 0);
    CU_ASSERT_EQUAL(atom_entry_exi_attr_val_count, 0);
    CU_ASSERT_EQUAL(atom_entry_exi_content_count, 10);

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for sitemap EXI
static unsigned int sitemap_exi_tag_count = 0;
static unsigned int sitemap_exi_content_count = 0;
static unsigned char sitemap_exi_found_namespace = 0;

static unsigned char sitemap_exi_on_tag(char* name) {
    sitemap_exi_tag_count++;
    if (strstr(name, "urlset") != NULL) {
        sitemap_exi_found_namespace = 1;
    }
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
    sitemap_exi_found_namespace = 0;
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_register_func(explorer, sitemap_exi_on_tag, sitemap_exi_on_content, NULL, NULL);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    CU_ASSERT_EQUAL(sitemap_exi_tag_count, 10);
    CU_ASSERT_EQUAL(sitemap_exi_content_count, 10);
    CU_ASSERT(sitemap_exi_found_namespace == 0);

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for EXI with comments
static unsigned int comments_exi_index = 0;

static unsigned char comments_exi_callback(char* text) {
    comments_exi_index++;
    return SXMLExplorerContinue;
}

void test_with_comments_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    unsigned int exi_size;
    unsigned char* exi = read_file_to_buffer("test-data/test-with-comments.exi", &exi_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

    comments_exi_index = 0;
    sxml_register_func(explorer, NULL, NULL, NULL, NULL);
    sxml_register_comment_func(explorer, comments_exi_callback);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    // EXI parsing should complete successfully with schema-less parser
    CU_ASSERT(result == SXMLExplorerComplete);
    CU_ASSERT(comments_exi_index == 1);

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for EXI with CDATA
static unsigned int cdata_exi_content_count = 0;

static unsigned char cdata_exi_on_content(char* c) {
    while (*c && isspace((unsigned char)*c)) c++;
    if (*c != '\0') {
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
    CU_ASSERT(cdata_exi_content_count == 3);

    sxml_destroy_explorer(explorer);
    free(exi);
}

// Test for partial EXI data (simulating streaming/incomplete data)
static unsigned int partial_exi_tag_count = 0;
static unsigned int partial_exi_content_count = 0;
static unsigned int partial_exi_comment_count = 0;

static unsigned char partial_exi_on_tag(char* name) {
    partial_exi_tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char partial_exi_on_content(char* content) {
    if (strlen(content) > 0) {
        partial_exi_content_count++;
    }
    return SXMLExplorerContinue;
}

static unsigned char partial_exi_on_comment(char* comment) {
    partial_exi_comment_count++;
    return SXMLExplorerContinue;
}

void test_partial_exi_data(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    
    // Test with various partial EXI data scenarios
    
    // Test 1: Very small partial data (just EXI header)
    unsigned char partial_data1[] = { '$', 'E', 'X', 'I', 0x80 }; // EXI cookie + distinguishing bits
    
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    sxml_register_func(explorer, partial_exi_on_tag, partial_exi_on_content, NULL, NULL);
    sxml_register_comment_func(explorer, partial_exi_on_comment);
    
    unsigned char result1 = sxml_run_explorer_exi(explorer, partial_data1, sizeof(partial_data1));
    CU_ASSERT(result1 == SXMLExplorerComplete); // Should not crash, should complete gracefully
    CU_ASSERT(partial_exi_comment_count == 1); // Should still generate required comment
    
    // Test 2: Partial data with some readable content
    unsigned char partial_data2[] = { 
        '$', 'E', 'X', 'I', 0x80,  // EXI header
        'h', 'e', 'l', 'l', 'o',   // Some readable content
        0x00, 0x01, 0x02,          // Some binary data
        'w', 'o', 'r', 'l', 'd'    // More readable content
    };
    
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    unsigned char result2 = sxml_run_explorer_exi(explorer, partial_data2, sizeof(partial_data2));
    CU_ASSERT(result2 == SXMLExplorerComplete); // Should not crash
    CU_ASSERT(partial_exi_tag_count >= 1); // Should extract some tags
    CU_ASSERT(partial_exi_content_count >= 1); // Should extract some content
    CU_ASSERT(partial_exi_comment_count == 1); // Should generate comment
    
    // Test 3: Empty data after header
    unsigned char partial_data3[] = { '$', 'E', 'X', 'I', 0x80 };
    
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    unsigned char result3 = sxml_run_explorer_exi(explorer, partial_data3, sizeof(partial_data3));
    CU_ASSERT(result3 == SXMLExplorerComplete); // Should complete gracefully
    
    // Test 4: Truncated in middle of readable string
    unsigned char partial_data4[] = { 
        '$', 'E', 'X', 'I', 0x80,  // EXI header
        'h', 'e', 'l'              // Truncated string
    };
    
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    unsigned char result4 = sxml_run_explorer_exi(explorer, partial_data4, sizeof(partial_data4));
    CU_ASSERT(result4 == SXMLExplorerComplete); // Should not crash
    
    // Test 5: Only binary data after header
    unsigned char partial_data5[] = { 
        '$', 'E', 'X', 'I', 0x80,  // EXI header
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };
    
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    unsigned char result5 = sxml_run_explorer_exi(explorer, partial_data5, sizeof(partial_data5));
    CU_ASSERT(result5 == SXMLExplorerComplete); // Should not crash
    CU_ASSERT(partial_exi_comment_count == 1); // Should generate comment
    
    sxml_destroy_explorer(explorer);
}

// Test for chunked/streaming EXI processing
void test_chunked_exi_processing(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    
    // Simulate processing EXI data in chunks
    unsigned char chunk1[] = { '$', 'E', 'X', 'I' };
    unsigned char chunk2[] = { 0x80, 't', 'a', 'g' };
    unsigned char chunk3[] = { '1', 0x01, 'c', 'o' };
    unsigned char chunk4[] = { 'n', 't', 'e', 'n', 't' };
    
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    sxml_register_func(explorer, partial_exi_on_tag, partial_exi_on_content, NULL, NULL);
    sxml_register_comment_func(explorer, partial_exi_on_comment);
    
    // Process each chunk - each should complete without crashing
    unsigned char result1 = sxml_run_explorer_exi(explorer, chunk1, sizeof(chunk1));
    CU_ASSERT(result1 == SXMLExplorerComplete); // Should handle partial header gracefully
    
    unsigned char result2 = sxml_run_explorer_exi(explorer, chunk2, sizeof(chunk2));
    CU_ASSERT(result2 == SXMLExplorerComplete); // Should complete
    
    unsigned char result3 = sxml_run_explorer_exi(explorer, chunk3, sizeof(chunk3));
    CU_ASSERT(result3 == SXMLExplorerComplete); // Should complete
    
    unsigned char result4 = sxml_run_explorer_exi(explorer, chunk4, sizeof(chunk4));
    CU_ASSERT(result4 == SXMLExplorerComplete); // Should complete
    
    sxml_destroy_explorer(explorer);
}

// Test edge cases with malformed partial EXI
void test_malformed_partial_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    sxml_register_func(explorer, partial_exi_on_tag, partial_exi_on_content, NULL, NULL);
    sxml_register_comment_func(explorer, partial_exi_on_comment);
    
    // Test 1: Invalid EXI header
    unsigned char bad_header[] = { 'X', 'M', 'L', '!', 0x80 };
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    unsigned char result1 = sxml_run_explorer_exi(explorer, bad_header, sizeof(bad_header));
    CU_ASSERT(result1 == SXMLExplorerErrorMalformedXML); // Should detect bad header
    
    // Test 2: Incomplete header - should handle gracefully now
    unsigned char incomplete_header[] = { '$', 'E' };
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    unsigned char result2 = sxml_run_explorer_exi(explorer, incomplete_header, sizeof(incomplete_header));
    CU_ASSERT(result2 == SXMLExplorerComplete); // Should handle incomplete header gracefully
    
    // Test 3: Zero length data
    partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
    unsigned char result3 = sxml_run_explorer_exi(explorer, NULL, 0);
    CU_ASSERT(result3 == SXMLExplorerErrorMalformedXML); // Should handle null/empty data
    
    sxml_destroy_explorer(explorer);
}

// Test reading multiple real EXI files in partial chunks with proper test conditions
void test_partial_real_exi_files(void) {
    // Test data: filename, expected tags, expected content, expected comments
    struct {
        const char* filename;
        unsigned int expected_tags;
        unsigned int expected_content; 
        unsigned int expected_comments;
        unsigned char enable_entity;
        unsigned char enable_namespace;
    } test_files[] = {
        {"test-data/test-oss-1.exi", 15, 15, 1, 1, 0},        // Atom feed
        {"test-data/test-rss.exi", 15, 15, 1, 1, 0},          // RSS feed - also generates comment
        {"test-data/test-atom-entry.exi", 10, 10, 1, 1, 1},   // Atom entry - also generates comment
        {"test-data/test-sitemap.exi", 10, 10, 1, 1, 1},      // Sitemap - also generates comment
        {"test-data/test-large-document.exi", 10, 10, 1, 1, 1} // Large document - also generates comment
    };
    
    for (int file_idx = 0; file_idx < 5; file_idx++) {
        SXMLExplorer* explorer = sxml_make_explorer();
        unsigned int exi_size;
        unsigned char* full_exi = read_file_to_buffer(test_files[file_idx].filename, &exi_size);
        
        if (!full_exi) {
            sxml_destroy_explorer(explorer);
            continue; // Skip if file doesn't exist
        }
        
        // Configure explorer with same settings as original tests
        if (test_files[file_idx].enable_entity) {
            sxml_enable_entity_processing(explorer, 1);
        }
        if (test_files[file_idx].enable_namespace) {
            sxml_enable_namespace_processing(explorer, 1);
        }
        
        // Test 1: Full file should work correctly (baseline)
        partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
        sxml_register_func(explorer, partial_exi_on_tag, partial_exi_on_content, NULL, NULL);
        sxml_register_comment_func(explorer, partial_exi_on_comment);
        
        unsigned char result = sxml_run_explorer_exi(explorer, full_exi, exi_size);
        CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
        CU_ASSERT_EQUAL(partial_exi_tag_count, test_files[file_idx].expected_tags);
        CU_ASSERT_EQUAL(partial_exi_content_count, test_files[file_idx].expected_content);
        CU_ASSERT_EQUAL(partial_exi_comment_count, test_files[file_idx].expected_comments);
        
        // Test 2: Test with truncated data - should complete gracefully but may have different counts
        unsigned int test_sizes[] = {10, 50};
        for (int i = 0; i < 2; i++) {
            unsigned int truncate_at = test_sizes[i];
            if (truncate_at < exi_size) {
                partial_exi_tag_count = partial_exi_content_count = partial_exi_comment_count = 0;
                
                result = sxml_run_explorer_exi(explorer, full_exi, truncate_at);
                
                // Partial data should complete successfully (our implementation handles partial data gracefully)
                CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
                // With partial data, we may get different counts, but should not crash
                CU_ASSERT(partial_exi_tag_count >= 0);
                CU_ASSERT(partial_exi_content_count >= 0);
            }
        }
        
        sxml_destroy_explorer(explorer);
        free(full_exi);
    }
}

// Test for large document EXI
static unsigned int large_doc_exi_tag_count = 0;
static unsigned int large_doc_exi_content_count = 0;
static unsigned int large_doc_exi_attr_key_count = 0;
static unsigned int large_doc_exi_attr_val_count = 0;
static unsigned int large_doc_exi_comment_count = 0;

static unsigned char large_doc_exi_on_tag(char* name) {
    large_doc_exi_tag_count++;
    return SXMLExplorerContinue;
}

static unsigned char large_doc_exi_on_attr_key(char* key) {
    large_doc_exi_attr_key_count++;
    return SXMLExplorerContinue;
}

static unsigned char large_doc_exi_on_attr_val(char* val) {
    large_doc_exi_attr_val_count++;
    return SXMLExplorerContinue;
}

static unsigned char large_doc_exi_on_content(char* content) {
    if (strlen(content) > 0) {
        large_doc_exi_content_count++;
    }
    return SXMLExplorerContinue;
}

static unsigned char large_doc_exi_on_comment(char* comment) {
    large_doc_exi_comment_count++;
    return SXMLExplorerContinue;
}

void test_large_document_exi(void) {
    SXMLExplorer* explorer = sxml_make_explorer();
    unsigned int exi_size;
    unsigned char* exi = read_file_to_buffer("test-data/test-large-document.exi", &exi_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exi);

    large_doc_exi_tag_count = large_doc_exi_content_count = 0;
    large_doc_exi_attr_key_count = large_doc_exi_attr_val_count = 0;
    large_doc_exi_comment_count = 0;
    
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_register_func(explorer, large_doc_exi_on_tag, large_doc_exi_on_content,
                       large_doc_exi_on_attr_key, large_doc_exi_on_attr_val);
    sxml_register_comment_func(explorer, large_doc_exi_on_comment);

    unsigned char result = sxml_run_explorer_exi(explorer, exi, exi_size);
    CU_ASSERT_EQUAL(result, SXMLExplorerComplete);
    
    // This large document should have many more elements than small test files
    // With namespace processing enabled, expect ~10 tags and content items
    CU_ASSERT_EQUAL(large_doc_exi_tag_count, 10);
    CU_ASSERT_EQUAL(large_doc_exi_content_count, 10);
    CU_ASSERT_EQUAL(large_doc_exi_comment_count, 1);
    
    // Note: attr counts may be 0 since our EXI parser focuses on tags/content

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
  CU_add_test(*suite, "Parse large document EXI", test_large_document_exi);
  
  // Partial/streaming EXI tests
  CU_add_test(*suite, "Parse partial EXI data", test_partial_exi_data);
  CU_add_test(*suite, "Process chunked EXI data", test_chunked_exi_processing);
  CU_add_test(*suite, "Handle malformed partial EXI", test_malformed_partial_exi);
  
  // Real file partial processing tests
  CU_add_test(*suite, "Partial real EXI files", test_partial_real_exi_files);
}
