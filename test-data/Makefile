# Generate the EXI test data from the XML sources with EXIficient,
# the W3C EXI reference implementation.
#
#   make          encode .exi files whose .xml is newer
#   make -B       re-encode all .exi files (also re-downloads the jars)
#   make check    verify the committed .exi files match a fresh encoding
#   make distclean  remove the downloaded EXIficient jars
#
# Requires java and curl. The jars are downloaded from Maven Central into
# exificient/ and verified against the SHA-256 sums below.

JAVA ?= java
CURL ?= curl
SHA256SUM ?= sha256sum
MAVEN ?= https://repo1.maven.org/maven2

JAR_DIR = exificient
JARS = $(JAR_DIR)/exificient-1.0.4.jar \
       $(JAR_DIR)/exificient-core-1.0.4.jar \
       $(JAR_DIR)/exificient-grammars-1.0.4.jar \
       $(JAR_DIR)/xercesImpl-2.12.2.jar \
       $(JAR_DIR)/xml-apis-1.4.01.jar

$(JAR_DIR)/exificient-1.0.4.jar: URL = $(MAVEN)/com/siemens/ct/exi/exificient/1.0.4/exificient-1.0.4.jar
$(JAR_DIR)/exificient-1.0.4.jar: SHA256 = 79f2717bbd8cd16c610d5d11d219d02a4b0d90b7c160fac2eb5343cc886ac655
$(JAR_DIR)/exificient-core-1.0.4.jar: URL = $(MAVEN)/com/siemens/ct/exi/exificient-core/1.0.4/exificient-core-1.0.4.jar
$(JAR_DIR)/exificient-core-1.0.4.jar: SHA256 = 6dc25bcd044669c2701ba3758b0a47113ae50ac2de1176e54bc7b4065e44a2f1
$(JAR_DIR)/exificient-grammars-1.0.4.jar: URL = $(MAVEN)/com/siemens/ct/exi/exificient-grammars/1.0.4/exificient-grammars-1.0.4.jar
$(JAR_DIR)/exificient-grammars-1.0.4.jar: SHA256 = ef6ef91d5a7dd1542dc747692810aa6d1d39317fedece6e3bcf963caf23a1afb
$(JAR_DIR)/xercesImpl-2.12.2.jar: URL = $(MAVEN)/xerces/xercesImpl/2.12.2/xercesImpl-2.12.2.jar
$(JAR_DIR)/xercesImpl-2.12.2.jar: SHA256 = 6fc991829af1708d15aea50c66f0beadcd2cfeb6968e0b2f55c1b0909883fe16
$(JAR_DIR)/xml-apis-1.4.01.jar: URL = $(MAVEN)/xml-apis/xml-apis/1.4.01/xml-apis-1.4.01.jar
$(JAR_DIR)/xml-apis-1.4.01.jar: SHA256 = a840968176645684bb01aed376e067ab39614885f9eee44abe35a5f20ebe7fad

empty :=
space := $(empty) $(empty)
CLASSPATH = $(subst $(space),:,$(JARS))
EXI_ENCODE = $(JAVA) -cp $(CLASSPATH) com.siemens.ct.exi.main.cmd.EXIficientCMD -encode -noSchema

FIDELITY = -preserveComments -preservePIs -preserveDTDs -preservePrefixes

# Output directory, "make check" encodes into a scratch directory
OUT ?= .

# Encoded with default options: <name>.xml -> <name>.exi
DEFAULT_EXI = test-oss-1.exi \
              test-rss.exi \
              test-atom-entry.exi \
              test-sitemap.exi \
              test-large-document.exi \
              test-with-cdata.exi \
              test-with-comments.exi \
              test-features.exi \
              test-xsi-type.exi

# Encoded with options (see the rules below)
OPTION_EXI = test-with-comments-preserved.exi \
             test-features-fidelity.exi \
             test-features-bytealigned.exi \
             test-features-fidelity-bytealigned.exi \
             test-features-header-options.exi

EXI_FILES = $(DEFAULT_EXI) $(OPTION_EXI)

all: $(addprefix $(OUT)/,$(EXI_FILES))

$(addprefix $(OUT)/,$(DEFAULT_EXI)): $(OUT)/%.exi: %.xml | $(JARS)
	$(EXI_ENCODE) -i $< -o $@

$(OUT)/test-with-comments-preserved.exi: test-with-comments.xml | $(JARS)
	$(EXI_ENCODE) -preserveComments -i $< -o $@

$(OUT)/test-features-fidelity.exi: test-features.xml | $(JARS)
	$(EXI_ENCODE) $(FIDELITY) -i $< -o $@

$(OUT)/test-features-bytealigned.exi: test-features.xml | $(JARS)
	$(EXI_ENCODE) -bytePacked -i $< -o $@

$(OUT)/test-features-fidelity-bytealigned.exi: test-features.xml | $(JARS)
	$(EXI_ENCODE) $(FIDELITY) -bytePacked -i $< -o $@

$(OUT)/test-features-header-options.exi: test-features.xml | $(JARS)
	$(EXI_ENCODE) -includeOptions -includeCookie -i $< -o $@

$(JARS):
	@mkdir -p $(JAR_DIR)
	$(CURL) -fsSL -o $@.tmp $(URL)
	echo "$(SHA256)  $@.tmp" | $(SHA256SUM) -c --quiet -
	mv $@.tmp $@

# Encode everything into a scratch directory and compare with the committed files
CHECK_DIR = .exi-check
check: | $(JARS)
	@rm -rf $(CHECK_DIR) && mkdir -p $(CHECK_DIR)
	@$(MAKE) --no-print-directory -s OUT=$(CHECK_DIR) all
	@status=0; \
	for f in $(EXI_FILES); do \
	  if cmp -s $$f $(CHECK_DIR)/$$f; then echo "ok       $$f"; \
	  else echo "MISMATCH $$f"; status=1; fi; \
	done; \
	rm -rf $(CHECK_DIR); exit $$status

distclean:
	rm -rf $(JAR_DIR) $(CHECK_DIR)

.DELETE_ON_ERROR:
.PHONY: all check distclean
