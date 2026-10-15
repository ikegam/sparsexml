# SparseXML
**Minimal XML Parser for Embedded Systems**

SparseXML is a lightweight XML parser designed for resource-constrained embedded systems. It provides SAX-style event-driven parsing with minimal memory footprint (~1.2KB parser state) and zero external dependencies. It can also decode schema-less W3C EXI (binary XML) into the same callbacks.

## Integration
Simply copy these 3 files to your project:
- `sparsexml.h` – Public API header
- `sparsexml-priv.h` – Private definitions
- `sparsexml.c` – Implementation

Then include `sparsexml.h` in your code. No build system or external dependencies required.

## Features
- **Minimal Memory**: Fixed buffers inside the parser, no allocation while parsing XML
- **Heap-free Option**: Place the parser in your own memory with `sxml_init_explorer()`
- **No Globals**: All state lives in the explorer, callbacks reach their own data through it
- **Streaming**: Feed XML in chunks split at any position
- **Event-Driven**: SAX-style callbacks for tags, attributes, content, comments
- **Entity Processing**: Standard XML, numeric (as UTF-8), and HTML entities
- **EXI Support**: Schema-less EXI decoder (built-in grammars, string tables)
- **Embedded-Ready**: Pure C, zero dependencies

## Quick Start
```c
#include "sparsexml.h"

unsigned char on_tag(char* name) { 
    printf("Tag: %s\n", name); 
    return SXMLExplorerContinue; 
}

unsigned char on_content(char* content) { 
    printf("Content: %s\n", content); 
    return SXMLExplorerContinue; 
}

int main() {
    SXMLExplorer* ex = sxml_make_explorer();
    sxml_register_func(ex, on_tag, on_content, NULL, NULL);
    sxml_enable_entity_processing(ex, 1);
    
    char xml[] = "<root>Hello &amp; World<br/></root>";
    sxml_run_explorer(ex, xml);   // Tag: root, Content: Hello & World,
                                  // Tag: br, Tag: /br, Tag: /root
    sxml_destroy_explorer(ex);
    return 0;
}
```

## Explorers
An explorer holds all parsing state and is passed to every function. It is created for one input
format:

| Format | Create | Heap-free | Run |
|--------|--------|-----------|-----|
| XML | `sxml_make_explorer()` | `sxml_init_explorer(mem, size)` | `sxml_run_explorer()` |
| EXI | `sxml_make_exi_explorer()` | `sxml_init_exi_explorer(mem, size)` | `sxml_run_explorer_exi()` |

Both kinds share the same callbacks and options (`sxml_register_func()`, `sxml_enable_*()`), and
running an explorer with the other format returns `SXMLExplorerErrorUnsupported`.
Internally each kind starts with the common part, so the same functions work on both.

Callbacks can keep their state in the explorer instead of globals:
```c
unsigned char count_tags(char* name) {
    unsigned int* count = sxml_get_user_data(sxml_callback_explorer(name));
    (*count)++;
    return SXMLExplorerContinue;
}

unsigned int count = 0;
sxml_set_user_data(ex, &count);
sxml_register_func(ex, count_tags, NULL, NULL, NULL);
```
`sxml_callback_explorer()` only works on the text a callback received.

## Events
| Callback | Called with |
|----------|-------------|
| tag | Element name; end tags are reported as `/name`, also for `<empty/>` elements |
| attribute key / value | Attribute name and value (single or double quoted) |
| content | Text, including CDATA sections and whitespace between elements |
| comment | Comment text |

Return `SXMLExplorerContinue` to keep parsing or `SXMLExplorerStop` to stop.
Data longer than `SXMLElementLength - 1` (1023) bytes is truncated.

### Return values
| Value | Meaning |
|-------|---------|
| `SXMLExplorerComplete` | Input consumed (for XML: ready for the next chunk) |
| `SXMLExplorerInterrupted` | A callback returned `SXMLExplorerStop` |
| `SXMLExplorerErrorInvalidEntity` | Unknown or invalid entity reference |
| `SXMLExplorerErrorBufferOverflow` | EXI workspace too small or nesting deeper than `SXML_EXI_MAX_DEPTH` |
| `SXMLExplorerErrorMalformedXML` | Invalid or truncated EXI stream, NULL input |
| `SXMLExplorerErrorUnsupported` | EXI feature not supported (see below) |

