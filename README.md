SparseXML: Simple, tiny and no-functional, but useful parser framework for XML

=Abstract
SparseXML is not XML parser library that is just for explorer of XML. For tiny computers like arduino(atmega), it is difficult to use general parser library(e.g. libxml2) because of its tiny hardware resource. Therefore there are some tiny implementation of Streaming API. However they can not treat XML correctly. And they provide mainly halfway. Thus SparseXML is developed. SparseXML is a explorer program it simply reads XML and occurs event at each elements of XML.
Using SparseXML, you can develop your own tiny XML parser more easily. SparseXML provides enough benefits to many scnenes of parsing XML on tiny computer.

=Usage
Look inside test code. it's totally simple, make explorer, register functions, feed xml to explorer then it occurs events, and finally destroy explorer. That's it.