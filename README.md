# SparseXML
**Minimal XML Parser for Embedded Systems**

SparseXML is a lightweight XML parser designed for resource-constrained embedded systems. It provides SAX-style event-driven parsing with minimal memory footprint (~1KB) and zero external dependencies.

## Integration
Simply copy these 3 files to your project:
- `sparsexml.h` – Public API header
- `sparsexml-priv.h` – Private definitions
- `sparsexml.c` – Implementation

Then include `sparsexml.h` in your code. No build system or external dependencies required.

## Features
- **Minimal Memory**: Fixed 1KB buffer, no dynamic allocation
- **Event-Driven**: SAX-style callbacks for tags, attributes, content
- **Entity Processing**: Standard XML, numeric, and HTML entities
- **EXI Support**: Schema-less EXI binary format parsing  
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
    
    char xml[] = "<root>Hello &amp; World</root>";
    sxml_run_explorer(ex, xml);
    
    sxml_destroy_explorer(ex);
    return 0;
}
```

## Building and Testing
```bash
make
./test-sparsexml
./examples/simple
```

## Memory Requirements
- **RAM**: ~1KB for parsing buffer + minimal stack usage
- **Flash**: ~2KB for code (varies by compiler/architecture)

## Supported XML Features
- ✅ Basic XML structure parsing
- ✅ XML comments (`<!-- -->`)
- ✅ CDATA sections (`<![CDATA[]]>`)
- ✅ Standard XML entities (`&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`)
- ✅ Numeric character references (`&#65;`, `&#x41;`)
- ✅ Extended HTML entities (`&copy;`, `&reg;`, `&trade;`, `&nbsp;`, etc.)
- ✅ XML namespaces (`ns:tag` format)
- ✅ Basic DOCTYPE parsing
- ✅ Buffer overflow protection
- ✅ Configurable entity processing

## EXI Support
Parse W3C EXI binary files in schema-less mode:
```c
// Parse EXI binary data
unsigned char result = sxml_run_explorer_exi(explorer, exi_data, exi_size);
```

- Compatible with exificient-generated EXI files
- Dynamic string table management
- Generic content classification

## Configuration Options
```c
sxml_enable_entity_processing(explorer, 1);      // Standard XML entities
sxml_enable_numeric_entities(explorer, 1);       // &#65; format
sxml_enable_extended_entities(explorer, 1);      // &copy; format
sxml_enable_namespace_processing(explorer, 1);   // ns:tag format
```

## Use Cases
- Configuration file parsing on microcontrollers
- IoT sensor data processing
- Embedded web service responses
- Simple data exchange protocols

## License
BSD 4-Clause License - see [LICENSE](LICENSE) file.