## Streaming
The parser keeps all state in the explorer, so XML can arrive in pieces:
```c
sxml_run_explorer(ex, "<root><ite");
sxml_run_explorer(ex, "m>text</item></root>");
```
Call `sxml_reset_explorer(ex)` before parsing another document with the same explorer.

## Building and Testing
```bash
make
./test-sparsexml
./examples/simple
```
Tests need CUnit, the benchmark needs libexpat.

The EXI test data is generated from the XML files with EXIficient (needs java and curl):
```bash
make -C test-data         # encode .exi files whose .xml changed
make -C test-data check   # verify the committed .exi files are reproducible
```

## Memory Requirements
- **RAM (XML)**: `sxml_explorer_size()` bytes (~1.2KB), no heap use while parsing
- **RAM (EXI)**: `sxml_exi_explorer_size()` bytes (~1.2KB) plus a workspace for string tables and
  learned grammars, 32KB by default (`SXML_EXI_WORKSPACE_SIZE`), allocated per call or supplied
  with `sxml_set_exi_workspace()`
- **Flash**: ~4.2KB XML only (`-DSXML_NO_EXI`), ~9.4KB with EXI (x86-64, `-Os`)

Without heap (memory must be aligned for pointers, hence `void*` arrays):
```c
static void* parser_memory[1280 / sizeof(void*)];
SXMLExplorer* ex = sxml_init_explorer(parser_memory, sizeof(parser_memory));

static void* exi_memory[1280 / sizeof(void*)];
static void* exi_workspace[8192 / sizeof(void*)];
SXMLExplorer* exi = sxml_init_exi_explorer(exi_memory, sizeof(exi_memory));
sxml_set_exi_workspace(exi, exi_workspace, sizeof(exi_workspace));
```

## Supported XML Features
- ✅ Basic XML structure parsing, self-closing tags
- ✅ Single and double quoted attributes, whitespace inside tags
- ✅ XML comments (`<!-- -->`)
- ✅ CDATA sections (`<![CDATA[]]>`)
- ✅ Standard XML entities (`&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`)
- ✅ Numeric character references (`&#65;`, `&#x41;`, non-ASCII as UTF-8)
- ✅ Extended HTML entities (`&copy;`, `&reg;`, `&trade;`, `&nbsp;`, etc.)
- ✅ XML namespaces (`ns:tag` reported as `tag` when enabled)
- ✅ DOCTYPE (skipped, including internal subsets) and processing instructions (skipped)
- ✅ Buffer overflow protection
- ✅ Configurable entity processing

## EXI Support
Decode W3C EXI 1.0 streams that were encoded without a schema (e.g. `exificient -encode -noSchema`):
```c
SXMLExplorer* explorer = sxml_make_exi_explorer();
sxml_register_func(explorer, on_tag, on_content, NULL, NULL);
unsigned char result = sxml_run_explorer_exi(explorer, exi_data, exi_size);
```
EXI events go to the same callbacks as XML. Names are reported as local names unless the stream
preserves prefixes (`SXML_EXI_PRESERVE_PREFIXES`); then they are qualified like in XML (`ns:tag`,
`xml:lang`, `xsi:type` values as `t:Type`), and namespace processing removes the prefix from tags. The whole stream must be in memory; a truncated
stream returns `SXMLExplorerErrorMalformedXML`.

EXI options are not stored in the stream by default, so the decoder must be told the options the
encoder used:
```c
sxml_set_exi_options(explorer, SXML_EXI_PRESERVE_COMMENTS | SXML_EXI_BYTE_ALIGNED);
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

## Configuration Options
```c
sxml_enable_entity_processing(explorer, 1);      // Standard XML entities
sxml_enable_numeric_entities(explorer, 1);       // &#65; format
sxml_enable_extended_entities(explorer, 1);      // &copy; format
sxml_enable_namespace_processing(explorer, 1);   // ns:tag format
```
Compile-time: `SXMLElementLength`, `SXMLNameLength`, `SXML_EXI_WORKSPACE_SIZE`, `SXML_EXI_MAX_DEPTH`, `SXML_NO_EXI`.

## Use Cases
- Configuration file parsing on microcontrollers
- IoT sensor data processing
- Embedded web service responses
- Simple data exchange protocols

## License
BSD 4-Clause License - see [LICENSE](LICENSE) file.
