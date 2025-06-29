#include <stdio.h>
#include "sparsexml.h"

static unsigned char on_tag(char *name) {
    printf("TAG: %s\n", name);
    return SXMLExplorerContinue;
}

static unsigned char on_content(char *content) {
    printf("CONTENT: %s\n", content);
    return SXMLExplorerContinue;
}

static unsigned char on_attribute_key(char *key) {
    printf("ATTR KEY: %s\n", key);
    return SXMLExplorerContinue;
}

static unsigned char on_attribute_value(char *value) {
    printf("ATTR VALUE: %s\n", value);
    return SXMLExplorerContinue;
}

static unsigned char on_comment(char *text) {
    printf("COMMENT: %s\n", text);
    return SXMLExplorerContinue;
}

int main(void) {
    SXMLExplorer *explorer = sxml_make_explorer();
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_register_func(explorer, on_tag, on_content,
                       on_attribute_key, on_attribute_value);
    sxml_register_comment_func(explorer, on_comment);

    char xml[] =
        "<?xml version=\"1.0\"?>"
        "<!-- Example XML -->"
        "<ns:root attr=\"value\">Text<child>More</child></ns:root>";

    sxml_run_explorer(explorer, xml);
    sxml_destroy_explorer(explorer);
    return 0;
}
