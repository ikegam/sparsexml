#ifndef __SXML_TEST_LOG__
#define __SXML_TEST_LOG__

#include "sparsexml.h"

/*
 * Event recorder for tests. The log is attached to an explorer as user data,
 * and the callbacks find it again through the explorer, so several logs can
 * be recorded at the same time without globals.
 *
 * Events are stored as "T[name]", "K[key]", "V[value]", "C[text]" and
 * "M[comment]".
 */

#define TEST_LOG_EVENTS 512
#define TEST_LOG_LENGTH 256

// Record comments
#define TEST_LOG_COMMENTS 0x01
// Compare XML with EXI: drop whitespace-only text, join adjacent text, drop
// namespace declarations and xsi:schemaLocation (EXIficient drops them)
#define TEST_LOG_CANONICAL 0x02
// Report attribute keys without prefix
#define TEST_LOG_LOCAL_KEYS 0x04

typedef struct {
  char events[TEST_LOG_EVENTS][TEST_LOG_LENGTH];
  unsigned int count;
  unsigned char flags;
  unsigned char skip_value;
  char joined[TEST_LOG_EVENTS * 16];
} TestLog;

// Clear the log and register its callbacks with the explorer
void test_log_attach(SXMLExplorer* explorer, TestLog* log, unsigned char flags);
// All events concatenated, e.g. "T[a]C[1]T[/a]"
const char* test_log_string(TestLog* log);
// Index of an event, or -1
int test_log_find(const TestLog* log, const char* event);

#endif
