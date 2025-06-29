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
    
    char xml[] = "<?xml version=\"1.0\"?><!-- This is a comment --><root>Hello <![CDATA[World & Universe]]></root>";
    unsigned char result = sxml_run_explorer(explorer, xml);
    
    sxml_destroy_explorer(explorer);
    return 0;
}
```

## Limitations
⚠️ **Important**: This is a minimal parser with several limitations:

### Security Concerns
- ✅ **Buffer overflow protection**: Now includes boundary checks
- **No entity reference handling**: `&lt;`, `&amp;` etc. not processed  
- **Limited error handling**: May crash on malformed XML

### XML Features Supported ✅
- ✅ XML comments (`<!-- -->`) - **NEW!**
- ✅ CDATA sections (`<![CDATA[]]>`) - **NEW!**
- ✅ Buffer overflow protection - **FIXED!**
- ✅ Basic XML structure parsing

### XML Features Not Supported
- Processing instructions beyond declaration
- XML namespaces (`ns:tag`)
- DOCTYPE declarations  
- Entity references (`&lt;`, `&gt;`, `&amp;`, etc.)
- XML validation

### Known Issues (Fixed)
- ~~Buffer overflow risk~~ ✅ **FIXED**: Now includes buffer boundary checks
- ~~Missing break statement~~ ✅ **FIXED**: Added missing break in attribute parsing
- Limited error handling for malformed XML remains

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

## Future Enhancements
Potential improvements for future versions:
- **Entity reference support**: Handle `&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`
- **XML namespace support**: Parse `ns:tag` format
- **Better error handling**: Return specific error codes
- **DOCTYPE parsing**: Basic DOCTYPE declaration support
- **Configurable buffer size**: Runtime buffer size configuration
- **Validation modes**: Optional well-formedness checking

## Contributing
This is a minimal implementation focused on embedded use cases. When contributing:
1. Maintain minimal memory footprint
2. Avoid dynamic memory allocation
3. Test on resource-constrained targets
4. Document any new limitations
5. Ensure backward compatibility
