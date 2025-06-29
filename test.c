#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "sparsexml.h"

void add_private_test(CU_pSuite*);
void add_oss_xml_tests(CU_pSuite*);
void add_entity_tests(CU_pSuite*);

void test_parse_simple_xml(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.1\"?><tag></tag>";
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
  ret = sxml_run_explorer(explorer, xml);
  CU_ASSERT (ret == SXMLExplorerInterrupted);
  sxml_destroy_explorer(explorer);
}

void test_check_event_on_content(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.1\"?><tag>1 0<tag2>2</tag2>3</tag>";
  unsigned char ret;

  unsigned char on_content(char *content) {
    static int c = 0;
    switch (content[0]) {
      case '1':
        if (content[2] == '0') {
          CU_ASSERT (c++ == 0);
        }
        break;
      case '2':
        c++;
        break;
      case '3':
        c++;
        break;
    }
    if (c==3) {
      return SXMLExplorerStop;
    }
    return SXMLExplorerContinue;
  }

  explorer = sxml_make_explorer();
  sxml_register_func(explorer, NULL, &on_content, NULL, NULL);
  ret = sxml_run_explorer(explorer, xml);
  CU_ASSERT (ret == SXMLExplorerInterrupted);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_attribute(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.1\"?><tag hoge=\"fuga\" no=\"</tag>\"><tag2 goe=\"ds\"   /></tag>";
  unsigned int c=0;

  unsigned char on_attribute_key(char *name) {
    if (strcmp("hoge", name) == 0 && c == 1) {
      c++;
    }
    if (strcmp("no", name) == 0 && c == 3) {
      c++;
    }
    if (strcmp("goe", name) == 0 && c == 6) {
      c++;
    }
    return SXMLExplorerContinue;
  }

  unsigned char on_attribute_value(char *name) {
    if (strcmp("fuga", name) == 0 && c == 2) {
      c++;
    }
    if (strcmp("</tag>", name) == 0 && c == 4) {
      c++;
    }
    if (strcmp("ds", name) == 0 && c == 7) {
      c++;
    }
    return SXMLExplorerContinue;
  }

  unsigned char on_tag(char *name) {
    if (strcmp("tag", name) == 0 && c == 0) {
      c++;
    }
    if (strcmp("tag2", name) == 0 && c == 5) {
      c++;
    }
    if (strcmp("/tag", name) == 0 && c == 8) {
      c++;
    }
    return SXMLExplorerContinue;
  }

  explorer = sxml_make_explorer();
  sxml_register_func(explorer, on_tag, NULL, on_attribute_key, on_attribute_value);
  sxml_run_explorer(explorer, xml);
  CU_ASSERT(c == 9);

  sxml_destroy_explorer(explorer);
}

void test_check_parsing_comments(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><!-- First comment --><root>content<!-- Second comment --></root>";
  unsigned int comment_count = 0;
  
  unsigned char on_comment(char *comment) {
    comment_count++;
    if (comment_count == 1) {
      CU_ASSERT(strcmp(comment, " First comment ") == 0);
    } else if (comment_count == 2) {
      CU_ASSERT(strcmp(comment, " Second comment ") == 0);
    }
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_register_func(explorer, NULL, NULL, NULL, NULL);
  sxml_register_comment_func(explorer, on_comment);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(comment_count == 2);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_cdata(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><root><![CDATA[<tag>content & special chars</tag>]]></root>";
  unsigned int content_count = 0;
  
  unsigned char on_content(char *content) {
    content_count++;
    if (content_count == 1) {
      CU_ASSERT(strcmp(content, "<tag>content & special chars</tag>") == 0);
    }
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_register_func(explorer, NULL, on_content, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(content_count == 1);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_entities(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><root>simple content</root>";
  unsigned int content_count = 0;
  
  unsigned char on_content(char *content) {
    content_count++;
    if (content_count == 1) {
      CU_ASSERT(strcmp(content, "simple content") == 0);
    }
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_enable_entity_processing(explorer, 1);
  sxml_register_func(explorer, NULL, on_content, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(content_count == 1);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_namespaces(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><ns:root xmlns:ns=\"http://example.com\"><ns:child>content</ns:child></ns:root>";
  unsigned int tag_count = 0;
  
  unsigned char on_tag(char *name) {
    tag_count++;
    if (tag_count == 1) {
      CU_ASSERT(strcmp(name, "root") == 0);  // namespace processing strips prefix
    } else if (tag_count == 2) {
      CU_ASSERT(strcmp(name, "child") == 0);
    } else if (tag_count == 3) {
      CU_ASSERT(strcmp(name, "child") == 0);  // closing tag
    } else if (tag_count == 4) {
      CU_ASSERT(strcmp(name, "root") == 0);   // closing tag
    }
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_enable_namespace_processing(explorer, 1);
  sxml_register_func(explorer, on_tag, NULL, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(tag_count == 4);
  sxml_destroy_explorer(explorer);
}

void test_check_parsing_doctype(void) {
  SXMLExplorer* explorer;
  char xml[] = "<?xml version=\"1.0\"?><!DOCTYPE root SYSTEM \"example.dtd\"><root>content</root>";
  unsigned int tag_count = 0;
  
  unsigned char on_tag(char *name) {
    tag_count++;
    if (tag_count == 1) {
      CU_ASSERT(strcmp(name, "root") == 0);
    } else if (tag_count == 2) {
      CU_ASSERT(strcmp(name, "/root") == 0);
    }
    return SXMLExplorerContinue;
  }
  
  explorer = sxml_make_explorer();
  sxml_register_func(explorer, on_tag, NULL, NULL, NULL);
  sxml_run_explorer(explorer, xml);
  
  CU_ASSERT(tag_count == 2);
  sxml_destroy_explorer(explorer);
}

int main(void) {
  CU_pSuite suite;
  CU_initialize_registry();

  suite = CU_add_suite("SparseXML", NULL, NULL);
  add_private_test(&suite);
  CU_add_test(suite, "Parse simple XML", test_parse_simple_xml);
  CU_add_test(suite, "Check status in running explorer", test_check_event_on_content);
  CU_add_test(suite, "Check parsing attribute", test_check_parsing_attribute);
  CU_add_test(suite, "Check XML comment parsing", test_check_parsing_comments);
  CU_add_test(suite, "Check CDATA section parsing", test_check_parsing_cdata);
  CU_add_test(suite, "Check entity reference parsing", test_check_parsing_entities);
  CU_add_test(suite, "Check namespace parsing", test_check_parsing_namespaces);
  CU_add_test(suite, "Check DOCTYPE parsing", test_check_parsing_doctype);
  add_oss_xml_tests(&suite);
  add_entity_tests(&suite);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}
