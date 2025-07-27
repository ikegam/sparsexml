#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>

#include "sparsexml.h"

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

void add_exi_tests(CU_pSuite* suite) {
  CU_add_test(*suite, "Parse simple EXI", test_parse_simple_exi);
}
