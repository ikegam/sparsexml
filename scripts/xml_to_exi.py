#!/usr/bin/env python3
import sys
import xml.etree.ElementTree as ET

if len(sys.argv) != 3:
    print("Usage: xml_to_exi.py input.xml output.exi", file=sys.stderr)
    sys.exit(1)

input_path, output_path = sys.argv[1], sys.argv[2]
parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
root = ET.parse(input_path, parser).getroot()

exi = bytearray()

def add_text(text):
    if text is not None:
        data = text.strip()
        if data:
            b = data.encode('utf-8')
            exi.append(0x04)
            exi.append(len(b))
            exi.extend(b)

def encode(elem):
    if elem.tag is ET.Comment:
        data = (elem.text or '').encode('utf-8')
        exi.append(0x05)
        exi.append(len(data))
        exi.extend(data)
        add_text(elem.tail)
        return
    tag = elem.tag
    if tag.startswith('{'):
        tag = tag.split('}',1)[1]
    name_b = tag.encode('utf-8')
    exi.append(0x01)
    exi.append(len(name_b))
    exi.extend(name_b)
    for k,v in elem.attrib.items():
        if k.startswith('{'):
            k = k.split('}',1)[1]
        kb = k.encode('utf-8')
        vb = v.encode('utf-8')
        exi.append(0x03)
        exi.append(len(kb))
        exi.extend(kb)
        exi.append(len(vb))
        exi.extend(vb)
    add_text(elem.text)
    for child in list(elem):
        encode(child)
    exi.append(0x02)
    exi.append(len(name_b))
    exi.extend(name_b)
    add_text(elem.tail)

encode(root)
exi.append(0xFF)
with open(output_path, 'wb') as f:
    f.write(exi)
