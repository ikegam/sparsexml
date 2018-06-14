# SparseXML: too Simple and non functional, but useful parser framework for XML

## Abstract
SparseXML is not a XML parser library that is just a ``explorer'' for XML. For tiny computers suck as arduino(atmega328p), it is difficult to use commonly used parser library(e.g. libxml2) because of its tiny hardware resource. Therefore there are some tiny implementation. However they can not treat XML correctly. Almost all of them are partially developed it is not full implementation. Therefore the design principle of SparseXML is just explorer (similar to SAX). SparseXML simply reads XML and occurs event at each elements of XML.
Using SparseXML, you can develop your own tiny XML parser more easily. SparseXML provides enough benefits to many scnenes of parsing XML on tiny computer.

## Usage
Look inside test code. it's totally simple, make explorer, register functions, feed xml to explorer then it occurs events, and finally destroy explorer. That's it.
