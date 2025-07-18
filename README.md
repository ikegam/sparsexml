# SparseXML: Minimal XML Parser for Embedded Systems

## Overview
SparseXML is a lightweight XML "explorer" designed for resource-constrained embedded systems like Arduino (ATmega328P). Unlike full-featured XML parsers such as libxml2, SparseXML prioritizes minimal memory footprint and simple event-driven parsing.

The library implements a SAX-like streaming parser that generates events for XML elements, attributes, and content without building a DOM tree. This approach makes it suitable for microcontrollers with limited RAM and processing power.

## Key Features
- **Minimal Memory Usage**: Fixed 1KB buffer, no dynamic memory allocation beyond initial explorer
- **Event-Driven**: SAX-style callback system for tags, attributes, and content
- **Streaming Support**: Can process XML data in chunks for large documents
- **Embedded-Friendly**: Written in C with minimal dependencies
- **Simple API**: Easy-to-use callback-based interface

## File Overview
This repository contains example programs and a set of tests, but the actual
library is intentionally tiny.  The parser is built from just three source
files:

1. `sparsexml.h` – public API header
2. `sparsexml-priv.h` – internal definitions
3. `sparsexml.c` – implementation

For users of the library, only `sparsexml.h` needs to be included and
understood.  All other files are for examples and test cases, emphasizing the
minimalism of the implementation.

## API Reference

### Core Functions
```c
SXMLExplorer* sxml_make_explorer(void);
void sxml_destroy_explorer(SXMLExplorer* explorer);
void sxml_register_func(SXMLExplorer* explorer, 
                       void* tag_func, 
                       void* content_func, 
                       void* attribute_key_func, 
                       void* attribute_value_func);
void sxml_register_comment_func(SXMLExplorer* explorer, void* comment_func);
void sxml_enable_entity_processing(SXMLExplorer* explorer, unsigned char enable);
void sxml_enable_namespace_processing(SXMLExplorer* explorer, unsigned char enable);
void sxml_enable_extended_entities(SXMLExplorer* explorer, unsigned char enable);
void sxml_enable_numeric_entities(SXMLExplorer* explorer, unsigned char enable);
unsigned char sxml_run_explorer(SXMLExplorer* explorer, char* xml);
```

### Callback Function Signatures
```c
unsigned char tag_callback(char* tag_name);
unsigned char content_callback(char* content);
unsigned char attribute_key_callback(char* key);
unsigned char attribute_value_callback(char* value);
unsigned char comment_callback(char* comment_text);
```

### Return Values
- `SXMLExplorerContinue (0x00)`: Continue parsing
- `SXMLExplorerStop (0x01)`: Stop parsing immediately
- `SXMLExplorerComplete (0x02)`: Parsing completed successfully
- `SXMLExplorerInterrupted (0x03)`: Parsing stopped by callback
- `SXMLExplorerErrorInvalidEntity (0x04)`: Invalid entity reference encountered
- `SXMLExplorerErrorBufferOverflow (0x05)`: Buffer overflow occurred
- `SXMLExplorerErrorMalformedXML (0x06)`: Malformed XML detected

## Usage Example
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

unsigned char on_comment(char* comment) {
    printf("Comment: %s\n", comment);
    return SXMLExplorerContinue;
}

int main() {
    SXMLExplorer* explorer = sxml_make_explorer();
    sxml_register_func(explorer, on_tag, on_content, NULL, NULL);
    sxml_register_comment_func(explorer, on_comment);
    
    // Enable advanced features
    sxml_enable_entity_processing(explorer, 1);
    sxml_enable_namespace_processing(explorer, 1);
    sxml_enable_extended_entities(explorer, 1);
    sxml_enable_numeric_entities(explorer, 1);
    
    char xml[] = "<?xml version=\"1.0\"?><!-- Comment --><ns:root>&lt;content&gt; &copy; &#65;</ns:root>";
    unsigned char result = sxml_run_explorer(explorer, xml);
    
    sxml_destroy_explorer(explorer);
    return 0;
}
```

## Example Program
A minimal demo is available in the `examples` directory. Build with `make` and run:

```bash
./examples/simple
```
to see parsing output.

## Limitations
⚠️ **Important**: This is a minimal parser with several limitations:

### Security Concerns
- ✅ **Buffer overflow protection**: Now includes boundary checks
- ✅ **Entity reference handling**: Standard XML entities now supported
- ✅ **Enhanced error handling**: Specific error codes for different failure modes

### XML Features Supported ✅
- ✅ XML comments (`<!-- -->`)
- ✅ CDATA sections (`<![CDATA[]]>`)
- ✅ **Standard XML entities** (`&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`) - **COMPLETED!**
- ✅ **Numeric character references** (`&#65;`, `&#x41;`) - **COMPLETED!**
- ✅ **Extended HTML entities** (`&copy;`, `&reg;`, `&trade;`, `&nbsp;`, etc.) - **COMPLETED!**
- ✅ XML namespaces (`ns:tag` format)
- ✅ Basic DOCTYPE parsing
- ✅ Buffer overflow protection
- ✅ Enhanced error handling with specific error codes
- ✅ **Configurable entity processing** with enable/disable flags - **COMPLETED!**
- ✅ Basic XML structure parsing

