#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "sparsexml-priv.h"

void test_initialize_parser(void) {
  SXMLParser* parser;

  parser = sxml_init_parser();
  CU_ASSERT (parser->state == INITIAL);
  CU_ASSERT (parser->bp == 0);
  CU_ASSERT (strlen(parser->buffer) == 0);
  sxml_destroy_parser(parser);
}

void test_parse_separated_xml(void) {
  SXMLParser* parser;
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
      return SXMLParserStop;
    }
    return SXMLParserContinue;
  }

  parser = sxml_init_parser();
  sxml_register_func(parser, &on_tag, NULL, NULL, NULL);
  ret = sxml_run_parser(parser, xml1);
  CU_ASSERT (parser->state == IN_HEADER);
  ret = sxml_run_parser(parser, xml2);
  CU_ASSERT (parser->state == IN_HEADER);
  ret = sxml_run_parser(parser, xml3);
  CU_ASSERT (parser->state == IN_TAG);
  CU_ASSERT (strcmp(parser->buffer, "ta") == 0);
  ret = sxml_run_parser(parser, xml4);
  CU_ASSERT (parser->state == IN_CONTENT);
  CU_ASSERT (ret == SXMLParserInterrupted);
  sxml_destroy_parser(parser);
}

void add_private_test(CU_pSuite* suite) {
  CU_add_test(*suite, "initialize phase", test_initialize_parser);
  CU_add_test(*suite, "Parse simple separated XML", test_parse_separated_xml);
}
