#ifndef TINYXML_BENCH_H
#define TINYXML_BENCH_H

#include <stddef.h>

/*
 * C interface to TinyXML-2 for the benchmarks (bench/tinyxml_bench.cpp).
 * tinyxml_load_string() parses the whole document into a DOM, as an
 * application using TinyXML-2 would.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TinyXMLDoc TinyXMLDoc;

// Parse xml into a DOM; NULL on a parse error
TinyXMLDoc* tinyxml_load_string(const char *xml);
// Heap allocated while parsing (document, nodes, copied text)
size_t tinyxml_mem_usage(TinyXMLDoc *doc);
// Number of elements in the DOM
unsigned int tinyxml_element_count(TinyXMLDoc *doc);
void tinyxml_free(TinyXMLDoc *doc);

#ifdef __cplusplus
}
#endif

#endif /* TINYXML_BENCH_H */
