# SparseXML: Minimal XML Parser for Embedded Systems

## Overview
SparseXML is a lightweight XML "explorer" designed for resource-constrained embedded systems like Arduino (ATmega328P). Unlike full-featured XML parsers such as libxml2, SparseXML prioritizes minimal memory footprint and simple event-driven parsing.

The library implements a SAX-like streaming parser that generates events for XML elements, attributes, and content without building a DOM tree. This approach makes it suitable for microcontrollers with limited RAM and processing power.

## Key Features
- **Minimal Memory Usage**: Fixed 1KB buffer, no dynamic memory allocation beyond initial explorer
- **Event-Driven**: SAX-style callback system for tags, attributes, and content
- **Streaming Support**: Can process XML data in chunks for large documents
- **Embedded-Friendly**: Written in C with minimal dependencies
- **Simple API**: Only 4 functions to learn

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
    
    char xml[] = "<?xml version=\"1.0\"?><!-- Comment --><ns:root>&lt;content&gt;</ns:root>";
    unsigned char result = sxml_run_explorer(explorer, xml);
    
    sxml_destroy_explorer(explorer);
    return 0;
}
```

## Limitations
⚠️ **Important**: This is a minimal parser with several limitations:

### Security Concerns
- ✅ **Buffer overflow protection**: Now includes boundary checks
- ✅ **Entity reference handling**: Standard XML entities now supported
- ✅ **Enhanced error handling**: Specific error codes for different failure modes

### XML Features Supported ✅
- ✅ XML comments (`<!-- -->`)
- ✅ CDATA sections (`<![CDATA[]]>`)
- ✅ Entity references (`&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`) - **NEW!**
- ✅ XML namespaces (`ns:tag` format) - **NEW!**
- ✅ Basic DOCTYPE parsing - **NEW!**
- ✅ Buffer overflow protection
- ✅ Enhanced error handling with specific error codes - **NEW!**
- ✅ Basic XML structure parsing

### XML Features Not Supported
- Complex DTD validation
- Processing instructions beyond declaration
- Custom entity definitions
- XML Schema validation
- Advanced namespace URIs

### Known Issues (Fixed)
- ~~Buffer overflow risk~~ ✅ **FIXED**: Now includes buffer boundary checks
- ~~Missing break statement~~ ✅ **FIXED**: Added missing break in attribute parsing
- ~~No entity support~~ ✅ **FIXED**: Added standard XML entity references
- ~~No namespace support~~ ✅ **FIXED**: Added basic namespace processing

## Building and Testing
```bash
make
./test-sparsexml
```

## Memory Requirements
- **RAM**: ~1KB for parsing buffer + minimal stack usage
- **Flash**: ~2KB for code (varies by compiler/architecture)

## Use Cases
- Configuration file parsing on microcontrollers
- Simple data exchange protocols
- IoT sensor data processing
- Embedded web service responses

## License
This project is licensed under the BSD 4-Clause License - see the [LICENSE](LICENSE) file for details.

## Latest Updates (v2.0)
Major enhancements in this version:
- ✅ **Entity reference support**: Handle `&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`
- ✅ **XML namespace support**: Parse `ns:tag` format
- ✅ **Better error handling**: Return specific error codes
- ✅ **DOCTYPE parsing**: Basic DOCTYPE declaration support
- ✅ **Enhanced API**: New enable/disable functions for optional features

## Future Enhancements
Potential improvements for future versions:
- **Configurable buffer size**: Runtime buffer size configuration
- **Validation modes**: Optional well-formedness checking
- **Custom entity definitions**: Support for user-defined entities
- **Advanced namespace handling**: Full namespace URI support
- **Streaming performance**: Optimizations for large document processing

## Contributing
This is a minimal implementation focused on embedded use cases. When contributing:
1. Maintain minimal memory footprint
2. Avoid dynamic memory allocation
3. Test on resource-constrained targets
4. Document any new limitations
5. Ensure backward compatibility
