# SparseXML

[![CI](https://github.com/ikegam/sparsexml/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/ikegam/sparsexml/actions/workflows/c-cpp.yml)
[![License: BSD-4-Clause](https://img.shields.io/badge/license-BSD--4--Clause-blue.svg)](LICENSE)

**Minimal XML and EXI parser for microcontrollers.**

SparseXML is a SAX-style, event-driven XML parser in plain C for resource-constrained
embedded systems. The parser state is about 1.2KB, it needs no heap while parsing XML,
has no global state and no dependencies. It also decodes schema-less
[W3C EXI](https://www.w3.org/TR/exi/) (binary XML) into the same callbacks.

- [Features](#features)
- [Integration](#integration)
- [Quick start](#quick-start)
- [Explorers](#explorers)
- [Events and return values](#events-and-return-values)
- [Streaming XML](#streaming-xml)
- [EXI](#exi)
- [Memory](#memory)
- [Configuration](#configuration)
- [Building and testing](#building-and-testing)
- [License](#license)

## Features
- **Small**: ~1.2KB of parser state, ~4.2KB of code for XML only (x86-64, `-Os`)
- **No heap, no globals**: place the parser in your own memory; callbacks receive the
  parser and reach their state through it
- **Streaming**: feed XML in chunks split at any position
- **Event-driven**: callbacks for tags, attributes, text and comments
- **Entities**: standard XML, numeric (delivered as UTF-8) and a few HTML entities
- **EXI**: schema-less EXI 1.0 decoder (built-in grammars, string tables)
- **Portable**: plain C, no dependencies

## Integration
Copy three files into your project and include `sparsexml.h`:

| File | Contents |
|------|----------|
| `sparsexml.h` | Public API |
| `sparsexml-priv.h` | Internal definitions |
| `sparsexml.c` | Implementation |

No build system is required. Define `SXML_NO_EXI` to leave out the EXI decoder.

## Quick start
```c
#include <stdio.h>
#include "sparsexml.h"

static unsigned char on_tag(SXMLExplorer* ex, char* name) {
    printf("Tag: %s\n", name);
    return SXMLExplorerContinue;
}

static unsigned char on_content(SXMLExplorer* ex, char* content) {
    printf("Content: %s\n", content);
    return SXMLExplorerContinue;
}

int main(void) {
    SXMLExplorer* ex = sxml_make_explorer();
    sxml_register_func(ex, on_tag, on_content, NULL, NULL);
    sxml_enable_entity_processing(ex, 1);

    char xml[] = "<root>Hello &amp; World<br/></root>";
    sxml_run_explorer(ex, xml);
    // Tag: root, Content: Hello & World, Tag: br, Tag: /br, Tag: /root

    sxml_destroy_explorer(ex);
    return 0;
}
```
More: [`examples/simple.c`](examples/simple.c).

## Explorers
An explorer holds all parsing state and is passed to every function.
It is created for one input format:

| Format | Create | Heap-free | Run |
|--------|--------|-----------|-----|
| XML | `sxml_make_explorer()` | `sxml_init_explorer(mem, size)` | `sxml_run_explorer()` |
| EXI | `sxml_make_exi_explorer()` | `sxml_init_exi_explorer(mem, size)` | `sxml_run_explorer_exi()` |

Both kinds share the callbacks and options (`sxml_register_func()`, `sxml_enable_*()`).
Running an explorer with the other format returns `SXMLExplorerErrorUnsupported`.

Callbacks receive the explorer that reports the event, so they can keep their state in it
instead of globals:
```c
static unsigned char count_tags(SXMLExplorer* ex, char* name) {
    unsigned int* count = sxml_get_user_data(ex);
    (*count)++;
    return SXMLExplorerContinue;
}

unsigned int count = 0;
sxml_set_user_data(ex, &count);
sxml_register_func(ex, count_tags, NULL, NULL, NULL);
```

## Events and return values
| Callback | Called with |
|----------|-------------|
| tag | Element name; end tags as `/name`, also for `<empty/>` elements |
| attribute key / value | Attribute name and value (single or double quoted) |
| content | Text, including CDATA sections and whitespace between elements |
| comment | Comment text |

All callbacks have the type `SXMLCallback`, `unsigned char (*)(SXMLExplorer* ex, char* text)`;
any of them may be `NULL`. `text` lives in the explorer and is valid until the callback
returns. Callbacks return `SXMLExplorerContinue` to go on or `SXMLExplorerStop` to stop.
Data longer than `SXMLElementLength - 1` (1023) bytes is truncated.

| Return value | Meaning |
|--------------|---------|
| `SXMLExplorerComplete` | Input consumed (XML: ready for the next chunk) |
| `SXMLExplorerInterrupted` | A callback returned `SXMLExplorerStop` |
| `SXMLExplorerErrorInvalidEntity` | Unknown or invalid entity reference |
| `SXMLExplorerErrorBufferOverflow` | EXI workspace too small or nesting deeper than `SXML_EXI_MAX_DEPTH` |
| `SXMLExplorerErrorMalformedXML` | Invalid or truncated EXI stream, NULL input |
| `SXMLExplorerErrorUnsupported` | Unsupported EXI feature, or explorer of the other format |

### Supported XML
- Elements, self-closing tags, single and double quoted attributes, whitespace inside tags
- Comments, CDATA sections
- Entities: `&lt;` `&gt;` `&amp;` `&quot;` `&apos;`, `&#65;` / `&#x41;` (as UTF-8),
  `&copy;` `&reg;` `&trade;` `&nbsp;` `&euro;` `&pound;` (as ASCII approximations)
- Namespaces: `ns:tag` reported as `tag` with namespace processing enabled
- XML declaration, processing instructions and DOCTYPE (including internal subsets) are skipped

Not supported: DTD validation, entity declarations, namespace URI resolution.

## Streaming XML
The explorer keeps all state, so XML can arrive in pieces:
```c
sxml_run_explorer(ex, "<root><ite");
sxml_run_explorer(ex, "m>text</item></root>");
```
Call `sxml_reset_explorer(ex)` before parsing another document with the same explorer.

## EXI
Decodes W3C EXI 1.0 streams encoded without a schema (e.g. `exificient -encode -noSchema`):
```c
SXMLExplorer* ex = sxml_make_exi_explorer();
sxml_register_func(ex, on_tag, on_content, NULL, NULL);
unsigned char result = sxml_run_explorer_exi(ex, exi_data, exi_size);
```
EXI events go to the same callbacks as XML. Names are local names unless the stream preserves
prefixes; then they are qualified like in XML (`ns:tag`, `xml:lang`, `xsi:type` values as
`t:Type`), and namespace processing removes the prefix from tags. The whole stream must be in
memory; a truncated stream returns `SXMLExplorerErrorMalformedXML`.

EXI options are normally not stored in the stream, so tell the decoder what the encoder used:
```c
sxml_set_exi_options(ex, SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_BYTE_ALIGNED);
```

| Flag | Encoder option |
|------|----------------|
| `SXML_EXI_PRESERVE_COMMENTS` | `-preserveComments` (comments go to the comment callback) |
| `SXML_EXI_PRESERVE_PIS` | `-preservePIs` (decoded and skipped) |
| `SXML_EXI_PRESERVE_DTD` | `-preserveDTDs` (decoded and skipped) |
| `SXML_EXI_PRESERVE_PREFIXES` | `-preservePrefixes` |
| `SXML_EXI_PRESERVE_LEXICAL_VALUES` | `-preserveLexicalValues` |
| `SXML_EXI_BYTE_ALIGNED` | `-bytePacked` |

Not supported (`SXMLExplorerErrorUnsupported` where detectable): options in the EXI header
(`-includeOptions`), compression and pre-compression, schema-informed streams, self-contained
elements, and bounded `valueMaxLength` / `valuePartitionCapacity`.

## Memory
| | RAM | Code (x86-64, `-Os`) |
|-|-----|----------------------|
| XML | `sxml_explorer_size()` (~1.2KB), no heap while parsing | ~4.2KB with `SXML_NO_EXI` |
| EXI | `sxml_exi_explorer_size()` (~1.2KB) + workspace | ~9.4KB together with XML |

The EXI workspace holds the string tables and learned grammars. By default 32KB
(`SXML_EXI_WORKSPACE_SIZE`) are allocated per call; supply your own with
`sxml_set_exi_workspace()`.

Without heap (memory must be aligned for pointers, hence `void*` arrays):
```c
static void* parser_memory[1280 / sizeof(void*)];
SXMLExplorer* ex = sxml_init_explorer(parser_memory, sizeof(parser_memory));

static void* exi_memory[1280 / sizeof(void*)];
static void* exi_workspace[8192 / sizeof(void*)];
SXMLExplorer* exi = sxml_init_exi_explorer(exi_memory, sizeof(exi_memory));
sxml_set_exi_workspace(exi, exi_workspace, sizeof(exi_workspace));
```

## Configuration
```c
sxml_enable_entity_processing(ex, 1);      // &lt; &amp; ...
sxml_enable_numeric_entities(ex, 1);       // &#65; &#x41;
sxml_enable_extended_entities(ex, 1);      // &copy; ...
sxml_enable_namespace_processing(ex, 1);   // ns:tag -> tag
```

| Compile-time option | Default | Meaning |
|---------------------|---------|---------|
| `SXMLElementLength` | 1024 | Callback buffer size |
| `SXMLNameLength` | 64 | Tag name kept to report the end of `<empty/>` elements |
| `SXML_EXI_WORKSPACE_SIZE` | 32768 | EXI workspace allocated when none is supplied |
| `SXML_EXI_MAX_DEPTH` | 32 | Maximum EXI element nesting |
| `SXML_NO_EXI` | – | Build without the EXI decoder |

## Building and testing
```bash
make                # library, tests, example and benchmark
./test-sparsexml    # CUnit tests
./examples/simple
./bench/bench       # comparison with expat and TinyXML-2
```
Tests need CUnit, the benchmark needs libexpat and TinyXML-2 (C++).

The EXI test data is generated from the XML files with EXIficient (needs java and curl):
```bash
make -C test-data         # encode .exi files whose .xml changed
make -C test-data check   # verify the committed .exi files are reproducible
```

| Path | Contents |
|------|----------|
| `test*.c`, `test-data/` | Tests and test documents |
| `examples/` | Usage example |
| `bench/` | Benchmarks |
| `paper.md` | Design and evaluation notes (Japanese) |
| `FUTURE.md` | Proposals, e.g. schema checks in the development environment |

## License
BSD 4-Clause License, see [LICENSE](LICENSE). Note the advertising clause (clause 3):
advertising materials that mention features or use of this software must display the
acknowledgement given there.