### XML Features Not Supported
- Complex DTD validation
- Processing instructions beyond declaration
- Custom entity definitions
- XML Schema validation
- Advanced namespace URIs

## Building and Testing
```bash
make
./test-sparsexml
./examples/simple
```

## Memory Requirements
- **RAM**: ~1KB for parsing buffer + minimal stack usage
- **Flash**: ~2KB for code (varies by compiler/architecture)

## Benchmark Results
SparseXML provides competitive performance compared to popular XML parsers.  The
following table summarizes memory usage and execution time on an AMD EPYC 7571
(gcc -O0) when parsing different workloads:

| Benchmark     | Param  | SparseXML (bytes) | Expat (bytes) | TinyXML (bytes) | SparseXML time(s) | Expat time(s) | Tiny time(s) |
|---------------|-------:|------------------:|--------------:|----------------:|------------------:|--------------:|-------------:|
| basic         | 100000 | 1144              | 1048          | 96              | 0.052120          | 0.273421      | 0.003042     |
| large_mem     |   1000 | 1152              | 35952         | 28048           | 0.000184          | 0.000454      | 0.000001     |
| deep_nesting  |    100 | 1152              | 18448         | 1552            | 0.000011          | 0.000022      | 0.000002     |
| many_attrs    |     50 | 1152              | 10000         | 432             | 0.000005          | 0.000099      | 0.000000     |
| comments      |    100 | 1152              | 4176          | 1648            | 0.000008          | 0.000006      | 0.000001     |
| entities      |   1000 | 1152              | 18512         | 15040           | 0.000061          | 0.000003      | 0.000000     |

## Use Cases
- Configuration file parsing on microcontrollers
- Simple data exchange protocols
- IoT sensor data processing
- Embedded web service responses

## License
This project is licensed under the BSD 4-Clause License - see the [LICENSE](LICENSE) file for details.

## Latest Updates (v2.1) - Entity Processing Complete!
Major enhancements completed in this version:
- ✅ **Comprehensive Entity Processing**: 
  - Standard XML entities (`&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`)
  - Numeric character references (`&#65;`, `&#x41;`)
  - Extended HTML entities (`&copy;`, `&reg;`, `&trade;`, `&nbsp;`, `&euro;`, `&pound;`)
- ✅ **Configurable Entity Support**: Enable/disable different entity types individually
- ✅ **Robust Buffer Management**: Fixed critical buffer management issues in entity processing
- ✅ **Enhanced Testing**: Comprehensive test suite with 24 test cases including real-world XML
- ✅ **XML namespace support**: Parse `ns:tag` format
- ✅ **Better error handling**: Return specific error codes
- ✅ **DOCTYPE parsing**: Basic DOCTYPE declaration support
- ✅ **Enhanced API**: New enable/disable functions for optional features

### Entity Processing Features:
```c
// Enable different types of entity processing
sxml_enable_entity_processing(explorer, 1);      // Standard XML entities
sxml_enable_numeric_entities(explorer, 1);       // &#65; and &#x41; format
sxml_enable_extended_entities(explorer, 1);      // &copy;, &reg;, etc.
```

### Supported Entity Types:
- **Standard XML**: `&lt;` → `<`, `&gt;` → `>`, `&amp;` → `&`, `&quot;` → `"`, `&apos;` → `'`
- **Numeric Decimal**: `&#65;` → `A`, `&#32;` → space
- **Numeric Hexadecimal**: `&#x41;` → `A`, `&#x20;` → space
- **Extended HTML**: `&copy;` → `(c)`, `&reg;` → `(R)`, `&trade;` → `(TM)`, `&nbsp;` → space, `&euro;` → `E`, `&pound;` → `#`

## Future Enhancements
Potential improvements for future versions:
- **Configurable buffer size**: Runtime buffer size configuration
- **Validation modes**: Optional well-formedness checking  
- **Custom entity definitions**: Support for user-defined entities
- **Advanced namespace handling**: Full namespace URI support
- **Streaming performance**: Optimizations for large document processing
- **Memory optimization**: Further reduce RAM usage for ultra-constrained devices

## Contributing
This is a minimal implementation focused on embedded use cases. When contributing:
1. Maintain minimal memory footprint
2. Avoid dynamic memory allocation
3. Test on resource-constrained targets
4. Document any new limitations
5. Ensure backward compatibility
