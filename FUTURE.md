# Future ideas

Proposals that are not implemented. They are optional helpers around the library and do not
change its core.

## Schema checks in the development environment

SparseXML does not validate against a schema at runtime: on a microcontroller that would cost
code size and RAM for little benefit, because the peer's message format is fixed when the
firmware is built. The schema of the peer is still useful, but on the host:

> The schema never goes to the device. Only constants, tables and checks derived from it are
> used, at build time and in host tests.

The helpers do not implement the specifications themselves. XML Schema alone is hundreds of
pages, and a partial reimplementation would disagree with real validators in the corner cases
that matter. Each helper builds on an established, complete implementation instead and only
turns its result into something SparseXML can use:

- **libxml2** for XML and XML Schema: validation (`xmllint`) and the compiled schema model that
  the generators read.
- **EXIficient** for EXI: encoding the test data (already used by `test-data/`) and the
  schema-informed grammars.

SparseXML stays a small parser on the device; correctness against the specifications comes
from these tools on the host.

The steps below build on each other and can be adopted one at a time.

### 1. Validate messages on the host

Keep the peer's XSD next to captured or hand-written sample messages, and add a target such as
`make -C test-data validate SCHEMA=peer.xsd` that

- validates the XML samples with `xmllint --schema`,
- decodes the EXI samples with EXIficient and validates the result the same way,
- checks that SparseXML reports the same events as a reference parser for each sample
  (the comparison used by `test-exi.c` already does this).

This answers both "are the messages really what the schema says?" and "does SparseXML read
them correctly?" in one place. It needs no new code in the library.

### 2. Generate a header from the schema

A host tool (for example `tools/sxml-schema`) loads the XSD with libxml2 and writes
`peer_schema.h` from the schema that libxml2 has parsed and resolved. Includes, imports,
substitution groups, type derivation and occurrence limits are then interpreted exactly as
`xmllint` interprets them in step 1, and the tool itself only walks the result. It can be written
in C against libxml2 or in Python with lxml, which wraps libxml2. The header contains:

- **Name IDs**: an enum of element and attribute names (`PEER_TAG_temperature`, ...) and a
  name-to-ID table. Applications switch on IDs, so a misspelt element name becomes a compile
  error instead of a silently ignored event.
- **Static limits**: maximum nesting depth, longest text from `xs:maxLength`, enumeration
  values. They are checked against the library configuration with `_Static_assert`, e.g.
  "`SXMLElementLength` is too small for this message" or "`SXML_EXI_MAX_DEPTH` is too shallow",
  before the firmware reaches the device.
- **Structure table** (host tests only): allowed children per element. A validation mode that
  is compiled in only for host tests compares the reported events with the table; target
  builds leave it out entirely.

On the device this adds at most the name-to-ID lookup, and nothing if it is not used.

Open questions:

- How much of libxml2's compiled schema is reachable through its public API
  (`schemasInternals.h`), and whether some information has to be read from the XSD document
  with libxml2's tree API instead.
- Which parts of the schema to turn into tables (sequence/choice/occurrence limits, simple type
  facets); the rest is still checked on the host by step 1.
- Whether callbacks should receive IDs directly (a new registration function) or applications
  look names up themselves.

### 3. Generate schema-informed EXI grammars

EXI streams encoded with a schema are much smaller than schema-less ones, and the decoder needs
almost no workspace for string tables and learned grammars (32KB by default today). The
grammars can be computed on the host and stored as `const` tables in flash, so the device
decodes schema-informed EXI without processing the schema at runtime. ISO 15118 (EV charging,
e.g. OpenV2G) uses this approach.

- The grammar construction of EXIficient could be reused, so this tool would be written in Java.
- The decoder would gain a schema-informed mode that walks the generated tables (typed values,
  strict and non-strict grammars).

This has the largest benefit and the largest effort; it is worth starting when there is a peer
that sends schema-informed EXI.

### Suggested order

Start with 1, then 2. Begin 3 once a concrete EXI peer exists.
