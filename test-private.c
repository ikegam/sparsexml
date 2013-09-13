#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "sparsexml-priv.h"

void test_initialize_explorer(void) {
  SXMLExplorer* explorer;

  explorer = sxml_make_explorer();
  CU_ASSERT (explorer->state == INITIAL);
  CU_ASSERT (explorer->bp == 0);
  CU_ASSERT (strlen(explorer->buffer) == 0);
  sxml_destroy_explorer(explorer);
}

void test_parse_separated_xml(void) {
  SXMLExplorer* explorer;
  char xml1[] = "<?xml versi";
  char xml2[] = "on=\"1.1\"?";
  char xml3[] = "><ta";
  char xml4[] = "g></tag>";
  unsigned char ret;

  unsigned char on_tag(char *name) {
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

  explorer = sxml_make_explorer();
  sxml_register_func(explorer, &on_tag, NULL, NULL, NULL);
  ret = sxml_run_explorer(explorer, xml1);
  CU_ASSERT (explorer->state == IN_HEADER);
  ret = sxml_run_explorer(explorer, xml2);
  CU_ASSERT (explorer->state == IN_HEADER);
  ret = sxml_run_explorer(explorer, xml3);
  CU_ASSERT (explorer->state == IN_TAG);
  CU_ASSERT (strcmp(explorer->buffer, "ta") == 0);
  ret = sxml_run_explorer(explorer, xml4);
  CU_ASSERT (explorer->state == IN_CONTENT);
  CU_ASSERT (ret == SXMLExplorerInterrupted);
  sxml_destroy_explorer(explorer);
}

void add_private_test(CU_pSuite* suite) {
  CU_add_test(*suite, "initialize phase", test_initialize_explorer);
  CU_add_test(*suite, "Parse simple separated XML", test_parse_separated_xml);
}
